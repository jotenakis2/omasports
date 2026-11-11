#include "backend.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QMap>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>
#include <QTime>
#include <QUrlQuery>

#include <algorithm>
#include <functional>

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
    {"rugby", "Rugby", "270559", "Top 14"},
    {"rugby", "Rugby", "180659", "Six Nations"},
    {"rugby", "Rugby", "164205", "Rugby World Cup"},
    {"cricket", "Cricket", "8039", "Cricket World Cup"},
    {"volleyball", "Volleyball", "womens-college-volleyball", "College Volleyball"},
    {"australian-football", "Australian Football", "afl", "AFL"},
};

// League marks come from ESPN's scoreboard payloads (`leagues[0].logos`).
// Some competitions only have a generic sport icon, which is still better
// than an empty row; the dark variant falls back to the light one below.
struct LeagueLogo {
    const char *sport;
    const char *league;
    const char *light;
    const char *dark;
};

const LeagueLogo leagueLogos[] = {
    {"soccer", "eng.1", "https://a.espncdn.com/i/leaguelogos/soccer/500/23.png", "https://a.espncdn.com/i/leaguelogos/soccer/500-dark/23.png"},
    {"soccer", "esp.1", "https://a.espncdn.com/i/leaguelogos/soccer/500/15.png", "https://a.espncdn.com/i/leaguelogos/soccer/500-dark/15.png"},
    {"soccer", "ita.1", "https://a.espncdn.com/i/leaguelogos/soccer/500/12.png", "https://a.espncdn.com/i/leaguelogos/soccer/500-dark/12.png"},
    {"soccer", "ger.1", "https://a.espncdn.com/i/leaguelogos/soccer/500/10.png", "https://a.espncdn.com/i/leaguelogos/soccer/500-dark/10.png"},
    {"soccer", "fra.1", "https://a.espncdn.com/i/leaguelogos/soccer/500/9.png", "https://a.espncdn.com/i/leaguelogos/soccer/500-dark/9.png"},
    {"soccer", "uefa.champions", "https://a.espncdn.com/i/leaguelogos/soccer/500/2.png", "https://a.espncdn.com/i/leaguelogos/soccer/500-dark/2.png"},
    {"soccer", "uefa.europa", "https://a.espncdn.com/i/leaguelogos/soccer/500/2310.png", "https://a.espncdn.com/i/leaguelogos/soccer/500-dark/2310.png"},
    {"soccer", "usa.1", "https://a.espncdn.com/i/leaguelogos/soccer/500/19.png", "https://a.espncdn.com/i/leaguelogos/soccer/500-dark/19.png"},
    {"soccer", "mex.1", "https://a.espncdn.com/i/leaguelogos/soccer/500/22.png", "https://a.espncdn.com/i/leaguelogos/soccer/500-dark/22.png"},
    {"soccer", "bra.1", "https://a.espncdn.com/i/leaguelogos/soccer/500/85.png", "https://a.espncdn.com/i/leaguelogos/soccer/500-dark/85.png"},
    {"soccer", "arg.1", "https://a.espncdn.com/i/leaguelogos/soccer/500/1.png", "https://a.espncdn.com/i/leaguelogos/soccer/500-dark/1.png"},
    {"soccer", "ned.1", "https://a.espncdn.com/i/leaguelogos/soccer/500/11.png", "https://a.espncdn.com/i/leaguelogos/soccer/500-dark/11.png"},
    {"soccer", "por.1", "https://a.espncdn.com/i/leaguelogos/soccer/500/14.png", "https://a.espncdn.com/i/leaguelogos/soccer/500-dark/14.png"},
    {"football", "nfl", "https://a.espncdn.com/i/teamlogos/leagues/500/nfl.png", "https://a.espncdn.com/i/teamlogos/leagues/500-dark/nfl.png"},
    {"football", "college-football", "https://a.espncdn.com/redesign/assets/img/icons/NCAASports_Football.png", ""},
    {"basketball", "nba", "https://a.espncdn.com/i/teamlogos/leagues/500/nba.png", "https://a.espncdn.com/combiner/i?img=/i/teamlogos/leagues/500-dark/nba.png&w=500&h=500&transparent=true"},
    {"basketball", "wnba", "https://a.espncdn.com/i/teamlogos/leagues/500/wnba.png", "https://a.espncdn.com/i/teamlogos/leagues/500-dark/wnba.png"},
    {"basketball", "mens-college-basketball", "https://a.espncdn.com/redesign/assets/img/icons/NCAASports_MensBasketball.png", ""},
    {"baseball", "mlb", "https://a.espncdn.com/i/teamlogos/leagues/500/mlb.png", "https://a.espncdn.com/combiner/i?img=/i/teamlogos/leagues/500-dark/mlb.png&w=500&h=500&transparent=true"},
    {"baseball", "college-baseball", "https://a.espncdn.com/combiner/i?img=/redesign/assets/img/icons/ESPN-icon-baseball.png", ""},
    {"hockey", "nhl", "https://a.espncdn.com/i/teamlogos/leagues/500/nhl.png", "https://a.espncdn.com/i/teamlogos/leagues/500-dark/nhl.png"},
    {"tennis", "atp", "https://a.espncdn.com/combiner/i?img=/redesign/assets/img/icons/ESPN-icon-tennis.png", ""},
    {"tennis", "wta", "https://a.espncdn.com/combiner/i?img=/redesign/assets/img/icons/ESPN-icon-tennis.png", ""},
    {"golf", "pga", "https://a.espncdn.com/combiner/i?img=/i/teamlogos/leagues/500/pgatour.png", "https://a.espncdn.com/combiner/i?img=/i/teamlogos/leagues/500-dark/pgatour.png"},
    {"golf", "lpga", "https://a.espncdn.com/combiner/i?img=/i/teamlogos/leagues/500/lpga.png", "https://a.espncdn.com/combiner/i?img=/i/teamlogos/leagues/500-dark/lpga.png"},
    {"mma", "ufc", "https://a.espncdn.com/i/teamlogos/leagues/500/ufc.png", ""},
    {"racing", "f1", "https://a.espncdn.com/combiner/i?img=/i/teamlogos/leagues/500/f1.png", ""},
    {"racing", "nascar-premier", "https://a.espncdn.com/combiner/i?img=/redesign/assets/img/icons/ESPN-icon-NASCAR.png", ""},
    {"rugby", "270559", "https://a.espncdn.com/combiner/i?img=/redesign/assets/img/icons/ESPN-icon-rugby.png", ""},
    {"rugby", "180659", "https://a.espncdn.com/combiner/i?img=/redesign/assets/img/icons/ESPN-icon-rugby.png", "https://a.espncdn.com/guid/3c68c9b2-7e7c-3a78-9f96-cb954d22809d/logos/default-dark.png"},
    {"rugby", "164205", "https://a.espncdn.com/combiner/i?img=/redesign/assets/img/icons/ESPN-icon-rugby.png", ""},
    {"cricket", "8039", "https://a.espncdn.com/combiner/i?img=/redesign/assets/img/icons/ESPN-icon-cricket.png", ""},
    {"volleyball", "womens-college-volleyball", "https://a.espncdn.com/combiner/i?img=/redesign/assets/img/icons/sports-volleyball-solid.png", ""},
    {"australian-football", "afl", "https://a.espncdn.com/i/teamlogos/leagues/500/afl.png", ""},
};

