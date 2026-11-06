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
#include <QTime>
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

QString leagueKey(const QString &sport, const QString &league)
{
    return sport + '/' + league;
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

    // No saved choice means every league, so a fresh install has something to
    // show. An explicitly saved empty list means the user hid them all. The
    // older per-sport setting seeds the per-league one the first time.
    QSet<QString> saved;
    if (settings.contains("leagues/enabled")) {
        for (const auto &key : settings.value("leagues/enabled").toStringList())
            saved.insert(key);
    } else if (settings.contains("sports/enabled")) {
        const QStringList sports = settings.value("sports/enabled").toStringList();
        for (const auto &entry : catalog) {
            if (sports.contains(sportSlug(entry)))
                saved.insert(leagueKey(sportSlug(entry), QString::fromLatin1(entry.league)));
        }
        settings.remove("sports/enabled");
    } else {
        for (const auto &entry : catalog)
            saved.insert(leagueKey(sportSlug(entry), QString::fromLatin1(entry.league)));
    }

    // Keep only leagues the catalog still knows, in catalog order.
    for (const auto &entry : catalog) {
        const QString key = leagueKey(sportSlug(entry), QString::fromLatin1(entry.league));
        if (saved.contains(key))
            m_enabledLeagues.insert(key);
    }

    const QString sport = settings.value("league/sport").toString();
    const QString league = settings.value("league/id").toString();
    const bool found = selectBySlug(sport, league);
    if (!found || !leagueEnabled(this->league().value("sport").toString(),
                                this->league().value("league").toString())) {
        const int first = firstEnabledLeague();
        if (first >= 0) {
            m_leagueIndex = first;
            const QVariantMap selected = this->league();
            settings.setValue("league/sport", selected.value("sport"));
            settings.setValue("league/id", selected.value("league"));
        } else if (!found) {
            m_leagueIndex = 0;
        }
    }

    m_standingsHeaders = standingsHeadersFor(this->league().value("sport").toString());

    // Persist the normalized list, including an empty one, so "none" survives
    // the next launch instead of falling back to every league.
    settings.setValue("leagues/enabled", enabledLeagues());
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

QVariantList Backend::settingsRows() const
{
    QVariantList rows;
    QSet<QString> seenSports;
    for (const auto &entry : catalog) {
        const QString sport = sportSlug(entry);
        const QString league = QString::fromLatin1(entry.league);

        if (!seenSports.contains(sport)) {
            seenSports.insert(sport);
            int total = 0;
            int enabled = 0;
            for (const auto &candidate : catalog) {
                if (sportSlug(candidate) != sport)
                    continue;
                ++total;
                if (m_enabledLeagues.contains(
                        leagueKey(sport, QString::fromLatin1(candidate.league))))
                    ++enabled;
            }

            QVariantMap row;
            row.insert("type", "sport");
            row.insert("sport", sport);
            row.insert("name", tr(entry.sportName));
            row.insert("leagues", total);
            row.insert("enabledCount", enabled);
            row.insert("allEnabled", enabled == total);
            rows.append(row);
        }

        QVariantMap row;
        row.insert("type", "league");
        row.insert("sport", sport);
        row.insert("league", league);
        row.insert("name", QString::fromUtf8(entry.name));
        row.insert("enabled", m_enabledLeagues.contains(leagueKey(sport, league)));
        rows.append(row);
    }
    return rows;
}

QStringList Backend::enabledLeagues() const
{
    QStringList keys;
    for (const auto &entry : catalog) {
        const QString key = leagueKey(sportSlug(entry), QString::fromLatin1(entry.league));
        if (m_enabledLeagues.contains(key))
            keys.append(key);
    }
    return keys;
}

void Backend::selectLeague(int index)
{
    if (index < 0 || index >= m_leagues.size())
        return;
    const QVariantMap entry = m_leagues.at(index).toMap();
    if (!leagueEnabled(entry.value("sport").toString(), entry.value("league").toString()))
        return;

    const bool leavingLive = m_liveMode;
    if (index == m_leagueIndex && !leavingLive)
        return;

    m_liveMode = false;
    m_leagueIndex = index;
    m_events.clear();
    m_standings.clear();
    m_liveCount = 0;
    m_updatedAt = QDateTime();
    m_status = m_online ? "loading" : "idle";
    m_error.clear();
    saveSelection();
    if (leavingLive)
        emit liveModeChanged();
    emit standingsChanged();
    notifyAll();
    refresh();
}

void Backend::nextDay()
{
    if (m_liveMode) {
        m_liveMode = false;
        emit liveModeChanged();
    }
    if (m_standingsVisible) {
        m_standingsVisible = false;
        emit standingsChanged();
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
    if (m_standingsVisible) {
        m_standingsVisible = false;
        emit standingsChanged();
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
    const bool wasStandings = m_standingsVisible;
    if (wasLive) {
        m_liveMode = false;
        emit liveModeChanged();
    }
    if (wasStandings) {
        m_standingsVisible = false;
        emit standingsChanged();
    }
    if (m_dayOffset == 0 && !wasLive && !wasStandings) {
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

    if (live && m_standingsVisible) {
        m_standingsVisible = false;
        emit standingsChanged();
    }

    m_liveMode = live;
    m_events.clear();
    m_standings.clear();
    m_liveCount = 0;
    m_updatedAt = QDateTime();
    m_error.clear();
    m_status = m_online ? "loading" : "idle";
    emit liveModeChanged();
    notifyAll();
    refresh();
}

void Backend::toggleStandings()
{
    setStandings(!m_standingsVisible);
}

void Backend::setStandings(bool visible)
{
    if (visible == m_standingsVisible) {
        if (visible)
            refresh();
        return;
    }

    if (visible && m_liveMode) {
        m_liveMode = false;
        emit liveModeChanged();
    }

    m_standingsVisible = visible;
    m_standings.clear();
    m_standingsHeaders = standingsHeadersFor(this->league().value("sport").toString());
    m_events.clear();
    m_liveCount = 0;
    m_updatedAt = QDateTime();
    m_error.clear();
    m_status = m_online ? "loading" : "idle";
    emit standingsChanged();
    notifyAll();
    refresh();
}

void Backend::openMatch(const QString &id)
{
    QVariantMap source;
    for (const auto &value : m_events) {
        const QVariantMap event = value.toMap();
        if (event.value("id").toString() == id) {
            source = event;
            break;
        }
    }
    if (source.isEmpty())
        return;

    cancelMatchPending();
    m_matchOpen = true;
    m_matchId = id;
    m_match = source;
    m_matchStatus = "loading";
    m_matchError.clear();
    emit matchChanged();
    fetchMatch();
}

void Backend::closeMatch()
{
    cancelMatchPending();
    if (!m_matchOpen && m_match.isEmpty() && m_matchStatus.isEmpty())
        return;
    m_matchOpen = false;
    m_matchId.clear();
    m_match.clear();
    m_matchStatus.clear();
    m_matchError.clear();
    emit matchChanged();
}

void Backend::refreshMatch()
{
    if (m_matchOpen)
        fetchMatch();
}

void Backend::cancelMatchPending()
{
    if (!m_matchPending)
        return;
    QNetworkReply *obsolete = m_matchPending.data();
    m_matchPending.clear();
    obsolete->abort();
    obsolete->deleteLater();
}

void Backend::fetchMatch()
{
    if (!m_matchOpen)
        return;
    cancelMatchPending();

    QString sport = m_match.value("sport").toString();
    QString leagueSlug = m_match.value("league").toString();
    if (sport.isEmpty())
        sport = league().value("sport").toString();
    if (leagueSlug.isEmpty())
        leagueSlug = league().value("league").toString();
    const QString id = m_match.value("id").toString();

    if (sport.isEmpty() || leagueSlug.isEmpty() || id.isEmpty()) {
        m_matchStatus = "error";
        m_matchError = tr("Missing match information");
        emit matchChanged();
        return;
    }
    if (!m_online) {
        m_matchStatus = "idle";
        emit matchChanged();
        return;
    }

    m_matchStatus = "loading";
    m_matchError.clear();
    emit matchChanged();

    QUrl url(QStringLiteral("https://site.api.espn.com/apis/site/v2/sports/%1/%2/summary")
                 .arg(sport, leagueSlug));
    QUrlQuery query;
    query.addQueryItem("event", id);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setTransferTimeout(15000);
    request.setHeader(QNetworkRequest::UserAgentHeader, "omasports/0.1");

    QNetworkReply *reply = m_network.get(request);
    m_matchPending = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (reply != m_matchPending)
            return;
        m_matchPending.clear();

        const QByteArray payload = reply->readAll();
        const QString networkError = reply->error() == QNetworkReply::NoError
                                       ? QString() : reply->errorString();
        reply->deleteLater();

        if (!networkError.isEmpty()) {
            m_matchStatus = "error";
            m_matchError = networkError;
            emit matchChanged();
            return;
        }

        QString parseError;
        const QVariantMap parsed = parseMatchSummary(payload, m_match, &parseError);
        if (!parseError.isEmpty()) {
            m_matchStatus = "error";
            m_matchError = parseError;
            emit matchChanged();
            return;
        }

        for (auto it = parsed.begin(); it != parsed.end(); ++it)
            m_match.insert(it.key(), it.value());
        m_matchStatus = "ready";
        m_matchError.clear();
        emit matchChanged();
    });
}

void Backend::setLeagueEnabled(const QString &sport, const QString &league, bool enabled)
{
    bool known = false;
    for (const auto &entry : catalog) {
        if (sportSlug(entry) == sport && league == QLatin1String(entry.league)) {
            known = true;
            break;
        }
    }
    if (!known)
        return;

    const QString key = leagueKey(sport, league);
    if (m_enabledLeagues.contains(key) == enabled)
        return;

    if (enabled)
        m_enabledLeagues.insert(key);
    else
        m_enabledLeagues.remove(key);

    saveLeagues();
    emit leaguesChanged();
    applyLeaguesChange();
}

void Backend::setSportLeaguesEnabled(const QString &sport, bool enabled)
{
    bool changed = false;
    for (const auto &entry : catalog) {
        if (sportSlug(entry) != sport)
            continue;
        const QString key = leagueKey(sport, QString::fromLatin1(entry.league));
        if (enabled && !m_enabledLeagues.contains(key)) {
            m_enabledLeagues.insert(key);
            changed = true;
        } else if (!enabled && m_enabledLeagues.contains(key)) {
            m_enabledLeagues.remove(key);
            changed = true;
        }
    }
    if (!changed)
        return;

    saveLeagues();
    emit leaguesChanged();
    applyLeaguesChange();
}

void Backend::setAllLeaguesEnabled(bool enabled)
{
    QSet<QString> desired;
    if (enabled) {
        for (const auto &entry : catalog)
            desired.insert(leagueKey(sportSlug(entry), QString::fromLatin1(entry.league)));
    }
    if (desired == m_enabledLeagues)
        return;

    m_enabledLeagues = desired;
    saveLeagues();
    emit leaguesChanged();
    applyLeaguesChange();
}

void Backend::applyLeaguesChange()
{
    const bool hidden = !leagueEnabled(league().value("sport").toString(),
                                       league().value("league").toString());
    int first = -1;
    if (hidden) {
        first = firstEnabledLeague();
        if (first >= 0) {
            m_leagueIndex = first;
            saveSelection();
            emit leagueChanged();
        }
    }

    // Live mode scans again so hidden leagues disappear from the list at once.
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
    if (m_standingsVisible) {
        fetchStandings();
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

        QVariantList annotated;
        const QVariantMap current = this->league();
        for (const auto &value : events) {
            QVariantMap event = value.toMap();
            event.insert("sport", current.value("sport"));
            event.insert("league", current.value("league"));
            annotated.append(event);
        }

        m_events = annotated;
        m_liveCount = liveCountOf(annotated);
        m_updatedAt = QDateTime::currentDateTime();
        m_status = "ready";
        m_error.clear();
        notifyAll();
    });
}

void Backend::fetchLive()
{
    m_events.clear();
    m_liveCount = 0;
    m_liveEvents.clear();
    m_liveSeen.clear();
    m_liveQueue.clear();
    m_liveRequests = 0;
    m_liveFailures = 0;
    m_liveError.clear();

    // ESPN groups a game under its US day, which can still be yesterday when
    // the local clock has just crossed midnight, so scan both days early in
    // the morning.
    const QDate today = QDate::currentDate();
    const QList<QDate> days = QTime::currentTime().hour() < 9
                                  ? QList<QDate>{today, today.addDays(-1)}
                                  : QList<QDate>{today};

    for (const auto &value : m_leagues) {
        const QVariantMap entry = value.toMap();
        const QString sport = entry.value("sport").toString();
        const QString league = entry.value("league").toString();
        if (!leagueEnabled(sport, league))
            continue;
        for (const QDate &day : days) {
            LiveTask task;
            task.url = scoreboardUrl(sport, league, day);
            task.sport = sport;
            task.league = league;
            task.sportName = entry.value("sportName").toString();
            task.leagueName = entry.value("name").toString();
            m_liveQueue.append(task);
        }
    }

    m_status = "loading";
    m_error.clear();
    emit statusChanged();

    if (m_liveQueue.isEmpty()) {
        m_status = "ready";
        notifyAll();
        return;
    }

    m_liveRequests = m_liveQueue.size();
    startLiveRequests();
}

void Backend::startLiveRequests()
{
    static constexpr int concurrency = 6;

    while (m_liveReplies.size() < concurrency && !m_liveQueue.isEmpty()) {
        const LiveTask task = m_liveQueue.takeFirst();
        QNetworkRequest request(task.url);
        request.setTransferTimeout(15000);
        request.setHeader(QNetworkRequest::UserAgentHeader, "omasports/0.1");

        QNetworkReply *reply = m_network.get(request);
        m_liveReplies.insert(reply);
        connect(reply, &QNetworkReply::finished, this, [this, reply, task] {
            if (!m_liveReplies.contains(reply))
                return;
            m_liveReplies.remove(reply);

            const QByteArray payload = reply->readAll();
            const QString networkError = reply->error() == QNetworkReply::NoError
                                           ? QString() : reply->errorString();
            reply->deleteLater();

            if (!networkError.isEmpty()) {
                ++m_liveFailures;
                if (m_liveError.isEmpty())
                    m_liveError = networkError;
            } else {
                QString parseError;
                const QVariantList events = parseScoreboard(payload, &parseError);
                if (!parseError.isEmpty()) {
                    ++m_liveFailures;
                    if (m_liveError.isEmpty())
                        m_liveError = parseError;
                } else {
                    const QVariantList live = liveEventsFrom(events, task.sport, task.league,
                                                             task.sportName, task.leagueName);
                    for (const auto &event : live) {
                        const QString id = event.toMap().value("id").toString();
                        if (m_liveSeen.contains(id))
                            continue;
                        m_liveSeen.insert(id);
                        m_liveEvents.append(event);
                    }
                }
            }

            if (!m_liveQueue.isEmpty()) {
                startLiveRequests();
                return;
            }
            if (m_liveReplies.isEmpty())
                finishLive();
        });
    }
}

void Backend::finishLive()
{
    std::sort(m_liveEvents.begin(), m_liveEvents.end(),
              [](const QVariant &left, const QVariant &right) {
                  const QVariantMap a = left.toMap();
                  const QVariantMap b = right.toMap();
                  const QString sectionA = a.value("sectionLabel").toString();
                  const QString sectionB = b.value("sectionLabel").toString();
                  if (sectionA != sectionB)
                      return sectionA < sectionB;
                  return a.value("start").toString() < b.value("start").toString();
              });

    m_events = m_liveEvents;
    m_liveCount = m_events.size();
    m_updatedAt = QDateTime::currentDateTime();

    if (m_events.isEmpty() && m_liveRequests > 0 && m_liveFailures == m_liveRequests) {
        m_status = "error";
        m_error = m_liveError;
        emit statusChanged();
        return;
    }

    m_status = "ready";
    m_error.clear();
    notifyAll();
}

void Backend::cancelLiveRequests()
{
    m_liveQueue.clear();
    const QSet<QNetworkReply *> replies = m_liveReplies;
    m_liveReplies.clear();
    for (QNetworkReply *reply : replies) {
        reply->abort();
        reply->deleteLater();
    }
}

void Backend::cancelPending()
{
    if (m_pending) {
        // Clearing the pointer first makes the abort's finished signal a
        // no-op, and keeps the old reply alive until deleteLater runs.
        QNetworkReply *obsolete = m_pending.data();
        m_pending.clear();
        obsolete->abort();
        obsolete->deleteLater();
    }
    cancelLiveRequests();
}

void Backend::fetchStandings()
{
    const QVariantMap league = this->league();
    if (league.isEmpty())
        return;

    const QString sport = league.value("sport").toString();
    m_standingsHeaders = standingsHeadersFor(sport);
    if (m_standingsHeaders.isEmpty()) {
        m_standings.clear();
        m_updatedAt = QDateTime::currentDateTime();
        m_status = "ready";
        m_error.clear();
        emit standingsChanged();
        emit statusChanged();
        return;
    }

    m_status = "loading";
    m_error.clear();
    emit statusChanged();

    const QUrl url(QStringLiteral("https://site.api.espn.com/apis/v2/sports/%1/%2/standings")
                       .arg(sport, league.value("league").toString()));
    QNetworkRequest request(url);
    request.setTransferTimeout(15000);
    request.setHeader(QNetworkRequest::UserAgentHeader, "omasports/0.1");

    QNetworkReply *reply = m_network.get(request);
    m_pending = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, sport] {
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
        const QVariantList groups = parseStandings(payload, sport, &parseError);
        if (!parseError.isEmpty()) {
            m_status = "error";
            m_error = parseError;
            emit statusChanged();
            return;
        }

        m_standings = groups;
        m_standingsHeaders = standingsHeadersFor(sport);
        m_updatedAt = QDateTime::currentDateTime();
        m_status = "ready";
        m_error.clear();
        emit eventsChanged();
        emit standingsChanged();
        emit statusChanged();
    });
}

