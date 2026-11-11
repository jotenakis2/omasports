#pragma once

#include <QDate>
#include <QDateTime>
#include <QList>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>

class QNetworkReply;

// Scores, fixtures, and live results for the leagues in the catalog, one
// league and one day at a time, plus a live view that scans every enabled
// league at once. The catalog is compiled in because ESPN has no public
// endpoint that lists its leagues; the scores themselves come from its public
// scoreboard API, so there is no key to configure.
class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString error READ error NOTIFY statusChanged)
    Q_PROPERTY(QVariantList leagues READ leagues NOTIFY leaguesChanged)
    Q_PROPERTY(int leagueIndex READ leagueIndex NOTIFY leagueChanged)
    Q_PROPERTY(QVariantMap league READ league NOTIFY leagueChanged)
    Q_PROPERTY(QVariantList events READ events NOTIFY eventsChanged)
    Q_PROPERTY(int liveCount READ liveCount NOTIFY eventsChanged)
    Q_PROPERTY(QString dateLabel READ dateLabel NOTIFY dateChanged)
    Q_PROPERTY(QString dateIso READ dateIso NOTIFY dateChanged)
    Q_PROPERTY(QString updatedLabel READ updatedLabel NOTIFY eventsChanged)
    Q_PROPERTY(bool isToday READ isToday NOTIFY dateChanged)
    Q_PROPERTY(bool liveMode READ liveMode NOTIFY liveModeChanged)
    Q_PROPERTY(bool standingsVisible READ standingsVisible NOTIFY standingsChanged)
    Q_PROPERTY(QVariantList standings READ standings NOTIFY standingsChanged)
    Q_PROPERTY(QStringList standingsHeaders READ standingsHeaders NOTIFY standingsChanged)
    Q_PROPERTY(bool standingsSeasonUncertain READ standingsSeasonUncertain NOTIFY standingsChanged)
    Q_PROPERTY(QVariantList upcoming READ upcoming NOTIFY upcomingChanged)
    Q_PROPERTY(bool upcomingLoading READ upcomingLoading NOTIFY upcomingChanged)
    Q_PROPERTY(QString upcomingError READ upcomingError NOTIFY upcomingChanged)
    Q_PROPERTY(bool upcomingVisible READ upcomingVisible NOTIFY upcomingChanged)
    Q_PROPERTY(QVariantList settingsRows READ settingsRows NOTIFY leaguesChanged)
    Q_PROPERTY(QStringList enabledLeagues READ enabledLeagues NOTIFY leaguesChanged)
    Q_PROPERTY(bool matchOpen READ matchOpen NOTIFY matchChanged)
    Q_PROPERTY(QString matchStatus READ matchStatus NOTIFY matchChanged)
    Q_PROPERTY(QString matchError READ matchError NOTIFY matchChanged)
    Q_PROPERTY(QVariantMap match READ match NOTIFY matchChanged)
    Q_PROPERTY(int fontScaleLevel READ fontScaleLevel NOTIFY fontScaleChanged)
    Q_PROPERTY(qreal fontScale READ fontScale NOTIFY fontScaleChanged)