QString leagueLogoUrl(const QString &sport, const QString &league, bool dark)
{
    for (const auto &entry : leagueLogos) {
        if (sport != QLatin1String(entry.sport) || league != QLatin1String(entry.league))
            continue;
        if (dark && entry.dark && *entry.dark)
            return QString::fromLatin1(entry.dark);
        return QString::fromLatin1(entry.light);
    }
    return {};
}

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

QStringList allLeagueKeys()
{
    QStringList keys;
    for (const auto &entry : catalog)
        keys.append(leagueKey(sportSlug(entry), QString::fromLatin1(entry.league)));
    return keys;
}

bool allowsCountryLogos(const QString &sport, const QString &league)
{
    if (sport == QLatin1String("rugby")
        && (league == QLatin1String("180659") || league == QLatin1String("164205"))) {
        return true;
    }
    if (sport == QLatin1String("cricket") && league == QLatin1String("8039"))
        return true;
    return false;
}

QString teamLogo(const QJsonObject &team, bool allowCountryLogos)
{
    auto pick = [&allowCountryLogos](const QString &logo) -> QString {
        if (logo.isEmpty())
            return {};
        if (!allowCountryLogos && logo.contains(QStringLiteral("/teamlogos/countries/")))
            return {};
        return logo;
    };

    const QString logo = pick(team.value("logo").toString());
    if (!logo.isEmpty())
        return logo;
    const auto logos = team.value("logos").toArray();
    for (const auto &entry : logos) {
        const QString candidate = pick(entry.toObject().value("href").toString());
        if (!candidate.isEmpty())
            return candidate;
    }
    return {};
}

int uniformMatchesPlayed(const QVariantList &groups)
{
    int totalTeams = 0;
    int uniform = -1;
    for (const auto &groupValue : groups) {
        const QVariantList rows = groupValue.toMap().value("rows").toList();
        for (const auto &rowValue : rows) {
            ++totalTeams;
            const QVariantMap row = rowValue.toMap();
            bool ok = false;
            int played = row.value("gamesPlayed").toInt(&ok);
            if (!ok || !row.contains("gamesPlayed")) {
                const QStringList cells = row.value("cells").toStringList();
                if (cells.isEmpty())
                    return -1;
                played = cells.first().toInt(&ok);
            }
            if (!ok)
                return -1;
            if (uniform == -1)
                uniform = played;
            else if (played != uniform)
                return -1;
        }
    }
    return totalTeams == 0 ? -1 : uniform;
}