QVariantMap Backend::parseMatchSummary(const QByteArray &payload, const QVariantMap &event,
                                                QString *error)
{
    if (error)
        error->clear();

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error)
            *error = parseError.errorString().isEmpty()
                       ? QStringLiteral("Unexpected response from the match summary")
                       : parseError.errorString();
        return {};
    }

    const QJsonObject root = document.object();

    QHash<QString, QString> abbreviations;
    for (const auto &teamValue : event.value("teams").toList()) {
        const QVariantMap team = teamValue.toMap();
        const QString abbreviation = team.value("abbreviation").toString();
        if (!team.value("id").toString().isEmpty())
            abbreviations.insert(team.value("id").toString(), abbreviation);
        if (!team.value("name").toString().isEmpty())
            abbreviations.insert(team.value("name").toString(), abbreviation);
    }

    QVariantList goals;
    QVariantList cards;
    const QString eventSport = event.value("sport").toString();
    const bool playDetails = eventSport.isEmpty() || eventSport == "soccer"
                             || eventSport == "hockey";
    for (const auto &keyEventValue : root.value("keyEvents").toArray()) {
        if (!playDetails)
            break;
        const QJsonObject keyEvent = keyEventValue.toObject();
        const QJsonObject type = keyEvent.value("type").toObject();
        const QString typeText = type.value("text").toString();
        const QString text = keyEvent.value("text").toString();
        const QString clock = keyEvent.value("clock").toObject()
                                  .value("displayValue").toString();
        const QJsonObject team = keyEvent.value("team").toObject();
        const QString teamName = team.value("displayName").toString();
        const QString teamId = team.value("id").toString();
        const QString abbreviation = abbreviations.value(teamId,
                                                          abbreviations.value(teamName));

        const QJsonArray participants = keyEvent.value("participants").toArray();
        auto playerName = [&participants](int index) {
            if (index >= participants.size())
                return QString();
            return participants.at(index).toObject().value("athlete").toObject()
                       .value("displayName").toString();
        };

        const QString player = playerName(0);
        const bool goal = keyEvent.value("scoringPlay").toBool()
                          || typeText.contains("Goal", Qt::CaseInsensitive);
        const bool card = typeText.contains("Card", Qt::CaseInsensitive);

        if (goal) {
            QVariantMap item;
            item.insert("clock", clock);
            item.insert("type", typeText);
            item.insert("scorer", player);
            item.insert("assist", playerName(1));
            item.insert("teamName", teamName);
            item.insert("teamAbbreviation", abbreviation);
            item.insert("ownGoal", typeText.contains("Own Goal", Qt::CaseInsensitive)
                                      || text.contains("own goal", Qt::CaseInsensitive));
            item.insert("penalty", typeText.contains("Penalty", Qt::CaseInsensitive));
            item.insert("text", text);
            goals.append(item);
        } else if (card) {
            QVariantMap item;
            item.insert("clock", clock);
            item.insert("type", typeText);
            item.insert("player", player);
            item.insert("teamName", teamName);
            item.insert("teamAbbreviation", abbreviation);
            item.insert("red", typeText.contains("Red", Qt::CaseInsensitive));
            cards.append(item);
        }
    }

    QVariantMap result;
    result.insert("goals", goals);
    result.insert("cards", cards);

    const QJsonArray boxTeams = root.value("boxscore").toObject()
                                    .value("teams").toArray();
    QVariantList stats;
    if (!boxTeams.isEmpty()) {
        QHash<QString, QString> homeStats;
        QHash<QString, QString> awayStats;
        for (const auto &teamValue : boxTeams) {
            const QJsonObject team = teamValue.toObject();
            QHash<QString, QString> &target = team.value("homeAway").toString() == "home"
                                                  ? homeStats : awayStats;
            for (const auto &statValue : team.value("statistics").toArray()) {
                const QJsonObject stat = statValue.toObject();
                const QString name = stat.value("name").toString();
                if (!name.isEmpty())
                    target.insert(name, stat.value("displayValue").toString());
            }
        }

        struct StatKey {
            const char *name;
            const char *label;
            bool percent;
        };
        static const StatKey keys[] = {
            {"possessionPct", "Possession", true},
            {"totalShots", "Shots", false},
            {"shotsOnTarget", "Shots on target", false},
            {"wonCorners", "Corners", false},
            {"foulsCommitted", "Fouls", false},
            {"offsides", "Offsides", false},
            {"yellowCards", "Yellow cards", false},
            {"redCards", "Red cards", false},
            {"saves", "Saves", false},
        };

        for (const auto &key : keys) {
            const QString name = QString::fromLatin1(key.name);
            if (!homeStats.contains(name) && !awayStats.contains(name))
                continue;
            QString homeValue = homeStats.value(name);
            QString awayValue = awayStats.value(name);
            if (key.percent) {
                if (!homeValue.isEmpty())
                    homeValue += QStringLiteral("%");
                if (!awayValue.isEmpty())
                    awayValue += QStringLiteral("%");
            }
            QVariantMap row;
            row.insert("label", QString::fromLatin1(key.label));
            row.insert("home", homeValue);
            row.insert("away", awayValue);
            stats.append(row);
        }
    }
    result.insert("stats", stats);

    return result;
}

