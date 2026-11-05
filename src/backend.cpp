#include "backend.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QStandardPaths>
#include <QUrlQuery>

#include <algorithm>

namespace {

struct CatalogEntry {
    const char *sport;
    const char *sportName;
    const char *league;
    const char *name;
};

// ESPN's slugs, kept to the competitions people actually follow, across every
// sport the API covers. Adding a league is one line; its scoreboard is at
// https://site.api.espn.com/apis/site/v2/sports/<sport>/<league>/scoreboard.
const CatalogEntry catalog[] = {
    {"soccer", "Soccer", "eng.1", "Premier League"},
    {"soccer", "Soccer", "esp.1", "LaLiga"},
    {"soccer", "Soccer", "ita.1", "Serie A"},
    {"soccer", "Soccer", "ger.1", "Bundesliga"},
    {"soccer", "Soccer", "fra.1", "Ligue 1"},
    {"soccer", "Soccer", "uefa.champions", "Champions League"},
    {"soccer", "Soccer", "uefa.europa", "Europa League"},
    {"soccer", "Soccer", "usa.1", "MLS"},
    {"soccer", "Soccer", "mex.1", "Liga MX"},
    {"soccer", "Soccer", "bra.1", "Brasileirão"},
    {"soccer", "Soccer", "arg.1", "Liga Profesional"},
    {"soccer", "Soccer", "ned.1", "Eredivisie"},
    {"soccer", "Soccer", "por.1", "Primeira Liga"},
    {"football", "American Football", "nfl", "NFL"},
    {"football", "American Football", "college-football", "College Football"},
    {"basketball", "Basketball", "nba", "NBA"},
    {"basketball", "Basketball", "wnba", "WNBA"},
    {"basketball", "Basketball", "mens-college-basketball", "College Basketball"},
    {"baseball", "Baseball", "mlb", "MLB"},
    {"baseball", "Baseball", "college-baseball", "College Baseball"},
    {"hockey", "Ice Hockey", "nhl", "NHL"},
    {"tennis", "Tennis", "atp", "ATP"},
    {"tennis", "Tennis", "wta", "WTA"},
    {"golf", "Golf", "pga", "PGA Tour"},
    {"golf", "Golf", "lpga", "LPGA"},
    {"mma", "MMA", "ufc", "UFC"},
    {"racing", "Racing", "f1", "Formula 1"},
    {"racing", "Racing", "nascar-premier", "NASCAR Cup Series"},
    {"rugby", "Rugby", "180659", "Six Nations"},
    {"rugby", "Rugby", "164205", "Rugby World Cup"},
    {"cricket", "Cricket", "8039", "Cricket World Cup"},
    {"volleyball", "Volleyball", "womens-college-volleyball", "College Volleyball"},
    {"australian-football", "Australian Football", "afl", "AFL"},
};

QString defaultSettingsPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/settings.ini";
}

QString teamLogo(const QJsonObject &team)
{
    const QString logo = team.value("logo").toString();
    if (!logo.isEmpty())
        return logo;
    const auto logos = team.value("logos").toArray();
    return logos.isEmpty() ? QString() : logos.first().toObject().value("href").toString();
}

// "12-4" for the season so far, when the feed gives one.
QString competitorRecord(const QJsonObject &competitor)
{
    QString fallback;
    for (const auto &entry : competitor.value("records").toArray()) {
        const auto record = entry.toObject();
        const QString summary = record.value("summary").toString();
        if (summary.isEmpty())
            continue;
        if (record.value("name").toString() == "overall")
            return summary;
        if (fallback.isEmpty())
            fallback = summary;
    }
    return fallback;
}

} // namespace

Backend::Backend(const QString &settingsPath, bool online, QObject *parent)
    : QObject(parent)
    , m_settingsPath(settingsPath.isEmpty() ? defaultSettingsPath() : settingsPath)
    , m_online(online)
    , m_leagues(leagueCatalog())
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    const QString sport = settings.value("league/sport").toString();
    const QString league = settings.value("league/id").toString();
    if (!selectBySlug(sport, league))
        m_leagueIndex = 0;
}

QVariantMap Backend::league() const
{
    return m_leagues.value(m_leagueIndex).toMap();
}