bool isCompleteSeasonTable(const QVariantList &groups)
{
    const int played = uniformMatchesPlayed(groups);
    if (played <= 0)
        return false;
    int teamCount = 0;
    for (const auto &groupValue : groups)
        teamCount += groupValue.toMap().value("rows").toList().size();
    if (teamCount <= 1)
        return false;
    // Double ou simple tournoi à la ronde : toutes les équipes ont disputé le
    // même nombre de matchs, égal au volume d'un championnat complet.
    return played == 2 * (teamCount - 1) || played == (teamCount - 1);
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

void orderHomeFirst(QVariantList *teams)
{
    if (teams->size() != 2)
        return;
    const QVariantMap first = teams->first().toMap();
    const QVariantMap second = teams->last().toMap();
    const QString firstSide = first.value("homeAway").toString();
    const QString secondSide = second.value("homeAway").toString();
    const bool firstHome = firstSide == QLatin1String("home");
    const bool firstAway = firstSide == QLatin1String("away");
    const bool secondHome = secondSide == QLatin1String("home");
    const bool secondAway = secondSide == QLatin1String("away");
    if (!(firstHome || firstAway) || !(secondHome || secondAway))
        return; // information de camp absente : ordre du flux conservé
    if (firstAway && secondHome) {
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

    // A league added since the saved list was written starts enabled, while
    // leagues the user had already turned off stay off.
    const QStringList knownLeagues = settings.value("leagues/known").toStringList();
    if (!knownLeagues.isEmpty()) {
        for (const auto &key : allLeagueKeys())
            if (!knownLeagues.contains(key))
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

    // Font size levels: 0 Petit, 1 Compact, 2 Normal, 3 Grand, 4 Très Grand.
    // Older releases stored 0 Compact, 1 Normal, 2 Large; shift those once so
    // an existing choice keeps its apparent size after the upgrade.
    int level = settings.value("ui/fontScale", 2).toInt();
    if (settings.contains("ui/fontScale") && !settings.contains("ui/fontScaleVersion"))
        level += 1;
    m_fontScaleLevel = qBound(0, level, 4);

    // League marks show in both places by default; Settings can turn either
    // one off independently.
    m_sidebarLogos = settings.value("ui/sidebarLogos", true).toBool();
    m_headerLogos = settings.value("ui/headerLogos", true).toBool();

    // Persist the normalized list, including an empty one, so "none" survives
    // the next launch instead of falling back to every league.
    settings.setValue("leagues/enabled", enabledLeagues());
    settings.setValue("leagues/known", allLeagueKeys());
    settings.setValue("ui/fontScale", m_fontScaleLevel);
    settings.setValue("ui/fontScaleVersion", 2);
    settings.setValue("ui/sidebarLogos", m_sidebarLogos);
    settings.setValue("ui/headerLogos", m_headerLogos);
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

qreal Backend::fontScale() const
{
    switch (m_fontScaleLevel) {
    case 0:
        return 0.8; // Petit
    case 1:
        return 0.9; // Compact
    case 3:
        return 1.2; // Grand
    case 4:
        return 1.4; // Très Grand
    default:
        return 1.0; // Normal
    }
}

void Backend::setFontScale(int level)
{
    level = qBound(0, level, 4);
    if (level == m_fontScaleLevel)
        return;
    m_fontScaleLevel = level;
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    settings.setValue("ui/fontScale", m_fontScaleLevel);
    settings.setValue("ui/fontScaleVersion", 2);
    emit fontScaleChanged();
}

void Backend::setSidebarLogos(bool visible)
{
    if (visible == m_sidebarLogos)
        return;
    m_sidebarLogos = visible;
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    settings.setValue("ui/sidebarLogos", m_sidebarLogos);
    emit logoOptionsChanged();
}

void Backend::setHeaderLogos(bool visible)
{
    if (visible == m_headerLogos)
        return;
    m_headerLogos = visible;
    QSettings settings(m_settingsPath, QSettings::IniFormat);
    settings.setValue("ui/headerLogos", m_headerLogos);
    emit logoOptionsChanged();
}

void Backend::selectLeague(int index)
{
    if (index < 0 || index >= m_leagues.size())
        return;
    const QVariantMap entry = m_leagues.at(index).toMap();
    if (!leagueEnabled(entry.value("sport").toString(), entry.value("league").toString()))
        return;

    const bool leavingLive = m_liveMode;
    const bool keepUpcoming = m_upcomingVisible;
    if (index == m_leagueIndex && !leavingLive)
        return;

    m_liveMode = false;
    m_leagueIndex = index;
    m_events.clear();
    m_standings.clear();
    m_standingsSeasonUncertain = false;
    m_liveCount = 0;
    m_updatedAt = QDateTime();
    m_status = m_online ? "loading" : "idle";
    m_error.clear();
    saveSelection();
    if (leavingLive)
        emit liveModeChanged();
    emit standingsChanged();
    if (keepUpcoming) {
        m_standingsHeaders = standingsHeadersFor(this->league().value("sport").toString());
        emit leagueChanged();
        fetchUpcoming(false);
        return;
    }
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
    if (m_upcomingVisible) {
        m_upcomingVisible = false;
        emit upcomingChanged();
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
    if (m_upcomingVisible) {
        m_upcomingVisible = false;
        emit upcomingChanged();
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
    const bool wasUpcoming = m_upcomingVisible;
    if (wasLive) {
        m_liveMode = false;
        emit liveModeChanged();
    }
    if (wasStandings) {
        m_standingsVisible = false;
        emit standingsChanged();
    }
    if (wasUpcoming) {
        m_upcomingVisible = false;
        emit upcomingChanged();
    }
    if (m_dayOffset == 0 && !wasLive && !wasStandings && !wasUpcoming) {
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
    if (live && m_upcomingVisible) {
        m_upcomingVisible = false;
        emit upcomingChanged();
    }

    // Live scores are today's games, so the day picker shows today even when
    // the user had browsed another day before opening the live view.
    if (live)
        m_dayOffset = 0;

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
    if (visible && m_upcomingVisible) {
        m_upcomingVisible = false;
        emit upcomingChanged();
    }

    m_standingsVisible = visible;
    m_standings.clear();
    m_standingsSeasonUncertain = false;
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

void Backend::toggleUpcoming()
{
    setUpcoming(!m_upcomingVisible);
}

void Backend::setUpcoming(bool visible)
{
    if (visible == m_upcomingVisible) {
        if (visible)
            refreshUpcoming();
        return;
    }

    if (visible && m_liveMode) {
        m_liveMode = false;
        emit liveModeChanged();
    }
    if (visible && m_standingsVisible) {
        m_standingsVisible = false;
        emit standingsChanged();
    }

    m_upcomingVisible = visible;
    m_upcomingError.clear();
    if (visible) {
        m_upcoming.clear();
        m_upcomingLoading = true;
        emit upcomingChanged();
        fetchUpcoming(false);
    } else {
        m_upcomingLoading = false;
        emit upcomingChanged();
    }
}

void Backend::invalidateUpcomingCache()
{
    m_upcomingCache.clear();
}

void Backend::injectUpcomingMonthPayload(const QString &sport, const QString &league,
                                         const QString &month, const QByteArray &payload)
{
    UpcomingCacheEntry entry;
    entry.payload = payload;
    m_upcomingCache.insert(leagueKey(sport, league) + "/" + month, entry);
}

void Backend::refreshUpcoming()
{
    if (!m_upcomingVisible)
        return;
    fetchUpcoming(true);
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

    // Golf, tennis, MMA, and racing have no match summary endpoint; show the
    // scoreboard data instead of an avoidable 404.
    if (!matchSummarySupported(sport)) {
        m_match.insert("detailsUnavailable", true);
        m_matchStatus = "ready";
        m_matchError.clear();
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
    request.setHeader(QNetworkRequest::UserAgentHeader, "omasports/1.0");

    QNetworkReply *reply = m_network.get(request);
    m_matchPending = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (reply != m_matchPending)
            return;
        m_matchPending.clear();

        const QByteArray payload = reply->readAll();
        const QString networkError = reply->error() == QNetworkReply::NoError
                                       ? QString() : reply->errorString();
        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        reply->deleteLater();

        if (!networkError.isEmpty()) {
            // Some leagues or events simply have no summary; that is not a
            // loading failure worth an error screen.
            if (httpStatus == 400 || httpStatus == 404) {
                m_match.insert("detailsUnavailable", true);
                m_matchStatus = "ready";
                m_matchError.clear();
            } else {
                m_matchStatus = "error";
                m_matchError = networkError;
            }
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
    if (m_upcomingVisible) {
        refreshUpcoming();
        return;
    }
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
    request.setHeader(QNetworkRequest::UserAgentHeader, "omasports/1.0");

    QNetworkReply *reply = m_network.get(request);
    m_pending = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, league] {
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
        const QVariantList events = parseScoreboard(payload, &parseError,
                                                    league.value("sport").toString(),
                                                    league.value("league").toString());
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
        request.setHeader(QNetworkRequest::UserAgentHeader, "omasports/1.0");

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
                const QVariantList events = parseScoreboard(payload, &parseError, task.sport, task.league);
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
    const QSet<QNetworkReply *> upcomingReplies = m_upcomingReplies;
    m_upcomingReplies.clear();
    for (QNetworkReply *reply : upcomingReplies) {
        reply->abort();
        reply->deleteLater();
    }
    m_upcomingPending = 0;
}

void Backend::fetchStandings()
{
    const QVariantMap league = this->league();
    if (league.isEmpty())
        return;

    const QString sport = league.value("sport").toString();
    const QString leagueSlug = league.value("league").toString();
    m_standingsHeaders = standingsHeadersFor(sport);
    if (m_standingsHeaders.isEmpty()) {
        m_standings.clear();
        m_standingsSeasonUncertain = false;
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

    auto runCandidates = [this, sport, leagueSlug](int scoreboardYear, bool seasonInProgress) {
        const QList<int> candidates = standingsSeasonCandidates(sport, date(), scoreboardYear);
        if (candidates.isEmpty()) {
            m_status = "error";
            m_error = tr("No standings season candidates");
            emit statusChanged();
            return;
        }

        struct State {
            QList<int> candidates;
            int index = 0;
            QMap<int, QByteArray> payloads;
        };
        auto state = std::make_shared<State>();
        state->candidates = candidates;

        auto tryNext = std::make_shared<std::function<void()>>();
        *tryNext = [this, sport, leagueSlug, seasonInProgress, state, tryNext]() {
            if (state->index >= state->candidates.size()) {
                const QVariantMap chosen = chooseStandingsSeason(
                    sport, state->candidates, state->payloads, seasonInProgress, leagueSlug);
                m_standings = chosen.value("groups").toList();
                m_standingsHeaders = standingsHeadersFor(sport);
                m_standingsSeasonUncertain = chosen.value("uncertain", true).toBool();
                m_updatedAt = QDateTime::currentDateTime();
                m_status = "ready";
                m_error.clear();
                emit eventsChanged();
                emit standingsChanged();
                emit statusChanged();
                return;
            }

            const int season = state->candidates.at(state->index++);
            const QUrl url = standingsUrlForSeason(sport, leagueSlug, season);
            QNetworkRequest request(url);
            request.setTransferTimeout(15000);
            request.setHeader(QNetworkRequest::UserAgentHeader, "omasports/1.0");

            QNetworkReply *reply = m_network.get(request);
            m_pending = reply;
            connect(reply, &QNetworkReply::finished, this, [this, sport, leagueSlug, seasonInProgress, state, tryNext, season, reply] {
                if (reply != m_pending)
                    return;
                m_pending.clear();
                const QByteArray payload = reply->readAll();
                const QString networkError = reply->error() == QNetworkReply::NoError
                                               ? QString() : reply->errorString();
                reply->deleteLater();

                if (networkError.isEmpty())
                    state->payloads.insert(season, payload);

                const QVariantMap chosen = chooseStandingsSeason(
                    sport, state->candidates, state->payloads, seasonInProgress, leagueSlug);
                if (!chosen.value("uncertain", true).toBool()) {
                    m_standings = chosen.value("groups").toList();
                    m_standingsHeaders = standingsHeadersFor(sport);
                    m_standingsSeasonUncertain = false;
                    m_updatedAt = QDateTime::currentDateTime();
                    m_status = "ready";
                    m_error.clear();
                    emit eventsChanged();
                    emit standingsChanged();
                    emit statusChanged();
                    return;
                }

                (*tryNext)();
            });
        };

        (*tryNext)();
    };

    // Récupère l'année de saison indiquée par le scoreboard pour l'ajouter aux
    // candidates, sans jamais bloquer le chargement du classement en cas d'échec.
    const QUrl sbUrl = scoreboardUrl(sport, leagueSlug, date());
    QNetworkRequest sbRequest(sbUrl);
    sbRequest.setTransferTimeout(15000);
    sbRequest.setHeader(QNetworkRequest::UserAgentHeader, "omasports/1.0");
    QNetworkReply *sbReply = m_network.get(sbRequest);
    m_pending = sbReply;
    connect(sbReply, &QNetworkReply::finished, this, [this, sbReply, runCandidates] {
        if (sbReply != m_pending)
            return;
        m_pending.clear();
        const QByteArray payload = sbReply->readAll();
        const bool ok = sbReply->error() == QNetworkReply::NoError;
        sbReply->deleteLater();
        runCandidates(ok ? scoreboardSeasonYear(payload) : 0,
                      ok ? scoreboardIndicatesSeasonInProgress(payload) : true);
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

void Backend::fetchUpcoming(bool force)
{
    const QVariantMap currentLeague = league();
    const QString sport = currentLeague.value("sport").toString();
    const QString leagueSlug = currentLeague.value("league").toString();
    if (sport.isEmpty() || leagueSlug.isEmpty()) {
        m_upcoming.clear();
        m_upcomingLoading = false;
        m_upcomingError = tr("No leagues selected");
        emit upcomingChanged();
        return;
    }

    // Cancel in-flight upcoming requests.
    const QSet<QNetworkReply *> replies = m_upcomingReplies;
    m_upcomingReplies.clear();
    for (QNetworkReply *reply : replies) {
        reply->abort();
        reply->deleteLater();
    }
    m_upcomingPending = 0;

    const QDate today = QDate::currentDate();
    const QDate currentMonth(today.year(), today.month(), 1);
    const QDate nextMonth = currentMonth.addMonths(1);
    const QString currentKey = leagueKey(sport, leagueSlug) + "/" + currentMonth.toString("yyyyMM");
    const QString nextKey = leagueKey(sport, leagueSlug) + "/" + nextMonth.toString("yyyyMM");

    auto isFresh = [](const UpcomingCacheEntry &entry) {
        return entry.fetchedAt.isValid() && entry.fetchedAt.secsTo(QDateTime::currentDateTime()) < 300;
    };

    bool needCurrent = force || !m_upcomingCache.contains(currentKey) || !isFresh(m_upcomingCache.value(currentKey));
    bool needNext = force || !m_upcomingCache.contains(nextKey) || !isFresh(m_upcomingCache.value(nextKey));

    if (!needCurrent && !needNext) {
        QString parseError;
        m_upcoming = parseUpcomingSchedule(m_upcomingCache.value(currentKey).payload,
                                           m_upcomingCache.value(nextKey).payload,
                                           today, &parseError, sport, leagueSlug);
        m_upcomingLoading = false;
        m_upcomingError = parseError;
        emit upcomingChanged();
        return;
    }

    m_upcomingLoading = true;
    m_upcomingError.clear();
    emit upcomingChanged();

    struct MonthTask {
        QString key;
        QString month;
        bool needed;
        QUrl url;
    };
    const QList<MonthTask> tasks = {
        {currentKey, currentMonth.toString("yyyyMM"), needCurrent, scoreboardMonthUrl(sport, leagueSlug, currentMonth)},
        {nextKey, nextMonth.toString("yyyyMM"), needNext, scoreboardMonthUrl(sport, leagueSlug, nextMonth)},
    };

    m_upcomingHadNetworkError = false;
    m_upcomingNetworkError.clear();
    m_upcomingPending = 0;

    auto finalize = [this, sport, leagueSlug, today, currentKey, nextKey]() {
        QString parseError;
        m_upcoming = parseUpcomingSchedule(m_upcomingCache.value(currentKey).payload,
                                           m_upcomingCache.value(nextKey).payload,
                                           today, &parseError, sport, leagueSlug);
        m_upcomingLoading = false;
        if (m_upcomingHadNetworkError && m_upcoming.isEmpty())
            m_upcomingError = m_upcomingNetworkError.isEmpty() ? tr("Could not load schedule") : m_upcomingNetworkError;
        else
            m_upcomingError = parseError;
        emit upcomingChanged();
    };

    for (const auto &task : tasks) {
        if (!task.needed)
            continue;

        ++m_upcomingNetworkRequests;
        if (m_online) {
            ++m_upcomingPending;
            QNetworkRequest request(task.url);
            request.setTransferTimeout(15000);
            request.setHeader(QNetworkRequest::UserAgentHeader, "omasports/1.0");
            QNetworkReply *reply = m_network.get(request);
            m_upcomingReplies.insert(reply);
            connect(reply, &QNetworkReply::finished, this, [this, reply, key = task.key, finalize] {
                if (!m_upcomingReplies.contains(reply))
                    return;
                m_upcomingReplies.remove(reply);
                const QByteArray payload = reply->readAll();
                const QString networkError = reply->error() == QNetworkReply::NoError
                                               ? QString() : reply->errorString();
                reply->deleteLater();
                if (networkError.isEmpty()) {
                    UpcomingCacheEntry entry;
                    entry.payload = payload;
                    entry.fetchedAt = QDateTime::currentDateTime();
                    m_upcomingCache.insert(key, entry);
                } else {
                    m_upcomingHadNetworkError = true;
                    if (m_upcomingNetworkError.isEmpty())
                        m_upcomingNetworkError = networkError;
                }
                if (--m_upcomingPending <= 0)
                    finalize();
            });
        } else {
            // Offline/test: rely on injected payloads placed in cache for the month key.
            if (!m_upcomingCache.contains(task.key)) {
                m_upcomingHadNetworkError = true;
                if (m_upcomingNetworkError.isEmpty())
                    m_upcomingNetworkError = tr("Could not load schedule");
            } else {
                UpcomingCacheEntry entry = m_upcomingCache.value(task.key);
                entry.fetchedAt = QDateTime::currentDateTime();
                m_upcomingCache.insert(task.key, entry);
            }
        }
    }

    if (m_upcomingPending == 0)
        finalize();
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
    emit upcomingChanged();
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
    settings.setValue("leagues/known", allLeagueKeys());
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

    // Rugby's "overall" record is the last few results, not a W-D-L tally.
    static const QRegularExpression formPattern("^[WDL]+$");
    if (formPattern.match(record).hasMatch()) {
        QStringList form;
        for (const QChar result : record) {
            if (result == QLatin1Char('W'))
                form.append(tr("W"));
            else if (result == QLatin1Char('D'))
                form.append(tr("D"));
            else
                form.append(tr("L"));
        }
        return tr("Form: %1").arg(form.join(QLatin1Char(' ')));
    }

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
        const QString sport = sportSlug(entry);
        const QString leagueSlug = QString::fromLatin1(entry.league);
        QVariantMap league;
        league.insert("index", index++);
        league.insert("sport", sport);
        league.insert("sportName", tr(entry.sportName));
        league.insert("league", leagueSlug);
        league.insert("name", QString::fromUtf8(entry.name));
        league.insert("logo", leagueLogoUrl(sport, leagueSlug, false));
        league.insert("logoDark", leagueLogoUrl(sport, leagueSlug, true));
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

QUrl Backend::scoreboardMonthUrl(const QString &sport, const QString &league, const QDate &monthDate)
{
    QUrl url(QStringLiteral("https://site.api.espn.com/apis/site/v2/sports/%1/%2/scoreboard")
                 .arg(sport, league));
    QUrlQuery query;
    query.addQueryItem("dates", monthDate.toString("yyyyMM"));
    url.setQuery(query);
    return url;
}

int Backend::standingsSeason(const QString &sport, const QDate &date)
{
    // Saison explicite par famille (ESPN renvoie sinon un classement périmé,
    // ex. Top 14 : classement final de la saison précédente sans ?season=).
    // Rugby : année de FIN de saison (2026-27 => 2027).
    // Football (soccer) : année de DÉBUT de saison (2026-27 => 2026).
    // NBA/NHL (basketball/hockey) : année de FIN de saison.
    // Les autres sports à saisons chevauchantes suivent la fin de saison ;
    // en cas de doute on privilégie l'année courante selon le même seuil.
    const int year = date.year();
    const int month = date.month();
    if (sport == QLatin1String("soccer")) {
        // Saison européenne typique août-mai : à partir de juillet on bascule
        // sur la saison qui commence cette année-là.
        return month >= 7 ? year : year - 1;
    }
    if (sport == QLatin1String("rugby") || sport == QLatin1String("basketball")
        || sport == QLatin1String("hockey") || sport == QLatin1String("football")) {
        // Saisons à cheval sur deux années civiles, identifiées par l'année de fin.
        return month >= 7 ? year + 1 : year;
    }
    return year;
}

int Backend::scoreboardSeasonYear(const QByteArray &payload)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return 0;
    const QJsonArray leagues = document.object().value("leagues").toArray();
    if (leagues.isEmpty())
        return 0;
    return leagues.first().toObject().value("season").toObject().value("year").toInt();
}

bool Backend::scoreboardIndicatesSeasonInProgress(const QByteArray &payload)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return false;
    const QJsonObject root = document.object();
    if (root.value("leagues").toArray().isEmpty())
        return false;
    // Le scoreboard décrit la saison courante : un match non terminé ou à venir
    // indique une saison en cours (et non une saison archivée entièrement jouée).
    for (const auto &eventValue : root.value("events").toArray()) {
        const QJsonObject event = eventValue.toObject();
        QJsonObject status = event.value("status").toObject();
        const QJsonArray competitions = event.value("competitions").toArray();
        if (!competitions.isEmpty())
            status = competitions.first().toObject().value("status").toObject();
        const QString state = status.value("type").toObject().value("state").toString();
        if (state != QLatin1String("post"))
            return true;
    }
    // Sans événement exploitable, la présence d'une saison dans le scoreboard
    // suffit à considérer le contexte comme courant.
    return root.value("leagues").toArray().first().toObject().value("season").isObject();
}

QList<int> Backend::standingsSeasonCandidates(const QString &sport, const QDate &date,
                                              int scoreboardYear)
{
    const int computed = standingsSeason(sport, date);
    QList<int> candidates;
    auto add = [&candidates](int year) {
        if (year > 0 && !candidates.contains(year))
            candidates.append(year);
    };
    // Année calculée par sport, puis l'autre année (± 1) de part et d'autre,
    // puis l'année lue dans le scoreboard quand disponible.
    add(computed);
    add(computed - 1);
    add(computed + 1);
    add(scoreboardYear);
    return candidates;
}

QUrl Backend::standingsUrlForSeason(const QString &sport, const QString &league, int season)
{
    QUrl url(QStringLiteral("https://site.api.espn.com/apis/v2/sports/%1/%2/standings")
                 .arg(sport, league));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("season"), QString::number(season));
    url.setQuery(query);
    return url;
}

QUrl Backend::standingsUrl(const QString &sport, const QString &league, const QDate &date)
{
    return standingsUrlForSeason(sport, league, standingsSeason(sport, date));
}

QVariantMap Backend::chooseStandingsSeason(const QString &sport, const QList<int> &candidates,
                                           const QMap<int, QByteArray> &payloads,
                                           bool scoreboardSeasonInProgress,
                                           const QString &league)
{
    QVariantMap chosen;
    chosen.insert("season", candidates.isEmpty() ? 0 : candidates.first());
    chosen.insert("groups", QVariantList());
    chosen.insert("uncertain", true);

    for (const int season : candidates) {
        if (!payloads.contains(season))
            continue;
        QString error;
        const QVariantList groups = parseStandings(payloads.value(season), sport, &error, league);
        if (!error.isEmpty())
            continue;

        bool teamFound = false;
        for (const auto &groupValue : groups)
            teamFound = teamFound || !groupValue.toMap().value("rows").toList().isEmpty();
        if (!teamFound)
            continue;

        const bool complete = isCompleteSeasonTable(groups);
        if (complete && scoreboardSeasonInProgress)
            continue;

        chosen.insert("season", season);
        chosen.insert("groups", groups);
        chosen.insert("uncertain", false);
        return chosen;
    }

    // Aucune candidate concluante : on retient la première disponible et on
    // signale l'incertitude plutôt que de présenter une table périmée comme sûre.
    for (const int season : candidates) {
        if (!payloads.contains(season))
            continue;
        QString error;
        const QVariantList groups = parseStandings(payloads.value(season), sport, &error, league);
        if (error.isEmpty()) {
            chosen.insert("season", season);
            chosen.insert("groups", groups);
            chosen.insert("uncertain", true);
            return chosen;
        }
    }

    return chosen;
}

QVariantList Backend::parseStandings(const QByteArray &payload, const QString &sport,
                                          QString *error, const QString &league)
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
    const bool allowCountries = allowsCountryLogos(sport, league);
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
                cells << display({"points"}) << display({"gamesPlayed"})
                      << display({"wins"}) << display({"ties"})
                      << display({"losses"}) << display({"pointDifferential"});
            } else if (sport == "rugby") {
                cells << display({"points"}) << display({"gamesPlayed"})
                      << display({"wins", "gamesWon"})
                      << display({"ties", "gamesDrawn"}) << display({"losses", "gamesLost"})
                      << display({"pointsDifference", "pointDifferential"});
            } else if (sport == "football") {
                cells << display({"wins"}) << display({"losses"}) << display({"ties"})
                      << display({"winPercent"});
            } else if (sport == "basketball" || sport == "baseball") {
                cells << display({"wins"}) << display({"losses"}) << display({"winPercent"})
                      << display({"gamesBehind"});
            } else if (sport == "hockey") {
                const QString gamesPlayed = display({"gamesPlayed", "games", "gp"});
                if (!gamesPlayed.isEmpty())
                    cells << display({"points"}) << gamesPlayed;
                else
                    cells << display({"points"});
                cells << display({"wins"}) << display({"losses"}) << display({"otLosses"});
            } else {
                return {};
            }

            QVariantMap teamMap;
            teamMap.insert("name", team.value("displayName").toString());
            teamMap.insert("abbreviation", team.value("abbreviation").toString());
            teamMap.insert("logo", teamLogo(team, allowCountries));
            teamMap.insert("color", team.value("color").toString());

            int gamesPlayed = int(numeric("gamesPlayed"));
            if (gamesPlayed == 0) {
                bool ok = false;
                const int fromDisplay = display({"gamesPlayed"}).toInt(&ok);
                if (ok)
                    gamesPlayed = fromDisplay;
            }

            QVariantMap row;
            row.insert("rank", rank);
            row.insert("team", teamMap);
            row.insert("cells", cells);
            row.insert("gamesPlayed", gamesPlayed);
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
        return {tr("Pts"), tr("MP"), tr("W"), tr("D"), tr("L"), tr("GD")};
    if (sport == "rugby")
        return {tr("Pts"), tr("MP"), tr("W"), tr("D"), tr("L"), tr("GD")};
    if (sport == "football")
        return {tr("W"), tr("L"), tr("T"), tr("Pct")};
    if (sport == "basketball" || sport == "baseball")
        return {tr("W"), tr("L"), tr("Pct"), tr("GB")};
    if (sport == "hockey")
        return {tr("Pts"), tr("W"), tr("L"), tr("OTL")};
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

QVariantList Backend::parseUpcomingSchedule(const QByteArray &currentMonthPayload,
                                             const QByteArray &nextMonthPayload,
                                             const QDate &today, QString *error,
                                             const QString &sport, const QString &league,
                                             int limit)
{
    if (error)
        error->clear();

    QString currentError;
    QString nextError;
    const QVariantList currentEvents = parseScoreboard(currentMonthPayload, &currentError, sport, league);
    const QVariantList nextEvents = parseScoreboard(nextMonthPayload, &nextError, sport, league);

    if (!currentError.isEmpty() && !nextError.isEmpty()) {
        if (error)
            *error = currentError;
        return {};
    }
    if (!currentError.isEmpty() && error)
        *error = currentError;
    if (!nextError.isEmpty() && error)
        *error = nextError;

    QVariantList merged = currentEvents;
    for (const auto &event : nextEvents)
        merged.append(event);

    // Reuse the existing deduplication rules (same teams within 36 h, in > post > pre).
    QVariantList deduped;
    for (const auto &value : merged) {
        const QVariantMap event = value.toMap();
        const QString key = [&event]() {
            const QVariantList teams = event.value("teams").toList();
            if (teams.size() != 2)
                return QString();
            QStringList ids;
            for (const auto &teamValue : teams) {
                const QVariantMap team = teamValue.toMap();
                QString id = team.value("id").toString();
                if (id.isEmpty())
                    id = team.value("abbreviation").toString();
                if (id.isEmpty())
                    id = team.value("name").toString();
                ids.append(id.trimmed().toLower());
            }
            std::sort(ids.begin(), ids.end());
            return ids.join(QLatin1Char('|'));
        }();
        const QDateTime start = QDateTime::fromString(event.value("start").toString(), Qt::ISODate);
        bool mergedDuplicate = false;
        if (!key.isEmpty() && start.isValid()) {
            auto stateRank = [](const QString &state) {
                if (state == QLatin1String("in"))
                    return 2;
                if (state == QLatin1String("post"))
                    return 1;
                return 0;
            };
            auto isZeroZero = [](const QVariantMap &candidate) {
                const QVariantList teams = candidate.value("teams").toList();
                if (teams.size() != 2)
                    return false;
                for (const auto &teamValue : teams) {
                    const QString score = teamValue.toMap().value("score").toString().trimmed();
                    if (score.isEmpty() || score == QStringLiteral("0"))
                        continue;
                    return false;
                }
                return true;
            };
            auto preferredOver = [&](const QVariantMap &candidate, const QVariantMap &current) {
                const int candRank = stateRank(candidate.value("state").toString());
                const int curRank = stateRank(current.value("state").toString());
                if (candRank != curRank)
                    return candRank > curRank;
                const bool candZero = isZeroZero(candidate);
                const bool curZero = isZeroZero(current);
                if (candZero != curZero)
                    return !candZero;
                return false;
            };
            for (int i = 0; i < deduped.size(); ++i) {
                const QVariantMap existing = deduped.at(i).toMap();
                const QString existingKey = [&existing]() {
                    const QVariantList teams = existing.value("teams").toList();
                    if (teams.size() != 2)
                        return QString();
                    QStringList ids;
                    for (const auto &teamValue : teams) {
                        const QVariantMap team = teamValue.toMap();
                        QString id = team.value("id").toString();
                        if (id.isEmpty())
                            id = team.value("abbreviation").toString();
                        if (id.isEmpty())
                            id = team.value("name").toString();
                        ids.append(id.trimmed().toLower());
                    }
                    std::sort(ids.begin(), ids.end());
                    return ids.join(QLatin1Char('|'));
                }();
                if (existingKey != key)
                    continue;
                const QDateTime otherStart = QDateTime::fromString(existing.value("start").toString(), Qt::ISODate);
                if (!otherStart.isValid())
                    continue;
                if (qAbs(start.secsTo(otherStart)) > 36 * 3600)
                    continue;
                if (preferredOver(event, existing))
                    deduped[i] = event;
                mergedDuplicate = true;
                break;
            }
        }
        if (!mergedDuplicate)
            deduped.append(event);
    }

    const QDateTime startOfToday(today, QTime(0, 0));
    QVariantList upcoming;
    for (const auto &value : deduped) {
        const QVariantMap event = value.toMap();
        if (event.value("state").toString() != QLatin1String("pre"))
            continue;
        const QDateTime start = QDateTime::fromString(event.value("start").toString(), Qt::ISODate).toLocalTime();
        if (!start.isValid() || start < startOfToday)
            continue;
        QVariantMap item = event;
        item.insert("localStart", start);
        item.insert("localDate", start.date());
        item.insert("timeLabel", QLocale().toString(start.time(), QLocale::ShortFormat));
        upcoming.append(item);
    }

    std::sort(upcoming.begin(), upcoming.end(), [](const QVariant &left, const QVariant &right) {
        return left.toMap().value("localStart").toDateTime()
             < right.toMap().value("localStart").toDateTime();
    });

    if (limit > 0 && upcoming.size() > limit)
        upcoming = upcoming.mid(0, limit);

    QVariantList groups;
    QDate currentDate;
    QVariantList matches;
    for (const auto &value : upcoming) {
        const QVariantMap event = value.toMap();
        const QDate day = event.value("localDate").toDate();
        if (day != currentDate) {
            if (!matches.isEmpty()) {
                QVariantMap group;
                group.insert("dateIso", currentDate.toString(Qt::ISODate));
                group.insert("label", QLocale().toString(currentDate, "ddd d MMM"));
                group.insert("matches", matches);
                groups.append(group);
                matches.clear();
            }
            currentDate = day;
        }
        QVariantMap match = event;
        match.remove("localStart");
        match.remove("localDate");
        matches.append(match);
    }
    if (!matches.isEmpty()) {
        QVariantMap group;
        group.insert("dateIso", currentDate.toString(Qt::ISODate));
        group.insert("label", QLocale().toString(currentDate, "ddd d MMM"));
        group.insert("matches", matches);
        groups.append(group);
    }

    return groups;
}

QVariantList Backend::parseScoreboard(const QByteArray &payload, QString *error,
                                         const QString &sport, const QString &league)
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

    const bool allowCountries = allowsCountryLogos(sport, league);

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

            QString name = team.value("displayName").toString();
            if (name.isEmpty())
                name = team.value("shortDisplayName").toString();
            if (name.isEmpty())
                name = team.value("name").toString();
            if (name.isEmpty())
                name = athlete.value("displayName").toString();
            if (name.isEmpty())
                name = team.value("abbreviation").toString();

            QString shortName = team.value("shortDisplayName").toString();
            if (shortName.isEmpty())
                shortName = team.value("name").toString();
            if (shortName.isEmpty())
                shortName = athlete.value("shortName").toString();

            QVariantMap item;
            item.insert("id", team.value("id").toString());
            item.insert("home", competitor.value("homeAway").toString() == "home");
            item.insert("homeAway", competitor.value("homeAway").toString());
            item.insert("name", name);
            item.insert("shortName", shortName);
            item.insert("abbreviation", team.value("abbreviation").toString());
            item.insert("logo", teamLogo(team, allowCountries));
            item.insert("color", team.value("color").toString());
            item.insert("score", scoreString(competitor.value("score")));
            item.insert("winner", competitor.value("winner").toBool());
            item.insert("record", competitorRecord(competitor));
            teams.append(item);
        }

        // Convention football : domicile en premier, extérieur ensuite quand
        // l'information de camp est connue. Sans camp, l'ordre du flux est conservé.
        orderHomeFirst(&teams);

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

        QString venue = competition.value("venue").toObject().value("fullName").toString();
        if (venue.isEmpty())
            venue = event.value("venue").toObject().value("fullName").toString();

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
        item.insert("venue", venue);
        events.append(item);
    }

    auto stateRank = [](const QString &state) {
        if (state == QLatin1String("in"))
            return 2;
        if (state == QLatin1String("post"))
            return 1;
        return 0; // pre ou inconnu
    };
    auto teamPairKey = [](const QVariantMap &event) {
        const QVariantList teams = event.value("teams").toList();
        if (teams.size() != 2)
            return QString();
        QStringList ids;
        for (const auto &value : teams) {
            const QVariantMap team = value.toMap();
            QString id = team.value("id").toString();
            if (id.isEmpty())
                id = team.value("abbreviation").toString();
            if (id.isEmpty())
                id = team.value("name").toString();
            ids.append(id.trimmed().toLower());
        }
        std::sort(ids.begin(), ids.end());
        return ids.join(QLatin1Char('|'));
    };
    auto isZeroZero = [](const QVariantMap &event) {
        const QVariantList teams = event.value("teams").toList();
        if (teams.size() != 2)
            return false;
        for (const auto &value : teams) {
            const QString score = value.toMap().value("score").toString().trimmed();
            if (score.isEmpty() || score == QStringLiteral("0"))
                continue;
            return false;
        }
        return true;
    };
    auto preferredOver = [&](const QVariantMap &candidate, const QVariantMap &current) {
        const int candRank = stateRank(candidate.value("state").toString());
        const int curRank = stateRank(current.value("state").toString());
        if (candRank != curRank)
            return candRank > curRank;
        const bool candZero = isZeroZero(candidate);
        const bool curZero = isZeroZero(current);
        if (candZero != curZero)
            return !candZero;
        return false;
    };

    QVariantList deduped;
    for (const auto &value : events) {
        const QVariantMap event = value.toMap();
        const QString key = teamPairKey(event);
        const QDateTime start = QDateTime::fromString(event.value("start").toString(), Qt::ISODate);
        bool merged = false;
        if (!key.isEmpty() && start.isValid()) {
            for (int i = 0; i < deduped.size(); ++i) {
                const QVariantMap existing = deduped.at(i).toMap();
                if (teamPairKey(existing) != key)
                    continue;
                const QDateTime otherStart = QDateTime::fromString(existing.value("start").toString(), Qt::ISODate);
                if (!otherStart.isValid())
                    continue;
                const qint64 diff = qAbs(start.secsTo(otherStart));
                if (diff > 36 * 3600)
                    continue;
                // Même affiche : on conserve le meilleur état, sinon le score non nul.
                if (preferredOver(event, existing))
                    deduped[i] = event;
                merged = true;
                break;
            }
        }
        if (!merged)
            deduped.append(event);
    }

    std::sort(deduped.begin(), deduped.end(), [](const QVariant &left, const QVariant &right) {
        return left.toMap().value("start").toString() < right.toMap().value("start").toString();
    });
    return deduped;
}

int Backend::liveCountOf(const QVariantList &events)
{
    int count = 0;
    for (const auto &event : events)
        if (event.toMap().value("state").toString() == "in")
            ++count;
    return count;
}

bool Backend::matchSummarySupported(const QString &sport)
{
    return sport != QLatin1String("golf") && sport != QLatin1String("tennis")
        && sport != QLatin1String("mma") && sport != QLatin1String("racing");
}