public:
    // Tests construct the backend offline so selection and navigation never
    // touch the network.
    explicit Backend(const QString &settingsPath = {}, bool online = true, QObject *parent = nullptr);

    QString status() const { return m_status; }
    QString error() const { return m_error; }
    QVariantList leagues() const { return m_leagues; }
    int leagueIndex() const { return m_leagueIndex; }
    QVariantMap league() const;
    QVariantList events() const { return m_events; }
    int liveCount() const { return m_liveCount; }
    QString dateLabel() const;
    QString dateIso() const { return date().toString("yyyy-MM-dd"); }
    QString updatedLabel() const;
    bool isToday() const { return m_dayOffset == 0; }
    bool liveMode() const { return m_liveMode; }
    bool standingsVisible() const { return m_standingsVisible; }
    QVariantList standings() const { return m_standings; }
    QStringList standingsHeaders() const { return m_standingsHeaders; }
    bool standingsSeasonUncertain() const { return m_standingsSeasonUncertain; }
    QVariantList upcoming() const { return m_upcoming; }
    bool upcomingLoading() const { return m_upcomingLoading; }
    QString upcomingError() const { return m_upcomingError; }
    bool upcomingVisible() const { return m_upcomingVisible; }
    QVariantList settingsRows() const;
    QStringList enabledLeagues() const;
    bool matchOpen() const { return m_matchOpen; }
    QString matchStatus() const { return m_matchStatus; }
    QString matchError() const { return m_matchError; }
    QVariantMap match() const { return m_match; }
    int fontScaleLevel() const { return m_fontScaleLevel; }
    qreal fontScale() const;
    QDate date() const { return QDate::currentDate().addDays(m_dayOffset); }

    Q_INVOKABLE void selectLeague(int index);
    Q_INVOKABLE void nextDay();
    Q_INVOKABLE void previousDay();
    Q_INVOKABLE void today();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void toggleLive();
    Q_INVOKABLE void setLive(bool live);
    Q_INVOKABLE void toggleStandings();
    Q_INVOKABLE void setStandings(bool visible);
    Q_INVOKABLE void toggleUpcoming();
    Q_INVOKABLE void setUpcoming(bool visible);
    Q_INVOKABLE void refreshUpcoming();
    Q_INVOKABLE void invalidateUpcomingCache();
    Q_INVOKABLE void openMatch(const QString &id);

    int upcomingRequestCount() const { return m_upcomingNetworkRequests; }
    void injectUpcomingMonthPayload(const QString &sport, const QString &league,
                                    const QString &month, const QByteArray &payload);
    Q_INVOKABLE void closeMatch();
    Q_INVOKABLE void refreshMatch();
    Q_INVOKABLE void setLeagueEnabled(const QString &sport, const QString &league, bool enabled);
    Q_INVOKABLE void setSportLeaguesEnabled(const QString &sport, bool enabled);
    Q_INVOKABLE void setAllLeaguesEnabled(bool enabled);
    Q_INVOKABLE void setFontScale(int level);

    // "3-1-5" read as a season record: 3V · 1N · 5D for soccer, 8V · 3D · 2OT
    // for hockey, and so on. Letters come from the Omarchy language.
    Q_INVOKABLE QString recordLabel(const QString &sport, const QString &record) const;

    // Rebuilds the translated sport names after Omarchy's language changes.
    Q_INVOKABLE void retranslate();

    static QVariantList leagueCatalog();
    static QVariantList parseScoreboard(const QByteArray &payload, QString *error = nullptr,
                                         const QString &sport = {}, const QString &league = {});
    static QVariantList parseUpcomingSchedule(const QByteArray &currentMonthPayload,
                                              const QByteArray &nextMonthPayload,
                                              const QDate &today, QString *error = nullptr,
                                              const QString &sport = {},
                                              const QString &league = {},
                                              int limit = 200);
    static QUrl scoreboardMonthUrl(const QString &sport, const QString &league,
                                   const QDate &monthDate);
    static QVariantList parseStandings(const QByteArray &payload, const QString &sport,
                                       QString *error = nullptr, const QString &league = {});
    static QVariantMap parseMatchSummary(const QByteArray &payload, const QVariantMap &event,
                                         QString *error = nullptr);
    static QVariantList liveEventsFrom(const QVariantList &scoreboardEvents, const QString &sport,
                                       const QString &leagueSlug, const QString &sportName,
                                       const QString &leagueName);
    static int liveCountOf(const QVariantList &events);
    static QStringList standingsHeadersFor(const QString &sport);
    static QUrl scoreboardUrl(const QString &sport, const QString &league, const QDate &date);
    static int standingsSeason(const QString &sport, const QDate &date);
    static int scoreboardSeasonYear(const QByteArray &payload);
    static bool scoreboardIndicatesSeasonInProgress(const QByteArray &payload);
    static QList<int> standingsSeasonCandidates(const QString &sport, const QDate &date,
                                                int scoreboardYear = 0);
    static QUrl standingsUrlForSeason(const QString &sport, const QString &league, int season);
    static QUrl standingsUrl(const QString &sport, const QString &league, const QDate &date);
    // Choisit la saison de classement la plus plausible parmi les candidates, à
    // partir des réponses JSON déjà chargées (une par saison). Aucune requête
    // réseau : les tests fournissent des charges enregistrées.
    static QVariantMap chooseStandingsSeason(const QString &sport, const QList<int> &candidates,
                                             const QMap<int, QByteArray> &payloads,
                                             bool scoreboardSeasonInProgress,
                                             const QString &league = {});

signals:
    void statusChanged();
    void leaguesChanged();
    void leagueChanged();
    void eventsChanged();
    void dateChanged();
    void liveModeChanged();
    void standingsChanged();
    void upcomingChanged();
    void matchChanged();
    void fontScaleChanged();

private:
    struct LiveTask {
        QUrl url;
        QString sport;
        QString league;
        QString sportName;
        QString leagueName;
    };

    void notifyAll();
    void cancelPending();
    void fetchLive();
    void fetchStandings();
    void fetchUpcoming(bool force = false);
    void fetchMatch();
    void cancelMatchPending();
    void startLiveRequests();
    void finishLive();
    void cancelLiveRequests();
    void applyLeaguesChange();
    void saveSelection() const;
    void saveLeagues() const;
    bool selectBySlug(const QString &sport, const QString &league);
    bool leagueEnabled(const QString &sport, const QString &league) const;
    int firstEnabledLeague() const;

    QString m_settingsPath;
    bool m_online = true;
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_pending;

    QList<LiveTask> m_liveQueue;
    QSet<QNetworkReply *> m_liveReplies;
    QSet<QString> m_liveSeen;
    QVariantList m_liveEvents;
    int m_liveRequests = 0;
    int m_liveFailures = 0;
    QString m_liveError;

    QVariantList m_leagues;
    QVariantList m_events;
    QVariantList m_standings;
    QVariantList m_upcoming;
    QStringList m_standingsHeaders;
    QVariantMap m_match;
    QString m_matchId;
    QString m_matchStatus;
    QString m_matchError;
    QPointer<QNetworkReply> m_matchPending;
    bool m_matchOpen = false;
    QSet<QString> m_enabledLeagues;
    int m_leagueIndex = 0;
    int m_dayOffset = 0;
    int m_liveCount = 0;
    int m_fontScaleLevel = 2;
    bool m_liveMode = false;
    bool m_standingsVisible = false;
    bool m_standingsSeasonUncertain = false;
    bool m_upcomingVisible = false;
    bool m_upcomingLoading = false;
    QString m_status = "idle";
    QString m_error;
    QString m_upcomingError;
    QDateTime m_updatedAt;

    struct UpcomingCacheEntry {
        QByteArray payload;
        QDateTime fetchedAt;
    };
    QMap<QString, UpcomingCacheEntry> m_upcomingCache;
    QSet<QNetworkReply *> m_upcomingReplies;
    int m_upcomingPending = 0;
    int m_upcomingNetworkRequests = 0;
    QByteArray m_upcomingCurrentPayload;
    QByteArray m_upcomingNextPayload;
    bool m_upcomingHadNetworkError = false;
    QString m_upcomingNetworkError;
};
