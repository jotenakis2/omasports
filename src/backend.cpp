#include "backend.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSet>
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

QString sportSlug(const CatalogEntry &entry)
{
    return QString::fromLatin1(entry.sport);
}

QStringList allSportSlugs()
{
    QStringList slugs;
    for (const auto &entry : catalog) {
        const QString slug = sportSlug(entry);
        if (!slugs.contains(slug))
            slugs.append(slug);
    }
    return slugs;
}

bool isKnownSport(const QString &slug)
{
    for (const auto &entry : catalog)
        if (slug == sportSlug(entry))
            return true;
    return false;
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

QString scoreString(const QJsonValue &score)
{
    if (score.isString())
        return score.toString();
    if (score.isDouble())
        return QString::number(score.toDouble());
    return {};
}

// The header feed and the scoreboard feed spell a competitor differently: one
// flattens the team fields onto the competitor.
QVariantMap headerCompetitor(const QJsonObject &competitor)
{
    QVariantMap team;
    team.insert("home", competitor.value("homeAway").toString() == "home");
    team.insert("homeAway", competitor.value("homeAway").toString());
    team.insert("name", competitor.value("displayName").toString());
    team.insert("shortName", competitor.value("name").toString());
    team.insert("abbreviation", competitor.value("abbreviation").toString());
    team.insert("logo", competitor.value("logo").toString());
    team.insert("color", competitor.value("color").toString());
    team.insert("score", scoreString(competitor.value("score")));
    team.insert("winner", competitor.value("winner").toBool());
    QString record = competitor.value("record").toString();
    if (record.isEmpty())
        record = competitorRecord(competitor);
    team.insert("record", record);
    return team;
}

void orderAwayFirst(QVariantList *teams)
{
    if (teams->size() != 2)
        return;
    const QVariantMap first = teams->first().toMap();
    const QVariantMap second = teams->last().toMap();
    if (first.value("home").toBool() && !second.value("home").toBool()) {
        teams->clear();
        teams->append(second);
        teams->append(first);
    }
}

} // namespace