QString Backend::dateLabel() const
{
    return QLocale().toString(date(), "ddd d MMM yyyy");
}

QString Backend::updatedLabel() const
{
    if (!m_updatedAt.isValid())
        return {};
    return QLocale().toString(m_updatedAt.time(), QLocale::ShortFormat);
}

void Backend::selectLeague(int index)
{
    if (index < 0 || index >= m_leagues.size() || index == m_leagueIndex)
        return;

    m_leagueIndex = index;
    m_events.clear();
    m_liveCount = 0;
    m_updatedAt = QDateTime();
    m_status = m_online ? "loading" : "idle";
    m_error.clear();
    saveSelection();
    notifyAll();
    refresh();
}

void Backend::nextDay()
{
    m_dayOffset = qMin(m_dayOffset + 1, 366);
    m_events.clear();
    m_liveCount = 0;
    m_status = m_online ? "loading" : "idle";
    m_error.clear();
    notifyAll();
    refresh();
}

void Backend::previousDay()
{
    m_dayOffset = qMax(m_dayOffset - 1, -366);
    m_events.clear();
    m_liveCount = 0;
    m_status = m_online ? "loading" : "idle";
    m_error.clear();
    notifyAll();
    refresh();
}

void Backend::today()
{
    if (m_dayOffset == 0) {
        refresh();
        return;
    }
    m_dayOffset = 0;
    m_events.clear();
    m_liveCount = 0;
    m_status = m_online ? "loading" : "idle";
    m_error.clear();
    notifyAll();
    refresh();
}

void Backend::refresh()
{
    if (!m_online)
        return;

    // A new league or day makes the reply in flight obsolete, and ESPN's API
    // answers a request per league and day, so cancel rather than wait.
    // Clearing the pointer first makes the abort's finished signal a no-op,
    // and keeps the old reply alive until deleteLater runs.
    if (m_pending) {
        QNetworkReply *obsolete = m_pending.data();
        m_pending.clear();
        obsolete->abort();
        obsolete->deleteLater();
    }

    const QVariantMap league = this->league();
    if (league.isEmpty())
        return;

    m_status = "loading";
    m_error.clear();
    emit statusChanged();

    const QUrl url = scoreboardUrl(league.value("sport").toString(),
                                   league.value("league").toString(), date());
    QNetworkRequest request(url);
    request.setTransferTimeout(15000);
    request.setHeader(QNetworkRequest::UserAgentHeader, "omasports/0.1");

    QNetworkReply *reply = m_network.get(request);
    m_pending = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (reply != m_pending)
            return;
        m_pending.clear();
        const QByteArray payload = reply->readAll();
        const QString networkError = reply->error() == QNetworkReply::NoError
                                       ? QString() : reply->errorString();
        reply->deleteLater();

        if (!networkError.isEmpty()) {
            m_status = "error";
            m_error = networkError;
            emit statusChanged();
            return;
        }

        QString parseError;
        const QVariantList events = parseScoreboard(payload, &parseError);
        if (!parseError.isEmpty()) {
            m_status = "error";
            m_error = parseError;
            emit statusChanged();
            return;
        }

        m_events = events;
        m_liveCount = liveCountOf(events);
        m_updatedAt = QDateTime::currentDateTime();
        m_status = "ready";
        m_error.clear();
        notifyAll();
    });
}

void Backend::retranslate()
{
    m_leagues = leagueCatalog();
    emit leaguesChanged();
    emit leagueChanged();
    emit dateChanged();
    emit eventsChanged();
}

void Backend::notifyAll()
{
    emit statusChanged();
    emit leagueChanged();
    emit eventsChanged();
    emit dateChanged();
}

void Backend::saveSelection() const
{
    const QVariantMap selected = league();
    QDir().mkpath(QFileInfo(m_settingsPath).absolutePath());
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    settings.setValue("league/sport", selected.value("sport"));
    settings.setValue("league/id", selected.value("league"));
}

bool Backend::selectBySlug(const QString &sport, const QString &league)
{
    for (int i = 0; i < m_leagues.size(); ++i) {
        const QVariantMap entry = m_leagues.at(i).toMap();
        if (entry.value("sport").toString() == sport && entry.value("league").toString() == league) {
            m_leagueIndex = i;
            return true;
        }
    }
    return false;
}

