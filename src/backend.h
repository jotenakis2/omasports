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
// league and one day at a time. The catalog is compiled in because ESPN has
// no public endpoint that lists its leagues; the scores themselves come from
// its public scoreboard API, so there is no key to configure.
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
    QDate date() const { return QDate::currentDate().addDays(m_dayOffset); }

    Q_INVOKABLE void selectLeague(int index);
    Q_INVOKABLE void nextDay();
    Q_INVOKABLE void previousDay();
    Q_INVOKABLE void today();
    Q_INVOKABLE void refresh();

    // Rebuilds the translated sport names after Omarchy's language changes.
    Q_INVOKABLE void retranslate();

    static QVariantList leagueCatalog();
    static QVariantList parseScoreboard(const QByteArray &payload, QString *error = nullptr);
    static int liveCountOf(const QVariantList &events);
    static QUrl scoreboardUrl(const QString &sport, const QString &league, const QDate &date);

signals:
    void statusChanged();
    void leaguesChanged();
    void leagueChanged();
    void eventsChanged();
    void dateChanged();

private:
    void notifyAll();
    void saveSelection() const;
    bool selectBySlug(const QString &sport, const QString &league);

    QString m_settingsPath;
    bool m_online = true;
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_pending;
    QVariantList m_leagues;
    QVariantList m_events;
    int m_leagueIndex = 0;
    int m_dayOffset = 0;
    int m_liveCount = 0;
    QString m_status = "idle";
    QString m_error;
    QDateTime m_updatedAt;
};
