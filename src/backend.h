#pragma once

#include <QDate>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QVariantList>
#include <QVariantMap>

class QNetworkReply;

// Scores, fixtures, and live results for the leagues in the catalog, one
// league and one day at a time, plus a live view that scans every league at
// once. The catalog is compiled in because ESPN has no public endpoint that
// lists its leagues; the scores themselves come from its public scoreboard
// API, so there is no key to configure.
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
    Q_PROPERTY(QVariantList sports READ sports NOTIFY sportsChanged)
    Q_PROPERTY(QStringList enabledSports READ enabledSports NOTIFY sportsChanged)

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
    QVariantList sports() const;
    QStringList enabledSports() const { return m_enabledSports; }
    QDate date() const { return QDate::currentDate().addDays(m_dayOffset); }

    Q_INVOKABLE void selectLeague(int index);
    Q_INVOKABLE void nextDay();
    Q_INVOKABLE void previousDay();
    Q_INVOKABLE void today();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void toggleLive();
    Q_INVOKABLE void setLive(bool live);
    Q_INVOKABLE void setSportEnabled(const QString &slug, bool enabled);
    Q_INVOKABLE void setAllSportsEnabled(bool enabled);

    // "3-1-5" read as a season record: 3V · 1N · 5D for soccer, 8V · 3D · 2OT
    // for hockey, and so on. Letters come from the Omarchy language.
    Q_INVOKABLE QString recordLabel(const QString &sport, const QString &record) const;

    // Rebuilds the translated sport names after Omarchy's language changes.
    Q_INVOKABLE void retranslate();

    static QVariantList leagueCatalog();
    static QVariantList parseScoreboard(const QByteArray &payload, QString *error = nullptr);
    static QVariantList parseLiveHeader(const QByteArray &payload, QString *error = nullptr);
    static int liveCountOf(const QVariantList &events);
    static QUrl scoreboardUrl(const QString &sport, const QString &league, const QDate &date);
    static QUrl liveHeaderUrl();

signals:
    void statusChanged();
    void leaguesChanged();
    void leagueChanged();
    void eventsChanged();
    void dateChanged();
    void liveModeChanged();
    void sportsChanged();

private:
    void notifyAll();
    void cancelPending();
    void fetchLive();
    void applySportsChange();
    void saveSelection() const;
    void saveSports() const;
    bool selectBySlug(const QString &sport, const QString &league);
    int firstEnabledLeague() const;

    QString m_settingsPath;
    bool m_online = true;
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_pending;
    QVariantList m_leagues;
    QVariantList m_events;
    int m_leagueIndex = 0;
    int m_dayOffset = 0;
    int m_liveCount = 0;
    bool m_liveMode = false;
    QStringList m_enabledSports;
    QString m_status = "idle";
    QString m_error;
    QDateTime m_updatedAt;
};