QVariantList Backend::leagueCatalog()
{
    QVariantList leagues;
    int index = 0;
    for (const auto &entry : catalog) {
        QVariantMap league;
        league.insert("index", index++);
        league.insert("sport", QString::fromLatin1(entry.sport));
        league.insert("sportName", tr(entry.sportName));
        league.insert("league", QString::fromLatin1(entry.league));
        league.insert("name", QString::fromUtf8(entry.name));
        leagues.append(league);
    }
    return leagues;
}

QUrl Backend::scoreboardUrl(const QString &sport, const QString &league, const QDate &date)
{
    QUrl url(QStringLiteral("https://site.api.espn.com/apis/site/v2/sports/%1/%2/scoreboard")
                 .arg(sport, league));
    QUrlQuery query;
    // ESPN expects the day in its own format, and reads ?dates=YYYYMMDD.
    query.addQueryItem("dates", date.toString("yyyyMMdd"));
    url.setQuery(query);
    return url;
}

QVariantList Backend::parseScoreboard(const QByteArray &payload, QString *error)
{
    if (error)
        error->clear();

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error)
            *error = parseError.errorString().isEmpty()
                       ? QStringLiteral("Unexpected response from the scoreboard")
                       : parseError.errorString();
        return {};
    }

    QVariantList events;
    for (const auto &entry : document.object().value("events").toArray()) {
        const QJsonObject event = entry.toObject();
        const QJsonArray competitions = event.value("competitions").toArray();
        const QJsonObject competition = competitions.isEmpty()
                                          ? QJsonObject() : competitions.first().toObject();

        QJsonObject status = competition.value("status").toObject();
        if (status.isEmpty())
            status = event.value("status").toObject();
        const QJsonObject type = status.value("type").toObject();

        QVariantList teams;
        for (const auto &competitorEntry : competition.value("competitors").toArray()) {
            const QJsonObject competitor = competitorEntry.toObject();
            const QJsonObject team = competitor.value("team").toObject();
            const QJsonObject athlete = competitor.value("athlete").toObject();

            QVariantMap item;
            item.insert("home", competitor.value("homeAway").toString() == "home");
            item.insert("homeAway", competitor.value("homeAway").toString());
            item.insert("name", team.value("displayName").toString(
                                    athlete.value("displayName").toString()));
            item.insert("shortName", team.value("shortDisplayName").toString(
                                         athlete.value("shortName").toString()));
            item.insert("abbreviation", team.value("abbreviation").toString());
            item.insert("logo", teamLogo(team));
            item.insert("color", team.value("color").toString());
            const QJsonValue score = competitor.value("score");
            item.insert("score", score.isString() ? score.toString()
                                  : score.isDouble() ? QString::number(score.toDouble()) : QString());
            item.insert("winner", competitor.value("winner").toBool());
            item.insert("record", competitorRecord(competitor));
            teams.append(item);
        }

        // ESPN lists the home side first in most feeds; the card reads away
        // then home, like a scoreboard.
        if (teams.size() == 2) {
            const QVariantMap first = teams.first().toMap();
            const QVariantMap second = teams.last().toMap();
            if (first.value("home").toBool() && !second.value("home").toBool()) {
                teams.clear();
                teams.append(second);
                teams.append(first);
            }
        }

        QVariantMap item;
        item.insert("id", event.value("id").toString());
        item.insert("name", event.value("name").toString());
        item.insert("shortName", event.value("shortName").toString());
        item.insert("start", event.value("date").toString());
        item.insert("state", type.value("state").toString());
        item.insert("detail", type.value("detail").toString());
        item.insert("shortDetail", type.value("shortDetail").toString());
        item.insert("completed", type.value("completed").toBool());
        item.insert("teams", teams);
        item.insert("venue", competition.value("venue").toObject().value("fullName").toString());
        events.append(item);
    }

    std::sort(events.begin(), events.end(), [](const QVariant &left, const QVariant &right) {
        return left.toMap().value("start").toString() < right.toMap().value("start").toString();
    });
    return events;
}

int Backend::liveCountOf(const QVariantList &events)
{
    int count = 0;
    for (const auto &event : events)
        if (event.toMap().value("state").toString() == "in")
            ++count;
    return count;
}