void Backend::retranslate()
{
    m_leagues = leagueCatalog();
    m_standingsHeaders = standingsHeadersFor(this->league().value("sport").toString());
    emit leaguesChanged();
    emit leagueChanged();
    emit dateChanged();
    emit eventsChanged();
    emit standingsChanged();
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

void Backend::saveLeagues() const
{
    QDir().mkpath(QFileInfo(m_settingsPath).absolutePath());
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    settings.setValue("leagues/enabled", enabledLeagues());
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

bool Backend::leagueEnabled(const QString &sport, const QString &league) const
{
    return m_enabledLeagues.contains(leagueKey(sport, league));
}

int Backend::firstEnabledLeague() const
{
    for (int i = 0; i < m_leagues.size(); ++i) {
        const QVariantMap entry = m_leagues.at(i).toMap();
        if (leagueEnabled(entry.value("sport").toString(), entry.value("league").toString()))
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

QVariantList Backend::parseStandings(const QByteArray &payload, const QString &sport,
                                          QString *error)
{
    if (error)
        error->clear();

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error)
            *error = parseError.errorString().isEmpty()
                       ? QStringLiteral("Unexpected response from the standings")
                       : parseError.errorString();
        return {};
    }

    const QJsonObject root = document.object();
    QJsonArray groups = root.value("children").toArray();
    if (groups.isEmpty()) {
        if (!root.value("standings").isObject())
            return {};
        groups.append(root);
    }

    const bool singleGroup = groups.size() == 1;
    QVariantList result;
    for (const auto &groupValue : groups) {
        const QJsonObject group = groupValue.toObject();
        const QJsonArray entries = group.value("standings").toObject()
                                       .value("entries").toArray();
        if (entries.isEmpty())
            continue;

        QVariantList rows;
        int index = 0;
        for (const auto &entryValue : entries) {
            ++index;
            const QJsonObject entry = entryValue.toObject();
            const QJsonObject team = entry.value("team").toObject();

            QHash<QString, QJsonObject> stats;
            for (const auto &statValue : entry.value("stats").toArray()) {
                const QJsonObject stat = statValue.toObject();
                const QString name = stat.value("name").toString();
                if (!name.isEmpty())
                    stats.insert(name, stat);
            }

            auto display = [&stats](std::initializer_list<const char *> names) {
                for (const char *name : names) {
                    const auto it = stats.constFind(QString::fromLatin1(name));
                    if (it != stats.constEnd())
                        return it.value().value("displayValue").toString();
                }
                return QString();
            };
            auto numeric = [&stats](const char *name) {
                const auto it = stats.constFind(QString::fromLatin1(name));
                return it == stats.constEnd() ? 0.0 : it.value().value("value").toDouble();
            };

            int rank = int(numeric("rank"));
            if (rank == 0)
                rank = int(numeric("playoffSeed"));
            if (rank == 0)
                rank = index;

            QStringList cells;
            if (sport == "soccer") {
                cells << display({"gamesPlayed"}) << display({"wins"}) << display({"ties"})
                      << display({"losses"}) << display({"pointDifferential"})
                      << display({"points"});
            } else if (sport == "rugby") {
                cells << display({"gamesPlayed"}) << display({"wins", "gamesWon"})
                      << display({"ties", "gamesDrawn"}) << display({"losses", "gamesLost"})
                      << display({"points"});
            } else if (sport == "football") {
                cells << display({"wins"}) << display({"losses"}) << display({"ties"})
                      << display({"winPercent"});
            } else if (sport == "basketball" || sport == "baseball") {
                cells << display({"wins"}) << display({"losses"}) << display({"winPercent"})
                      << display({"gamesBehind"});
            } else if (sport == "hockey") {
                cells << display({"wins"}) << display({"losses"}) << display({"otLosses"})
                      << display({"points"});
            } else {
                return {};
            }

            QVariantMap teamMap;
            teamMap.insert("name", team.value("displayName").toString());
            teamMap.insert("abbreviation", team.value("abbreviation").toString());
            teamMap.insert("logo", teamLogo(team));
            teamMap.insert("color", team.value("color").toString());

            QVariantMap row;
            row.insert("rank", rank);
            row.insert("team", teamMap);
            row.insert("cells", cells);
            rows.append(row);
        }

        if (rows.isEmpty())
            continue;

        std::sort(rows.begin(), rows.end(), [](const QVariant &left, const QVariant &right) {
            return left.toMap().value("rank").toInt()
                 < right.toMap().value("rank").toInt();
        });

        QVariantMap out;
        out.insert("name", singleGroup ? QString() : group.value("name").toString());
        out.insert("rows", rows);
        result.append(out);
    }
    return result;
}

QStringList Backend::standingsHeadersFor(const QString &sport)
{
    if (sport == "soccer")
        return {tr("MP"), tr("W"), tr("D"), tr("L"), tr("GD"), tr("Pts")};
    if (sport == "rugby")
        return {tr("MP"), tr("W"), tr("D"), tr("L"), tr("Pts")};
    if (sport == "football")
        return {tr("W"), tr("L"), tr("T"), tr("Pct")};
    if (sport == "basketball" || sport == "baseball")
        return {tr("W"), tr("L"), tr("Pct"), tr("GB")};
    if (sport == "hockey")
        return {tr("W"), tr("L"), tr("OTL"), tr("Pts")};
    return {};
}

QVariantList Backend::liveEventsFrom(const QVariantList &scoreboardEvents, const QString &sport,
                                     const QString &leagueSlug, const QString &sportName,
                                     const QString &leagueName)
{
    QVariantList live;
    for (const auto &event : scoreboardEvents) {
        QVariantMap item = event.toMap();
        if (item.value("state").toString() != "in")
            continue;
        item.insert("sport", sport);
        item.insert("league", leagueSlug);
        item.insert("leagueName", leagueName);
        item.insert("sectionLabel", sportName.isEmpty()
                                       ? leagueName
                                       : sportName + " · " + leagueName);
        live.append(item);
    }
    return live;
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
            item.insert("id", team.value("id").toString());
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

        QHash<QString, QString> abbreviations;
        for (const auto &teamValue : teams) {
            const QVariantMap team = teamValue.toMap();
            abbreviations.insert(team.value("id").toString(),
                                 team.value("abbreviation").toString());
        }

        QVariantList goals;
        QVariantList cards;
        for (const auto &detailValue : competition.value("details").toArray()) {
            const QJsonObject detail = detailValue.toObject();
            const QString clock = detail.value("clock").toObject()
                                      .value("displayValue").toString();
            const QString teamId = detail.value("team").toObject().value("id").toString();
            const QJsonArray athletes = detail.value("athletesInvolved").toArray();
            const QString athlete = athletes.isEmpty()
                                        ? QString()
                                        : athletes.first().toObject()
                                              .value("displayName").toString();

            const QString typeText = detail.value("type").toObject()
                                         .value("text").toString();
            if (detail.value("scoringPlay").toBool()
                && (typeText.contains("Goal", Qt::CaseInsensitive)
                    || typeText.contains("Penalty", Qt::CaseInsensitive))) {
                QVariantMap goal;
                goal.insert("clock", clock);
                goal.insert("type", typeText);
                goal.insert("scorer", athlete);
                goal.insert("teamId", teamId);
                goal.insert("teamAbbreviation", abbreviations.value(teamId));
                goal.insert("ownGoal", detail.value("ownGoal").toBool());
                goal.insert("penalty", detail.value("penaltyKick").toBool());
                goals.append(goal);
            }

            if (detail.value("yellowCard").toBool() || detail.value("redCard").toBool()) {
                QVariantMap card;
                card.insert("clock", clock);
                card.insert("type", typeText);
                card.insert("player", athlete);
                card.insert("teamId", teamId);
                card.insert("teamAbbreviation", abbreviations.value(teamId));
                card.insert("red", detail.value("redCard").toBool());
                cards.append(card);
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
        item.insert("goals", goals);
        item.insert("cards", cards);
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