Backend::Backend(const QString &settingsPath, bool online, QObject *parent)
    : QObject(parent)
    , m_settingsPath(settingsPath.isEmpty() ? defaultSettingsPath() : settingsPath)
    , m_online(online)
    , m_leagues(leagueCatalog())
{
    QSettings settings(m_settingsPath, QSettings::IniFormat);

    // No saved choice means every sport, so a fresh install has something to
    // show. An explicitly saved empty list means the user hid them all.
    const QStringList known = allSportSlugs();
    if (settings.contains("sports/enabled")) {
        const QStringList saved = settings.value("sports/enabled").toStringList();
        for (const auto &slug : known)
            if (saved.contains(slug))
                m_enabledSports.append(slug);
    } else {
        m_enabledSports = known;
    }

    const QString sport = settings.value("league/sport").toString();
    const QString league = settings.value("league/id").toString();
    const bool found = selectBySlug(sport, league);
    if (!found || !m_enabledSports.contains(this->league().value("sport").toString())) {
        const int first = firstEnabledLeague();
        if (first >= 0) {
            m_leagueIndex = first;
            saveSelection();
        } else if (!found) {
            m_leagueIndex = 0;
        }
    }
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

QVariantList Backend::sports() const
{
    QHash<QString, int> counts;
    for (const auto &entry : catalog)
        ++counts[sportSlug(entry)];

    QVariantList sports;
    QSet<QString> seen;
    for (const auto &entry : catalog) {
        const QString slug = sportSlug(entry);
        if (seen.contains(slug))
            continue;
        seen.insert(slug);

        QVariantMap sport;
        sport.insert("slug", slug);
        sport.insert("name", tr(entry.sportName));
        sport.insert("leagues", counts.value(slug));
        sport.insert("enabled", m_enabledSports.contains(slug));
        sports.append(sport);
    }
    return sports;
}

void Backend::selectLeague(int index)
{
    if (index < 0 || index >= m_leagues.size())
        return;
    if (!m_enabledSports.contains(m_leagues.at(index).toMap().value("sport").toString()))
        return;

    const bool leavingLive = m_liveMode;
    if (index == m_leagueIndex && !leavingLive)
        return;

    m_liveMode = false;
    m_leagueIndex = index;
    m_events.clear();
    m_liveCount = 0;
    m_updatedAt = QDateTime();
    m_status = m_online ? "loading" : "idle";
    m_error.clear();
    saveSelection();
    if (leavingLive)
        emit liveModeChanged();
    notifyAll();
    refresh();
}

void Backend::nextDay()
{
    if (m_liveMode) {
        m_liveMode = false;
        emit liveModeChanged();
    }
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
    if (m_liveMode) {
        m_liveMode = false;
        emit liveModeChanged();
    }
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
    const bool wasLive = m_liveMode;
    if (wasLive) {
        m_liveMode = false;
        emit liveModeChanged();
    }
    if (m_dayOffset == 0 && !wasLive) {
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

void Backend::toggleLive()
{
    setLive(!m_liveMode);
}

void Backend::setLive(bool live)
{
    if (live == m_liveMode) {
        if (live)
            refresh();
        return;
    }

    m_liveMode = live;
    m_events.clear();
    m_liveCount = 0;
    m_updatedAt = QDateTime();
    m_error.clear();
    m_status = m_online ? "loading" : "idle";
    emit liveModeChanged();
    notifyAll();
    refresh();
}

void Backend::setSportEnabled(const QString &slug, bool enabled)
{
    if (!isKnownSport(slug))
        return;
    if (m_enabledSports.contains(slug) == enabled)
        return;

    if (enabled) {
        // Rebuild in catalog order, so the sidebar sections keep their order.
        QStringList ordered;
        for (const auto &entry : catalog) {
            const QString candidate = sportSlug(entry);
            if ((candidate == slug || m_enabledSports.contains(candidate))
                && !ordered.contains(candidate))
                ordered.append(candidate);
        }
        m_enabledSports = ordered;
    } else {
        m_enabledSports.removeAll(slug);
    }

    saveSports();
    emit sportsChanged();
    applySportsChange();
}

void Backend::setAllSportsEnabled(bool enabled)
{
    const QStringList desired = enabled ? allSportSlugs() : QStringList();
    if (desired == m_enabledSports)
        return;

    m_enabledSports = desired;
    saveSports();
    emit sportsChanged();
    applySportsChange();
}

void Backend::applySportsChange()
{
    const bool hidden = !m_enabledSports.contains(league().value("sport").toString());
    int first = -1;
    if (hidden) {
        first = firstEnabledLeague();
        if (first >= 0) {
            m_leagueIndex = first;
            saveSelection();
            emit leagueChanged();
        }
    }

    // Live mode scans again so hidden sports disappear from the list at once.
    if (m_liveMode) {
        refresh();
        return;
    }
    if (!hidden)
        return;

    m_events.clear();
    m_liveCount = 0;
    m_updatedAt = QDateTime();
    m_error.clear();
    if (first >= 0) {
        m_status = m_online ? "loading" : "idle";
        notifyAll();
        refresh();
    } else {
        m_status = "ready";
        notifyAll();
    }
}

void Backend::refresh()
{
    if (!m_online)
        return;

    cancelPending();
    if (m_liveMode) {
        fetchLive();
        return;
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

void Backend::fetchLive()
{
    m_status = "loading";
    m_error.clear();
    emit statusChanged();

    QNetworkRequest request(liveHeaderUrl());
    request.setTransferTimeout(20000);
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
        const QVariantList all = parseLiveHeader(payload, &parseError);
        if (!parseError.isEmpty()) {
            m_status = "error";
            m_error = parseError;
            emit statusChanged();
            return;
        }

        QVariantList visible;
        for (const auto &event : all) {
            if (m_enabledSports.contains(event.toMap().value("sport").toString()))
                visible.append(event);
        }

        m_events = visible;
        m_liveCount = visible.size();
        m_updatedAt = QDateTime::currentDateTime();
        m_status = "ready";
        m_error.clear();
        notifyAll();
    });
}

void Backend::cancelPending()
{
    if (!m_pending)
        return;
    // Clearing the pointer first makes the abort's finished signal a no-op,
    // and keeps the old reply alive until deleteLater runs.
    QNetworkReply *obsolete = m_pending.data();
    m_pending.clear();
    obsolete->abort();
    obsolete->deleteLater();
}

void Backend::retranslate()
{
    m_leagues = leagueCatalog();
    emit leaguesChanged();
    emit leagueChanged();
    emit dateChanged();
    emit eventsChanged();
    emit sportsChanged();
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

void Backend::saveSports() const
{
    QDir().mkpath(QFileInfo(m_settingsPath).absolutePath());
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    settings.setValue("sports/enabled", m_enabledSports);
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

int Backend::firstEnabledLeague() const
{
    for (int i = 0; i < m_leagues.size(); ++i) {
        const QVariantMap entry = m_leagues.at(i).toMap();
        if (m_enabledSports.contains(entry.value("sport").toString()))
            return i;
    }
    return -1;
}

QString Backend::recordLabel(const QString &sport, const QString &record) const
{
    if (record.isEmpty())
        return {};

    const QStringList parts = record.split('-');
    QStringList letters;
    if (sport == "soccer") {
        letters = {tr("W"), tr("D"), tr("L")};
    } else if (sport == "hockey") {
        letters = {tr("W"), tr("L"), tr("OT")};
    } else if (sport == "football") {
        letters = parts.size() == 3 ? QStringList{tr("W"), tr("L"), tr("T")}
                                    : QStringList{tr("W"), tr("L")};
    } else if (sport == "mma") {
        letters = parts.size() == 3 ? QStringList{tr("W"), tr("L"), tr("D")}
                                    : QStringList{tr("W"), tr("L")};
    } else if (sport == "basketball" || sport == "baseball" || sport == "volleyball") {
        letters = {tr("W"), tr("L")};
    } else {
        return record;
    }

    if (parts.size() != letters.size())
        return record;

    static const QRegularExpression digits("^\\d+$");
    QStringList formatted;
    for (int i = 0; i < parts.size(); ++i) {
        if (!digits.match(parts.at(i)).hasMatch())
            return record;
        formatted.append(parts.at(i) + letters.at(i));
    }
    return formatted.join(" · ");
}

QVariantList Backend::leagueCatalog()
{
    QVariantList leagues;
    int index = 0;
    for (const auto &entry : catalog) {
        QVariantMap league;
        league.insert("index", index++);
        league.insert("sport", sportSlug(entry));
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

QUrl Backend::liveHeaderUrl()
{
    return QUrl(QStringLiteral("https://site.web.api.espn.com/apis/v2/scoreboard/header"));
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
            item.insert("score", scoreString(competitor.value("score")));
            item.insert("winner", competitor.value("winner").toBool());
            item.insert("record", competitorRecord(competitor));
            teams.append(item);
        }

        // ESPN lists the home side first in most feeds; the card reads away
        // then home, like a scoreboard.
        orderAwayFirst(&teams);

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

QVariantList Backend::parseLiveHeader(const QByteArray &payload, QString *error)
{
    if (error)
        error->clear();

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error)
            *error = parseError.errorString().isEmpty()
                       ? QStringLiteral("Unexpected response from the live scoreboard")
                       : parseError.errorString();
        return {};
    }

    // The catalog already knows the translated sport names.
    QHash<QString, QString> sportNames;
    for (const auto &league : leagueCatalog()) {
        const QVariantMap entry = league.toMap();
        sportNames.insert(entry.value("sport").toString(), entry.value("sportName").toString());
    }

    QVariantList events;
    for (const auto &sportValue : document.object().value("sports").toArray()) {
        const QJsonObject sport = sportValue.toObject();
        const QString sportName = sportNames.value(sport.value("slug").toString(),
                                                   sport.value("name").toString());
        for (const auto &leagueValue : sport.value("leagues").toArray()) {
            const QJsonObject league = leagueValue.toObject();
            const QString leagueName = league.value("shortName").toString(
                                           league.value("name").toString());

            QVariantList leagueEvents;
            for (const auto &eventValue : league.value("events").toArray()) {
                const QJsonObject event = eventValue.toObject();
                const QJsonObject fullStatus = event.value("fullStatus").toObject();
                const QJsonObject type = fullStatus.value("type").toObject();
                const QString state = type.value("state").toString(
                    event.value("status").toString());
                if (state != "in")
                    continue;

                QVariantList teams;
                for (const auto &competitor : event.value("competitors").toArray())
                    teams.append(headerCompetitor(competitor.toObject()));
                orderAwayFirst(&teams);

                QVariantMap item;
                item.insert("id", event.value("id").toString());
                item.insert("name", event.value("name").toString());
                item.insert("shortName", event.value("shortName").toString());
                item.insert("start", event.value("date").toString());
                item.insert("state", "in");
                item.insert("detail", type.value("detail").toString());
                item.insert("shortDetail", type.value("shortDetail").toString());
                item.insert("completed", false);
                item.insert("teams", teams);
                item.insert("venue", event.value("location").toString());
                item.insert("sport", sport.value("slug").toString());
                item.insert("leagueName", leagueName);
                item.insert("sectionLabel", sportName.isEmpty()
                                               ? leagueName
                                               : sportName + " · " + leagueName);
                leagueEvents.append(item);
            }

            std::sort(leagueEvents.begin(), leagueEvents.end(),
                      [](const QVariant &left, const QVariant &right) {
                          return left.toMap().value("start").toString()
                               < right.toMap().value("start").toString();
                      });
            events += leagueEvents;
        }
    }
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
