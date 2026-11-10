#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QMap>
#include <QSet>
#include <QSettings>
#include <QTemporaryDir>
#include <QUrlQuery>
#include <QtTest>

#include "backend.h"
#include "theme.h"
#include "translator.h"

namespace {

const char *nbaPayload = R"({
  "events": [{
    "id": "401",
    "name": "Orlando Magic at Phoenix Suns",
    "shortName": "ORL @ PHX",
    "date": "2026-11-06T02:00Z",
    "status": {"type": {"state": "in", "detail": "2nd 4:32",
                        "shortDetail": "2nd 4:32", "completed": false}},
    "competitions": [{
      "status": {"type": {"state": "in", "detail": "2nd 4:32",
                          "shortDetail": "2nd 4:32", "completed": false}},
      "competitors": [
        {"homeAway": "home", "winner": true, "score": "58",
         "records": [{"name": "overall", "summary": "5-2"}],
         "team": {"abbreviation": "PHX", "displayName": "Phoenix Suns",
                  "color": "29127a", "logo": "phx.png"}},
        {"homeAway": "away", "winner": false, "score": "56",
         "records": [{"name": "overall", "summary": "3-4"}],
         "team": {"abbreviation": "ORL", "displayName": "Orlando Magic",
                  "color": "0150b5", "logo": "orl.png"}}
      ],
      "venue": {"fullName": "Footprint Center"}
    }]
  }]
})";

const char *tournamentPayload = R"({
  "events": [{
    "id": "9",
    "name": "World Wide Technology Championship",
    "date": "2026-11-05T15:00Z",
    "status": {"type": {"state": "pre", "detail": "Thu, November 5th",
                        "shortDetail": "11/5"}},
    "competitions": [{
      "competitors": [{"athlete": {"displayName": "Someone"}}],
      "venue": {"fullName": "El Cardonal"}
    }]
  }]
})";

const char *soccerStandings = R"({
  "children": [{
    "name": "French Ligue 1 2026-27",
    "standings": {"entries": [
      {
        "team": {"displayName": "Marseille", "abbreviation": "MAR",
                 "color": "abc", "logo": "mar.png"},
        "stats": [
          {"name": "gamesPlayed", "value": 9, "displayValue": "9"},
          {"name": "wins", "value": 5, "displayValue": "5"},
          {"name": "ties", "value": 2, "displayValue": "2"},
          {"name": "losses", "value": 2, "displayValue": "2"},
          {"name": "pointDifferential", "value": 6, "displayValue": "+6"},
          {"name": "points", "value": 17, "displayValue": "17"},
          {"name": "rank", "value": 2, "displayValue": "2"}
        ]
      },
      {
        "team": {"displayName": "Paris Saint-Germain", "abbreviation": "PSG",
                 "color": "def", "logo": "psg.png"},
        "stats": [
          {"name": "gamesPlayed", "value": 9, "displayValue": "9"},
          {"name": "wins", "value": 6, "displayValue": "6"},
          {"name": "ties", "value": 2, "displayValue": "2"},
          {"name": "losses", "value": 1, "displayValue": "1"},
          {"name": "pointDifferential", "value": 11, "displayValue": "+11"},
          {"name": "points", "value": 20, "displayValue": "20"},
          {"name": "rank", "value": 1, "displayValue": "1"}
        ]
      }
    ]}
  }]
})";

const char *basketballStandings = R"({
  "children": [{
    "name": "Eastern Conference",
    "standings": {"entries": [
      {
        "team": {"displayName": "Boston Celtics", "abbreviation": "BOS",
                 "color": "123", "logo": "bos.png"},
        "stats": [
          {"name": "wins", "value": 6, "displayValue": "6"},
          {"name": "losses", "value": 2, "displayValue": "2"},
          {"name": "winPercent", "value": 0.75, "displayValue": ".750"},
          {"name": "gamesBehind", "value": 0, "displayValue": "-"},
          {"name": "playoffSeed", "value": 1, "displayValue": "1"}
        ]
      },
      {
        "team": {"displayName": "New York Knicks", "abbreviation": "NYK",
                 "color": "456", "logo": "nyk.png"},
        "stats": [
          {"name": "wins", "value": 4, "displayValue": "4"},
          {"name": "losses", "value": 4, "displayValue": "4"},
          {"name": "winPercent", "value": 0.5, "displayValue": ".500"},
          {"name": "gamesBehind", "value": 2, "displayValue": "2"},
          {"name": "playoffSeed", "value": 2, "displayValue": "2"}
        ]
      }
    ]}
  }, {
    "name": "Western Conference",
    "standings": {"entries": [
      {
        "team": {"displayName": "Denver Nuggets", "abbreviation": "DEN",
                 "color": "789", "logo": "den.png"},
        "stats": [
          {"name": "wins", "value": 7, "displayValue": "7"},
          {"name": "losses", "value": 1, "displayValue": "1"},
          {"name": "winPercent", "value": 0.875, "displayValue": ".875"},
          {"name": "gamesBehind", "value": 0, "displayValue": "-"},
          {"name": "playoffSeed", "value": 1, "displayValue": "1"}
        ]
      }
    ]}
  }]
})";

const char *soccerMatchPayload = R"({
  "events": [{
    "id": "42",
    "name": "Strasbourg at Toulouse",
    "date": "2026-11-06T20:00Z",
    "status": {"type": {"state": "post", "detail": "FT", "shortDetail": "FT",
                        "completed": true}},
    "competitions": [{
      "status": {"type": {"state": "post", "detail": "FT", "shortDetail": "FT",
                          "completed": true}},
      "competitors": [
        {"homeAway": "home", "winner": true, "score": "3",
         "team": {"id": "179", "displayName": "Toulouse", "abbreviation": "TLS",
                  "color": "123456", "logo": "tls.png"}},
        {"homeAway": "away", "winner": false, "score": "1",
         "team": {"id": "180", "displayName": "Strasbourg", "abbreviation": "STR",
                  "color": "654321", "logo": "str.png"}}
      ],
      "details": [
        {"clock": {"displayValue": "27'"}, "type": {"text": "Yellow Card"},
         "team": {"id": "180"}, "yellowCard": true,
         "athletesInvolved": [{"displayName": "Karim Coulibaly"}]},
        {"clock": {"displayValue": "45'+1'"}, "type": {"text": "Goal"},
         "team": {"id": "179"}, "scoringPlay": true,
         "athletesInvolved": [{"displayName": "Santiago Hidalgo"}]},
        {"clock": {"displayValue": "56'"}, "type": {"text": "Goal - Header"},
         "team": {"id": "179"}, "scoringPlay": true,
         "athletesInvolved": [{"displayName": "Mark McKenzie"}]},
        {"clock": {"displayValue": "67'"}, "type": {"text": "Goal"},
         "team": {"id": "180"}, "scoringPlay": true,
         "athletesInvolved": [{"displayName": "Giovanni Reyna"}]},
        {"clock": {"displayValue": "76'"}, "type": {"text": "Goal - Volley"},
         "team": {"id": "179"}, "scoringPlay": true,
         "athletesInvolved": [{"displayName": "Thomas Jørgensen"}]}
      ]
    }]
  }]
})";

const char *matchSummaryPayload = R"({
  "keyEvents": [
    {"clock": {"displayValue": "45'+1'"}, "type": {"text": "Goal"},
     "scoringPlay": true, "team": {"id": "179", "displayName": "Toulouse"},
     "participants": [
       {"athlete": {"displayName": "Santiago Hidalgo"}},
       {"athlete": {"displayName": "Sion Oppong"}}
     ],
     "text": "Goal! Toulouse 1, Strasbourg 0. Santiago Hidalgo assisted by Sion Oppong."},
    {"clock": {"displayValue": "67'"}, "type": {"text": "Goal"},
     "scoringPlay": true, "team": {"id": "180", "displayName": "Strasbourg"},
     "participants": [{"athlete": {"displayName": "Giovanni Reyna"}}],
     "text": "Goal! Toulouse 2, Strasbourg 1. Giovanni Reyna."},
    {"clock": {"displayValue": "27'"}, "type": {"text": "Yellow Card"},
     "team": {"id": "180", "displayName": "Strasbourg"},
     "participants": [{"athlete": {"displayName": "Karim Coulibaly"}}],
     "text": "Karim Coulibaly is shown the yellow card."}
  ],
  "boxscore": {"teams": [
    {"homeAway": "home", "statistics": [
      {"name": "possessionPct", "displayValue": "47.3"},
      {"name": "totalShots", "displayValue": "10"},
      {"name": "shotsOnTarget", "displayValue": "5"},
      {"name": "wonCorners", "displayValue": "7"},
      {"name": "foulsCommitted", "displayValue": "16"},
      {"name": "offsides", "displayValue": "1"},
      {"name": "yellowCards", "displayValue": "3"},
      {"name": "redCards", "displayValue": "0"},
      {"name": "saves", "displayValue": "3"}
    ]},
    {"homeAway": "away", "statistics": [
      {"name": "possessionPct", "displayValue": "52.7"},
      {"name": "totalShots", "displayValue": "12"},
      {"name": "shotsOnTarget", "displayValue": "6"},
      {"name": "wonCorners", "displayValue": "4"},
      {"name": "foulsCommitted", "displayValue": "11"},
      {"name": "offsides", "displayValue": "2"},
      {"name": "yellowCards", "displayValue": "1"},
      {"name": "redCards", "displayValue": "0"},
      {"name": "saves", "displayValue": "2"}
    ]}
  ]}
})";

void writeFile(const QString &path, const QByteArray &contents)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(contents);
}

} // namespace

class OmASportsTests : public QObject {
    Q_OBJECT

private slots:
    void eventCardScoreUsesDedicatedColumn()
    {
        QFile file(QStringLiteral(SRCDIR "/../src/EventCard.qml"));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            file.setFileName(QStringLiteral("src/EventCard.qml"));
        QVERIFY2(file.isOpen() || file.open(QIODevice::ReadOnly | QIODevice::Text),
                 "cannot open EventCard.qml");
        const QString qml = QString::fromUtf8(file.readAll());
        // Le score doit être dans sa propre colonne, largeur fixe, aligné à droite
        // sur la ligne du nom, indépendant du record/badge/nom long.
        QVERIFY(qml.contains("Layout.preferredWidth: 46"));
        QVERIFY(qml.contains("Layout.alignment: Qt.AlignTop | Qt.AlignRight"));
        QVERIFY(qml.contains("elide: Text.ElideNone"));
        QVERIFY(qml.contains("maximumLineCount: 1"));
        // Le score n'est plus imbriqué dans la RowLayout du nom (ancienne structure).
        QVERIFY(!qml.contains("Text {\n                                    Layout.preferredWidth: 46"));
    }

    void catalogCoversEverySport()
    {
        const QVariantList catalog = Backend::leagueCatalog();
        QVERIFY(catalog.size() >= 30);

        QSet<QString> sports;
        QSet<QString> keys;
        for (int i = 0; i < catalog.size(); ++i) {
            const QVariantMap entry = catalog.at(i).toMap();
            QCOMPARE(entry.value("index").toInt(), i);
            QVERIFY(!entry.value("sport").toString().isEmpty());
            QVERIFY(!entry.value("sportName").toString().isEmpty());
            QVERIFY(!entry.value("league").toString().isEmpty());
            QVERIFY(!entry.value("name").toString().isEmpty());
            sports.insert(entry.value("sport").toString());
            const QString key = entry.value("sport").toString() + "/" + entry.value("league").toString();
            QVERIFY2(!keys.contains(key), qPrintable(key));
            keys.insert(key);
        }
        QVERIFY(sports.contains("soccer"));
        QVERIFY(sports.contains("basketball"));
        QVERIFY(sports.contains("hockey"));
        QVERIFY(sports.contains("tennis"));
    }

    void scoreboardUrlCarriesSportLeagueAndDay()
    {
        const QUrl url = Backend::scoreboardUrl("soccer", "eng.1", QDate(2026, 11, 5));
        QCOMPARE(url.path(), QStringLiteral("/apis/site/v2/sports/soccer/eng.1/scoreboard"));
        QCOMPARE(QUrlQuery(url).queryItemValue("dates"), QStringLiteral("20261105"));
    }

    void parseScoreboardReadsBothSides()
    {
        QString error;
        const QVariantList events = Backend::parseScoreboard(nbaPayload, &error);
        QVERIFY(error.isEmpty());
        QCOMPARE(events.size(), 1);

        const QVariantMap event = events.first().toMap();
        QCOMPARE(event.value("state").toString(), QStringLiteral("in"));
        QCOMPARE(event.value("venue").toString(), QStringLiteral("Footprint Center"));
        QCOMPARE(event.value("teams").toList().size(), 2);

        const QVariantList teams = event.value("teams").toList();
        // Le domicile est affiché en premier, l'extérieur ensuite.
        const QVariantMap home = teams.first().toMap();
        const QVariantMap away = teams.last().toMap();
        QCOMPARE(home.value("name").toString(), QStringLiteral("Phoenix Suns"));
        QCOMPARE(home.value("score").toString(), QStringLiteral("58"));
        QVERIFY(home.value("winner").toBool());
        QCOMPARE(away.value("name").toString(), QStringLiteral("Orlando Magic"));
        QCOMPARE(away.value("score").toString(), QStringLiteral("56"));
        QCOMPARE(away.value("record").toString(), QStringLiteral("3-4"));
        QCOMPARE(away.value("logo").toString(), QStringLiteral("orl.png"));

        QCOMPARE(Backend::liveCountOf(events), 1);
    }

    void parseScoreboardKeepsHomeAwayOrderWhenKnown()
    {
        // Domicile en tête dans le flux : l'ordre reste domicile puis extérieur.
        const char *homeFirst = R"({
          "events": [{
            "id": "h", "name": "A v B", "date": "2026-11-06T20:00Z",
            "status": {"type": {"state": "pre", "detail": "20:00", "shortDetail": "20:00", "completed": false}},
            "competitions": [{"competitors": [
              {"homeAway": "home", "score": "0", "team": {"displayName": "Home FC"}},
              {"homeAway": "away", "score": "0", "team": {"displayName": "Away FC"}}
            ]}]
          }]
        })";
        QString error;
        QVariantList events = Backend::parseScoreboard(homeFirst, &error);
        QVERIFY(error.isEmpty());
        QVariantList teams = events.first().toMap().value("teams").toList();
        QCOMPARE(teams.first().toMap().value("name").toString(), QStringLiteral("Home FC"));
        QCOMPARE(teams.last().toMap().value("name").toString(), QStringLiteral("Away FC"));

        // Extérieur en tête dans le flux : on remet domicile puis extérieur.
        const char *awayFirst = R"({
          "events": [{
            "id": "a", "name": "B v A", "date": "2026-11-06T20:00Z",
            "status": {"type": {"state": "pre", "detail": "20:00", "shortDetail": "20:00", "completed": false}},
            "competitions": [{"competitors": [
              {"homeAway": "away", "score": "0", "team": {"displayName": "Away FC"}},
              {"homeAway": "home", "score": "0", "team": {"displayName": "Home FC"}}
            ]}]
          }]
        })";
        events = Backend::parseScoreboard(awayFirst, &error);
        QVERIFY(error.isEmpty());
        teams = events.first().toMap().value("teams").toList();
        QCOMPARE(teams.first().toMap().value("name").toString(), QStringLiteral("Home FC"));
        QCOMPARE(teams.last().toMap().value("name").toString(), QStringLiteral("Away FC"));
    }

    void parseScoreboardKeepsFeedOrderWithoutHomeAway()
    {
        // Sans notion de camp (tournoi, course, etc.) l'ordre du flux est conservé.
        const char *noCamps = R"({
          "events": [{
            "id": "t", "name": "Race", "date": "2026-11-06T12:00Z",
            "status": {"type": {"state": "post", "detail": "Final", "shortDetail": "Final", "completed": true}},
            "competitions": [{"competitors": [
              {"score": "1", "athlete": {"displayName": "Alpha"}},
              {"score": "2", "athlete": {"displayName": "Beta"}}
            ]}]
          }]
        })";
        QString error;
        const QVariantList events = Backend::parseScoreboard(noCamps, &error);
        QVERIFY(error.isEmpty());
        const QVariantList teams = events.first().toMap().value("teams").toList();
        QCOMPARE(teams.first().toMap().value("name").toString(), QStringLiteral("Alpha"));
        QCOMPARE(teams.last().toMap().value("name").toString(), QStringLiteral("Beta"));
    }

    void parseScoreboardKeepsTournamentEvents()
    {
        QString error;
        const QVariantList events = Backend::parseScoreboard(tournamentPayload, &error);
        QVERIFY(error.isEmpty());
        QCOMPARE(events.size(), 1);

        const QVariantMap event = events.first().toMap();
        QCOMPARE(event.value("teams").toList().size(), 1);
        QCOMPARE(event.value("name").toString(),
                 QStringLiteral("World Wide Technology Championship"));
        QCOMPARE(event.value("venue").toString(), QStringLiteral("El Cardonal"));
        QCOMPARE(Backend::liveCountOf(events), 0);
    }

    void parseScoreboardDeduplicatesGhostAndReversedFixtures()
    {
        // Même affiche à moins de 36 h, dont une inversée et non jouée (pre, 0-0) :
        // on conserve le vrai match terminé.
        const char *payload = R"({
          "events": [
            {
              "id": "ghost",
              "name": "Perpignan at Section Paloise",
              "date": "2026-11-07T15:00Z",
              "status": {"type": {"state": "pre", "detail": "Sat, November 7th", "shortDetail": "11/7", "completed": false}},
              "competitions": [{"competitors": [
                {"homeAway": "home", "score": "0",
                 "team": {"id": "1", "displayName": "Section Paloise", "abbreviation": "PAU"}},
                {"homeAway": "away", "score": "0",
                 "team": {"id": "2", "displayName": "Perpignan", "abbreviation": "PER"}}
              ]}]
            },
            {
              "id": "real",
              "name": "Section Paloise at Perpignan",
              "date": "2026-11-07T15:00Z",
              "status": {"type": {"state": "post", "detail": "FT", "shortDetail": "FT", "completed": true}},
              "competitions": [{"competitors": [
                {"homeAway": "home", "score": "26",
                 "team": {"id": "2", "displayName": "Perpignan", "abbreviation": "PER"}},
                {"homeAway": "away", "score": "63",
                 "team": {"id": "1", "displayName": "Section Paloise", "abbreviation": "PAU"}}
              ]}]
            }
          ]
        })";

        QString error;
        const QVariantList events = Backend::parseScoreboard(payload, &error);
        QVERIFY(error.isEmpty());
        QCOMPARE(events.size(), 1);
        QCOMPARE(events.first().toMap().value("id").toString(), QStringLiteral("real"));
        QCOMPARE(events.first().toMap().value("state").toString(), QStringLiteral("post"));
    }

    void parseScoreboardHandlesEmptyAndBrokenPayloads()
    {
        QString error;
        QVERIFY(Backend::parseScoreboard(R"({"events": []})", &error).isEmpty());
        QVERIFY(error.isEmpty());

        const QVariantList events = Backend::parseScoreboard("not json", &error);
        QVERIFY(events.isEmpty());
        QVERIFY(!error.isEmpty());
    }

    void recordsReadAsSeasonTotals()
    {
        QTemporaryDir dir;
        Backend backend(dir.filePath("settings.ini"), false);

        QCOMPARE(backend.recordLabel("soccer", "3-1-5"), QStringLiteral("3W · 1D · 5L"));
        QCOMPARE(backend.recordLabel("hockey", "11-3-3"), QStringLiteral("11W · 3L · 3OT"));
        QCOMPARE(backend.recordLabel("football", "6-2-1"), QStringLiteral("6W · 2L · 1T"));
        QCOMPARE(backend.recordLabel("football", "0-7"), QStringLiteral("0W · 7L"));
        QCOMPARE(backend.recordLabel("basketball", "4-4"), QStringLiteral("4W · 4L"));
        QCOMPARE(backend.recordLabel("baseball", "103-59"), QStringLiteral("103W · 59L"));
        QCOMPARE(backend.recordLabel("mma", "10-1-0"), QStringLiteral("10W · 1L · 0D"));
        QCOMPARE(backend.recordLabel("rugby", "LWWWW"), QStringLiteral("Form: L W W W W"));
        // Unexpected shapes and sports without a known layout stay as they are.
        QCOMPARE(backend.recordLabel("soccer", "3-1"), QStringLiteral("3-1"));
        QCOMPARE(backend.recordLabel("rugby", "4-1"), QStringLiteral("4-1"));
        QCOMPARE(backend.recordLabel("soccer", ""), QString());
    }

    void liveEventsAreFilteredAndAnnotated()
    {
        QString error;
        const QVariantList scoreboard = Backend::parseScoreboard(nbaPayload, &error);
        QVERIFY(error.isEmpty());

        const QVariantList live = Backend::liveEventsFrom(scoreboard, "basketball", "nba",
                                                          "Basketball", "NBA");
        QCOMPARE(live.size(), 1);
        const QVariantMap event = live.first().toMap();
        QCOMPARE(event.value("sport").toString(), QStringLiteral("basketball"));
        QCOMPARE(event.value("league").toString(), QStringLiteral("nba"));
        QCOMPARE(event.value("leagueName").toString(), QStringLiteral("NBA"));
        QCOMPARE(event.value("sectionLabel").toString(), QStringLiteral("Basketball · NBA"));
        QCOMPARE(Backend::liveCountOf(live), 1);

        // A schedule with nothing in progress contributes no live events.
        const QVariantList upcoming = Backend::parseScoreboard(tournamentPayload, &error);
        QVERIFY(error.isEmpty());
        QVERIFY(Backend::liveEventsFrom(upcoming, "golf", "pga",
                                        "Golf", "PGA Tour").isEmpty());
    }

    void parseStandingsSortsAndPicksColumns()
    {
        QString error;
        const QVariantList groups = Backend::parseStandings(soccerStandings, "soccer", &error);
        QVERIFY(error.isEmpty());
        QCOMPARE(groups.size(), 1);

        const QVariantMap group = groups.first().toMap();
        // A single table has no group heading.
        QCOMPARE(group.value("name").toString(), QString());
        const QVariantList rows = group.value("rows").toList();
        QCOMPARE(rows.size(), 2);
        QCOMPARE(rows.first().toMap().value("rank").toInt(), 1);
        QCOMPARE(rows.first().toMap().value("team").toMap().value("name").toString(),
                 QStringLiteral("Paris Saint-Germain"));
        QCOMPARE(rows.first().toMap().value("cells").toStringList(),
                 QStringList({"20", "9", "6", "2", "1", "+11"}));

        QCOMPARE(Backend::standingsHeadersFor("soccer"),
                 QStringList({"Pts", "MP", "W", "D", "L", "GD"}));
        QCOMPARE(Backend::standingsHeadersFor("rugby"),
                 QStringList({"Pts", "MP", "W", "D", "L"}));
        QCOMPARE(Backend::standingsHeadersFor("hockey"),
                 QStringList({"Pts", "W", "L", "OTL"}));
        QCOMPARE(Backend::standingsHeadersFor("basketball"),
                 QStringList({"W", "L", "Pct", "GB"}));
        QVERIFY(Backend::standingsHeadersFor("tennis").isEmpty());
    }

    void parseStandingsKeepsConferences()
    {
        QString error;
        const QVariantList groups = Backend::parseStandings(basketballStandings,
                                                            "basketball", &error);
        QVERIFY(error.isEmpty());
        QCOMPARE(groups.size(), 2);
        QCOMPARE(groups.at(0).toMap().value("name").toString(),
                 QStringLiteral("Eastern Conference"));
        QCOMPARE(groups.at(1).toMap().value("name").toString(),
                 QStringLiteral("Western Conference"));

        const QVariantList rows = groups.at(0).toMap().value("rows").toList();
        QCOMPARE(rows.first().toMap().value("team").toMap().value("name").toString(),
                 QStringLiteral("Boston Celtics"));
        QCOMPARE(rows.first().toMap().value("cells").toStringList(),
                 QStringList({"6", "2", ".750", "-"}));
    }

    void standingsSeasonFollowsSportFamily()
    {
        // Rugby : année de FIN de saison (2026-27 => 2027).
        QCOMPARE(Backend::standingsSeason("rugby", QDate(2026, 11, 10)), 2027);
        QCOMPARE(Backend::standingsSeason("rugby", QDate(2027, 5, 1)), 2027);
        // Football (soccer) : année de DÉBUT de saison (2026-27 => 2026).
        QCOMPARE(Backend::standingsSeason("soccer", QDate(2026, 11, 10)), 2026);
        QCOMPARE(Backend::standingsSeason("soccer", QDate(2027, 4, 1)), 2026);
        // NBA/NHL (basketball/hockey) : année de FIN de saison.
        QCOMPARE(Backend::standingsSeason("basketball", QDate(2026, 11, 10)), 2027);
        QCOMPARE(Backend::standingsSeason("hockey", QDate(2026, 11, 10)), 2027);

        const QUrl url = Backend::standingsUrl("rugby", "270559", QDate(2026, 11, 10));
        QCOMPARE(url.path(), QStringLiteral("/apis/v2/sports/rugby/270559/standings"));
        QCOMPARE(QUrlQuery(url).queryItemValue("season"), QStringLiteral("2027"));
    }

    void standingsSeasonRejectsCompletedRugbyTable()
    {
        QByteArray teams2026;
        QByteArray teams2027;
        for (int i = 1; i <= 14; ++i) {
            if (i > 1) { teams2026 += ','; teams2027 += ','; }
            teams2026 += QByteArray("{\"team\":{\"displayName\":\"T%1\"},\"stats\":["
                                    "{\"name\":\"gamesPlayed\",\"displayValue\":\"26\"},"
                                    "{\"name\":\"gamesWon\",\"displayValue\":\"13\"},"
                                    "{\"name\":\"gamesDrawn\",\"displayValue\":\"0\"},"
                                    "{\"name\":\"gamesLost\",\"displayValue\":\"13\"},"
                                    "{\"name\":\"points\",\"displayValue\":\"50\"},"
                                    "{\"name\":\"rank\",\"value\":%1}]}")
                             .replace("%1", QByteArray::number(i));
            teams2027 += QByteArray("{\"team\":{\"displayName\":\"T%1\"},\"stats\":["
                                    "{\"name\":\"gamesPlayed\",\"displayValue\":\"9\"},"
                                    "{\"name\":\"gamesWon\",\"displayValue\":\"5\"},"
                                    "{\"name\":\"gamesDrawn\",\"displayValue\":\"0\"},"
                                    "{\"name\":\"gamesLost\",\"displayValue\":\"4\"},"
                                    "{\"name\":\"points\",\"displayValue\":\"20\"},"
                                    "{\"name\":\"rank\",\"value\":%1}]}")
                             .replace("%1", QByteArray::number(i));
        }
        const QByteArray payload2026 = "{\"standings\":{\"entries\":[" + teams2026 + "]}}";
        const QByteArray payload2027 = "{\"standings\":{\"entries\":[" + teams2027 + "]}}";

        // Scoreboard de Top 14 indiquant 2026 alors que la saison en cours est 2027.
        const QByteArray scoreboard = R"({"leagues":[{"season":{"year":2026}}],"events":[{"status":{"type":{"state":"pre"}}}]})";
        const QList<int> candidates = Backend::standingsSeasonCandidates(
            "rugby", QDate(2026, 11, 10), Backend::scoreboardSeasonYear(scoreboard));
        QMap<int, QByteArray> payloads;
        payloads.insert(2026, payload2026);
        payloads.insert(2027, payload2027);

        const QVariantMap chosen = Backend::chooseStandingsSeason(
            "rugby", candidates, payloads,
            Backend::scoreboardIndicatesSeasonInProgress(scoreboard));
        QCOMPARE(chosen.value("season").toInt(), 2027);
        QVERIFY(!chosen.value("uncertain").toBool());
        const QVariantList groups = chosen.value("groups").toList();
        QCOMPARE(groups.size(), 1);
        const QStringList firstCells =
            groups.first().toMap().value("rows").toList().first().toMap()
                  .value("cells").toStringList();
        QCOMPARE(firstCells.value(0), QStringLiteral("20"));
        QCOMPARE(firstCells.value(1), QStringLiteral("9"));
    }

    void standingsSeasonRejectsCompletedBrasileiraoTable()
    {
        QByteArray teams2025;
        QByteArray teams2026;
        for (int i = 1; i <= 20; ++i) {
            if (i > 1) { teams2025 += ','; teams2026 += ','; }
            teams2025 += QByteArray("{\"team\":{\"displayName\":\"B%1\"},\"stats\":["
                                    "{\"name\":\"gamesPlayed\",\"displayValue\":\"38\"},"
                                    "{\"name\":\"wins\",\"displayValue\":\"20\"},"
                                    "{\"name\":\"ties\",\"displayValue\":\"9\"},"
                                    "{\"name\":\"losses\",\"displayValue\":\"9\"},"
                                    "{\"name\":\"pointDifferential\",\"displayValue\":\"+10\"},"
                                    "{\"name\":\"points\",\"displayValue\":\"69\"},"
                                    "{\"name\":\"rank\",\"value\":%1}]}")
                             .replace("%1", QByteArray::number(i));
            teams2026 += QByteArray("{\"team\":{\"displayName\":\"B%1\"},\"stats\":["
                                    "{\"name\":\"gamesPlayed\",\"displayValue\":\"34\"},"
                                    "{\"name\":\"wins\",\"displayValue\":\"18\"},"
                                    "{\"name\":\"ties\",\"displayValue\":\"8\"},"
                                    "{\"name\":\"losses\",\"displayValue\":\"8\"},"
                                    "{\"name\":\"pointDifferential\",\"displayValue\":\"+9\"},"
                                    "{\"name\":\"points\",\"displayValue\":\"62\"},"
                                    "{\"name\":\"rank\",\"value\":%1}]}")
                             .replace("%1", QByteArray::number(i));
        }
        const QByteArray payload2025 = "{\"standings\":{\"entries\":[" + teams2025 + "]}}";
        const QByteArray payload2026 = "{\"standings\":{\"entries\":[" + teams2026 + "]}}";

        const QByteArray scoreboard = R"({"leagues":[{"season":{"year":2026}}],"events":[{"status":{"type":{"state":"in"}}}]})";
        const QList<int> candidates = Backend::standingsSeasonCandidates(
            "soccer", QDate(2026, 3, 15), Backend::scoreboardSeasonYear(scoreboard));
        QMap<int, QByteArray> payloads;
        payloads.insert(2025, payload2025);
        payloads.insert(2026, payload2026);

        const QVariantMap chosen = Backend::chooseStandingsSeason(
            "soccer", candidates, payloads,
            Backend::scoreboardIndicatesSeasonInProgress(scoreboard));
        QCOMPARE(chosen.value("season").toInt(), 2026);
        QVERIFY(!chosen.value("uncertain").toBool());
    }

    void standingsSeasonKeepsLigue1FirstCandidate()
    {
        QByteArray teams;
        for (int i = 1; i <= 18; ++i) {
            if (i > 1) teams += ',';
            teams += QByteArray("{\"team\":{\"displayName\":\"L%1\"},\"stats\":["
                                "{\"name\":\"gamesPlayed\",\"displayValue\":\"9\"},"
                                "{\"name\":\"wins\",\"displayValue\":\"5\"},"
                                "{\"name\":\"ties\",\"displayValue\":\"2\"},"
                                "{\"name\":\"losses\",\"displayValue\":\"2\"},"
                                "{\"name\":\"pointDifferential\",\"displayValue\":\"+3\"},"
                                "{\"name\":\"points\",\"displayValue\":\"17\"},"
                                "{\"name\":\"rank\",\"value\":%1}]}")
                         .replace("%1", QByteArray::number(i));
        }
        const QByteArray payload = "{\"standings\":{\"entries\":[" + teams + "]}}";
        const QByteArray scoreboard = R"({"leagues":[{"season":{"year":2026}}],"events":[{"status":{"type":{"state":"pre"}}}]})";
        const QList<int> candidates = Backend::standingsSeasonCandidates(
            "soccer", QDate(2026, 11, 10), Backend::scoreboardSeasonYear(scoreboard));
        QVERIFY(!candidates.isEmpty());
        QCOMPARE(candidates.first(), 2026);

        QMap<int, QByteArray> payloads;
        payloads.insert(2026, payload);
        const QVariantMap chosen = Backend::chooseStandingsSeason(
            "soccer", candidates, payloads,
            Backend::scoreboardIndicatesSeasonInProgress(scoreboard));
        QCOMPARE(chosen.value("season").toInt(), 2026);
        QVERIFY(!chosen.value("uncertain").toBool());
    }

    void standingsSeasonExposesUncertainState()
    {
        // Aucune candidate concluante : tables vides pour toutes les saisons.
        const QByteArray emptyPayload = "{\"standings\":{\"entries\":[]}}";
        const QList<int> candidates = Backend::standingsSeasonCandidates(
            "soccer", QDate(2026, 11, 10), 2026);
        QMap<int, QByteArray> payloads;
        for (int season : candidates)
            payloads.insert(season, emptyPayload);

        const QVariantMap chosen = Backend::chooseStandingsSeason(
            "soccer", candidates, payloads, true);
        QVERIFY(chosen.value("uncertain").toBool());
        QCOMPARE(chosen.value("season").toInt(), candidates.first());
        QVERIFY(chosen.value("groups").toList().isEmpty());
    }

    void standingsPointsComeFirstFromRecordedPayload()
    {
        const QByteArray recorded = R"({
          "standings": {"entries": [
            {
              "team": {"displayName": "Stade Toulousain", "abbreviation": "TLS",
                       "color": "000000", "logo": "tls.png"},
              "stats": [
                {"name": "gamesPlayed", "displayValue": "9"},
                {"name": "wins", "displayValue": "7"},
                {"name": "ties", "displayValue": "0"},
                {"name": "losses", "displayValue": "2"},
                {"name": "pointDifferential", "displayValue": "+68"},
                {"name": "points", "displayValue": "32"},
                {"name": "rank", "value": 1}
              ]
            },
            {
              "team": {"displayName": "Other", "abbreviation": "OTH"},
              "stats": [
                {"name": "gamesPlayed", "displayValue": "9"},
                {"name": "wins", "displayValue": "1"},
                {"name": "ties", "displayValue": "0"},
                {"name": "losses", "displayValue": "8"},
                {"name": "pointDifferential", "displayValue": "-10"},
                {"name": "points", "displayValue": "5"},
                {"name": "rank", "value": 2}
              ]
            }
          ]}
        })";
        QString error;
        const QVariantList groups = Backend::parseStandings(recorded, "soccer", &error);
        QVERIFY(error.isEmpty());
        QCOMPARE(Backend::standingsHeadersFor("soccer").first(), QStringLiteral("Pts"));
        QCOMPARE(Backend::standingsHeadersFor("soccer").value(1), QStringLiteral("MP"));
        const QStringList cells = groups.first().toMap().value("rows").toList().first()
                                      .toMap().value("cells").toStringList();
        QCOMPARE(cells.value(0), QStringLiteral("32"));
        QCOMPARE(cells.value(1), QStringLiteral("9"));
    }

    void standingsModeCoexistsWithLive()
    {
        QTemporaryDir dir;
        Backend backend(dir.filePath("settings.ini"), false);
        QCOMPARE(backend.standingsHeaders().size(), 6); // soccer by default

        backend.setStandings(true);
        QVERIFY(backend.standingsVisible());
        QCOMPARE(backend.status(), QStringLiteral("idle"));

        backend.setLive(true);
        QVERIFY(!backend.standingsVisible());
        QVERIFY(backend.liveMode());

        backend.setStandings(true);
        QVERIFY(!backend.liveMode());
        QVERIFY(backend.standingsVisible());

        // Browsing leagues keeps the standings view.
        backend.selectLeague(1);
        QVERIFY(backend.standingsVisible());

        backend.setStandings(false);
        QVERIFY(!backend.standingsVisible());
    }

    void scoreboardReadsGoalsAndCards()
    {
        QString error;
        const QVariantList events = Backend::parseScoreboard(soccerMatchPayload, &error);
        QVERIFY(error.isEmpty());
        QCOMPARE(events.size(), 1);

        const QVariantMap event = events.first().toMap();
        const QVariantList teams = event.value("teams").toList();
        QCOMPARE(teams.size(), 2);
        // Domicile (Toulouse) en premier, extérieur (Strasbourg) ensuite.
        QCOMPARE(teams.first().toMap().value("id").toString(), QStringLiteral("179"));
        QCOMPARE(teams.first().toMap().value("abbreviation").toString(),
                 QStringLiteral("TLS"));

        const QVariantList goals = event.value("goals").toList();
        QCOMPARE(goals.size(), 4);
        QCOMPARE(goals.at(0).toMap().value("scorer").toString(),
                 QStringLiteral("Santiago Hidalgo"));
        QCOMPARE(goals.at(0).toMap().value("teamAbbreviation").toString(),
                 QStringLiteral("TLS"));
        QCOMPARE(goals.at(0).toMap().value("clock").toString(),
                 QStringLiteral("45'+1'"));
        QCOMPARE(goals.at(2).toMap().value("teamAbbreviation").toString(),
                 QStringLiteral("STR"));

        const QVariantList cards = event.value("cards").toList();
        QCOMPARE(cards.size(), 1);
        QCOMPARE(cards.first().toMap().value("player").toString(),
                 QStringLiteral("Karim Coulibaly"));
        QVERIFY(!cards.first().toMap().value("red").toBool());
    }

    void matchSummaryReadsGoalsAndStats()
    {
        QString error;
        const QVariantList events = Backend::parseScoreboard(soccerMatchPayload, &error);
        QVERIFY(error.isEmpty());

        const QVariantMap parsed = Backend::parseMatchSummary(matchSummaryPayload,
                                                              events.first().toMap(), &error);
        QVERIFY(error.isEmpty());

        const QVariantList goals = parsed.value("goals").toList();
        QCOMPARE(goals.size(), 2);
        QCOMPARE(goals.at(0).toMap().value("scorer").toString(),
                 QStringLiteral("Santiago Hidalgo"));
        QCOMPARE(goals.at(0).toMap().value("assist").toString(),
                 QStringLiteral("Sion Oppong"));
        QCOMPARE(goals.at(0).toMap().value("teamAbbreviation").toString(),
                 QStringLiteral("TLS"));
        QCOMPARE(goals.at(1).toMap().value("teamAbbreviation").toString(),
                 QStringLiteral("STR"));

        const QVariantList cards = parsed.value("cards").toList();
        QCOMPARE(cards.size(), 1);
        QCOMPARE(cards.first().toMap().value("player").toString(),
                 QStringLiteral("Karim Coulibaly"));

        const QVariantList stats = parsed.value("stats").toList();
        QCOMPARE(stats.size(), 9);
        QCOMPARE(stats.first().toMap().value("label").toString(),
                 QStringLiteral("Possession"));
        QCOMPARE(stats.first().toMap().value("home").toString(),
                 QStringLiteral("47.3%"));
        QCOMPARE(stats.first().toMap().value("away").toString(),
                 QStringLiteral("52.7%"));
    }

    void leaguesStartEnabledAndPersist()
    {
        QTemporaryDir dir;
        const QString settings = dir.filePath("settings.ini");

        {
            Backend backend(settings, false);
            QCOMPARE(backend.leagues().size(), 34);
            QCOMPARE(backend.enabledLeagues().size(), 34);
            QCOMPARE(backend.settingsRows().size(), 47);
            QVERIFY(backend.enabledLeagues().contains("soccer/eng.1"));

            backend.setLeagueEnabled("soccer", "eng.1", false);
            QVERIFY(!backend.enabledLeagues().contains("soccer/eng.1"));
        }

        Backend reopened(settings, false);
        QVERIFY(!reopened.enabledLeagues().contains("soccer/eng.1"));
        QVERIFY(reopened.enabledLeagues().contains("soccer/esp.1"));
        QCOMPARE(reopened.enabledLeagues().size(), 33);
    }

    void newLeaguesStartEnabledAfterAnUpgrade()
    {
        QTemporaryDir dir;
        const QString settings = dir.filePath("settings.ini");

        QStringList enabled;
        QStringList known;
        for (const auto &value : Backend::leagueCatalog()) {
            const QVariantMap entry = value.toMap();
            const QString key = entry.value("sport").toString() + "/"
                                + entry.value("league").toString();
            if (key != "rugby/270559")
                known.append(key); // the previous release did not know Top 14
            if (key != "rugby/270559" && key != "soccer/eng.1")
                enabled.append(key);
        }

        {
            QSettings store(settings, QSettings::IniFormat);
            store.setValue("leagues/enabled", enabled);
            store.setValue("leagues/known", known);
            store.sync();
        }

        Backend backend(settings, false);
        QVERIFY(backend.enabledLeagues().contains("rugby/270559"));
        QVERIFY(!backend.enabledLeagues().contains("soccer/eng.1"));
        QCOMPARE(backend.enabledLeagues().size(), 33);
    }

    void hidingTheCurrentLeaguePicksAnother()
    {
        QTemporaryDir dir;
        Backend backend(dir.filePath("settings.ini"), false);
        QCOMPARE(backend.league().value("league").toString(), QStringLiteral("eng.1"));

        backend.setLeagueEnabled("soccer", "eng.1", false);
        QCOMPARE(backend.league().value("league").toString(), QStringLiteral("esp.1"));
        QVERIFY(backend.enabledLeagues().contains(
            backend.league().value("sport").toString() + "/"
            + backend.league().value("league").toString()));

        // A hidden league cannot be selected from the list.
        const int previous = backend.leagueIndex();
        backend.selectLeague(0);
        QCOMPARE(backend.leagueIndex(), previous);
    }

    void hidingASportHidesItsLeagues()
    {
        QTemporaryDir dir;
        Backend backend(dir.filePath("settings.ini"), false);
        backend.setSportLeaguesEnabled("soccer", false);

        QCOMPARE(backend.enabledLeagues().size(), 21);
        QCOMPARE(backend.league().value("sport").toString(), QStringLiteral("football"));
        QCOMPARE(backend.league().value("league").toString(), QStringLiteral("nfl"));

        bool soccerRow = false;
        for (const auto &value : backend.settingsRows()) {
            const QVariantMap row = value.toMap();
            if (row.value("type").toString() != "sport"
                || row.value("sport").toString() != "soccer")
                continue;
            soccerRow = true;
            QCOMPARE(row.value("leagues").toInt(), 13);
            QCOMPARE(row.value("enabledCount").toInt(), 0);
            QCOMPARE(row.value("allEnabled").toBool(), false);
        }
        QVERIFY(soccerRow);

        backend.setSportLeaguesEnabled("soccer", true);
        QCOMPARE(backend.enabledLeagues().size(), 34);
    }

    void hidingEveryLeagueLeavesTheBoardEmpty()
    {
        QTemporaryDir dir;
        const QString settings = dir.filePath("settings.ini");

        {
            Backend backend(settings, false);
            backend.setAllLeaguesEnabled(false);
            QVERIFY(backend.enabledLeagues().isEmpty());
            QVERIFY(backend.events().isEmpty());
            QCOMPARE(backend.status(), QStringLiteral("ready"));
        }

        // "None" is a choice, not an empty setting that reverts to all.
        Backend reopened(settings, false);
        QVERIFY(reopened.enabledLeagues().isEmpty());

        reopened.setAllLeaguesEnabled(true);
        QCOMPARE(reopened.enabledLeagues().size(), 34);
    }

    void olderSportChoiceSeedsLeagues()
    {
        QTemporaryDir dir;
        const QString settings = dir.filePath("settings.ini");
        {
            QSettings store(settings, QSettings::IniFormat);
            store.setValue("sports/enabled", QStringList{"basketball"});
            store.sync();
        }

        {
            Backend backend(settings, false);
            QCOMPARE(backend.enabledLeagues().size(), 3);
            QVERIFY(backend.enabledLeagues().contains("basketball/nba"));
            QCOMPARE(backend.league().value("sport").toString(), QStringLiteral("basketball"));
        }

        QSettings store(settings, QSettings::IniFormat);
        QVERIFY(!store.contains("sports/enabled"));
        QVERIFY(store.contains("leagues/enabled"));

        Backend reopened(settings, false);
        QCOMPARE(reopened.enabledLeagues().size(), 3);
    }

    void liveModeTogglesOffline()
    {
        QTemporaryDir dir;
        Backend backend(dir.filePath("settings.ini"), false);
        QVERIFY(!backend.liveMode());

        backend.setLive(true);
        QVERIFY(backend.liveMode());
        QCOMPARE(backend.status(), QStringLiteral("idle"));

        backend.toggleLive();
        QVERIFY(!backend.liveMode());
    }

    void recordLabelsFollowTheLanguage()
    {
        QTemporaryDir dir;
        writeFile(dir.filePath("language"), "fr\n");
        Translator translator(dir.filePath("language"), ":/i18n");
        QCoreApplication::installTranslator(&translator);
        struct Remover {
            Translator *translator;
            ~Remover() { QCoreApplication::removeTranslator(translator); }
        } remover{&translator};

        Backend backend(dir.filePath("settings.ini"), false);
        QCOMPARE(backend.recordLabel("soccer", "3-1-5"), QStringLiteral("3V · 1N · 5D"));
        QCOMPARE(backend.recordLabel("hockey", "11-3-3"), QStringLiteral("11V · 3D · 3OT"));
        QCOMPARE(backend.recordLabel("football", "6-2-1"), QStringLiteral("6V · 2D · 1N"));
        QCOMPARE(backend.recordLabel("basketball", "4-4"), QStringLiteral("4V · 4D"));
        QCOMPARE(backend.recordLabel("rugby", "LWWWW"), QStringLiteral("Forme : D V V V V"));
    }

    void navigationMovesByDay()
    {
        QTemporaryDir dir;
        Backend backend(dir.filePath("settings.ini"), false);

        QVERIFY(backend.isToday());
        const QString today = backend.dateLabel();
        QVERIFY(!today.isEmpty());

        backend.nextDay();
        QVERIFY(!backend.isToday());
        QCOMPARE(backend.date(), QDate::currentDate().addDays(1));
        QCOMPARE(backend.dateLabel(), QLocale().toString(QDate::currentDate().addDays(1), "ddd d MMM yyyy"));

        backend.previousDay();
        QVERIFY(backend.isToday());

        backend.previousDay();
        QVERIFY(!backend.isToday());
        backend.today();
        QVERIFY(backend.isToday());
    }

    void selectionPersistsAcrossLaunches()
    {
        QTemporaryDir dir;
        const QString settings = dir.filePath("settings.ini");

        {
            Backend backend(settings, false);
            backend.selectLeague(5);
            QCOMPARE(backend.leagueIndex(), 5);
            QCOMPARE(backend.league().value("league").toString(), QStringLiteral("uefa.champions"));
        }

        Backend reopened(settings, false);
        QCOMPARE(reopened.leagueIndex(), 5);
        QCOMPARE(reopened.league().value("name").toString(), QStringLiteral("Champions League"));
    }

    void unknownSavedLeagueFallsBackToTheFirst()
    {
        QTemporaryDir dir;
        const QString settings = dir.filePath("settings.ini");

        QSettings store(settings, QSettings::IniFormat);
        store.setValue("league/sport", "curling");
        store.setValue("league/id", "nowhere");
        store.sync();

        Backend backend(settings, false);
        QCOMPARE(backend.leagueIndex(), 0);
    }

    void selectingTheSameLeagueDoesNotClearTheBoard()
    {
        QTemporaryDir dir;
        Backend backend(dir.filePath("settings.ini"), false);
        backend.selectLeague(3);
        QCOMPARE(backend.leagueIndex(), 3);
        backend.selectLeague(3);
        QCOMPARE(backend.leagueIndex(), 3);
        QCOMPARE(backend.status(), QStringLiteral("idle"));
    }

    void themeFallsBack()
    {
        QTemporaryDir dir;
        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#FFD60A"));
        QCOMPARE(theme.accentForeground(), QString("black"));
        QCOMPARE(theme.background(), QString("#0e0e10"));
        QCOMPARE(theme.foreground(), QString("#f2f2f4"));
        QCOMPARE(theme.dark(), true);
    }

    void themeReadsAccent()
    {
        QTemporaryDir dir;
        QDir(dir.path()).mkpath("theme");
        writeFile(dir.path() + "/theme/colors.toml", "accent = \"#112233\"\n");

        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#112233"));
        QCOMPARE(theme.accentForeground(), QString("white"));
    }

    void themeReadsTheWholePalette()
    {
        QTemporaryDir dir;
        QDir(dir.path()).mkpath("theme");
        writeFile(dir.path() + "/theme/colors.toml",
                  "mode = \"light\"\n"
                  "accent = \"#1e66f5\"\n"
                  "selection = \"#ccd0da\"\n"
                  "muted = \"#acb0be\"\n"
                  "background = \"#eff1f5\"\n"
                  "dark_background = \"#e3e4e8\"\n"
                  "darker_background = \"#d7d8dc\"\n"
                  "lighter_background = \"#dce0e8\"\n"
                  "foreground = \"#4c4f69\"\n"
                  "red = \"#d20f39\"\n"
                  "hyprland_active_border = \"rgba(1ee) rgba(2ee) 45deg\"\n");

        Theme theme(dir.path());
        QCOMPARE(theme.dark(), false);
        QCOMPARE(theme.accent(), QString("#1e66f5"));
        QCOMPARE(theme.background(), QString("#eff1f5"));
        QCOMPARE(theme.panel(), QString("#e3e4e8"));
        QCOMPARE(theme.card(), QString("#dce0e8"));
        QCOMPARE(theme.foreground(), QString("#4c4f69"));
        QCOMPARE(theme.selection(), QString("#ccd0da"));
        QCOMPARE(theme.danger(), QString("#d20f39"));
        QCOMPARE(theme.accentForeground(), QString("white"));
    }

    void themeFollowsHyprlandRounding()
    {
        QTemporaryDir dir;
        const QString response = dir.filePath("response.json"), command = dir.filePath("hyprctl");
        writeFile(command, "#!/bin/sh\ncat '" + response.toUtf8() + "'\n");
        QVERIFY(QFile::setPermissions(command, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        writeFile(response, "{\"int\":0}");

        Theme theme(dir.path(), nullptr, command);
        QCOMPARE(theme.radius(), 0);
        writeFile(response, "{\"int\":6}");
        QTRY_COMPARE(theme.radius(), 6);
    }

    void translatorFallsBackToEnglish()
    {
        QTemporaryDir dir;
        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("en"));
        QVERIFY(translator.translate("Main", "Save").isNull());
    }

    void translatorFollowsTheLanguageFile()
    {
        QTemporaryDir dir;
        writeFile(dir.path() + "/language", "pt-BR\n");
        writeFile(dir.path() + "/pt.json", R"({"Save": "Guardar", "Open": "Abrir"})");
        writeFile(dir.path() + "/pt-BR.json", R"({"Save": "Salvar"})");

        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("pt-BR"));
        QCOMPARE(translator.translate("Main", "Save"), QString("Salvar"));
        QCOMPARE(translator.translate("Main", "Open"), QString("Abrir"));
        QVERIFY(translator.translate("Main", "Quit").isNull());
    }

    void translatorPicksPluralForms()
    {
        QTemporaryDir dir;
        writeFile(dir.path() + "/language", "ru\n");
        writeFile(dir.path() + "/en.json", R"({"%n clips": {"one": "%n clip", "other": "%n clips"}})");
        writeFile(dir.path() + "/ru.json", R"({"%n clips": {"one": "%n клип", "few": "%n клипа", "many": "%n клипов", "other": "%n клипа"}})");

        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 21), QString("%n клип"));
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 22), QString("%n клипа"));
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 25), QString("%n клипов"));
        QCOMPARE(Translator::pluralCategory("ar", 11), QString("many"));
        QCOMPARE(Translator::pluralCategory("ja", 1), QString("other"));
        QCOMPARE(Translator::pluralCategory("fr", 1000000), QString("many"));
    }

    void checkScriptValidatesTheProject()
    {
        QFile file(QStringLiteral(SRCDIR "/../check.sh"));
        if (!file.exists())
            file.setFileName(QStringLiteral("check.sh"));
        QVERIFY2(file.exists(), "check.sh manquant");
        QVERIFY2(file.permissions() & QFile::ExeUser, "check.sh doit être exécutable");
        QVERIFY2(file.open(QIODevice::ReadOnly | QIODevice::Text), "check.sh illisible");
        const QString content = QString::fromUtf8(file.readAll());
        file.close();
        QVERIFY(content.contains("# shellcheck enable=all"));
        QVERIFY(content.contains("shellcheck"));
        QVERIFY(content.contains("bin/build"));
        QVERIFY(content.contains("bin/test"));
        QVERIFY(content.contains("QT_QPA_PLATFORM=offscreen"));
    }

    void catalogsHaveTheSameKeys()
    {
        QFile french(":/i18n/fr.json");
        QVERIFY(french.open(QIODevice::ReadOnly));
        const QJsonObject translations = QJsonDocument::fromJson(french.readAll()).object();
        QVERIFY(translations.contains("Final"));
        QVERIFY(translations.contains("No games on this day."));
        QVERIFY(translations.contains("Soccer"));
        QVERIFY(translations.value("%n games").isObject());
    }
};

QTEST_MAIN(OmASportsTests)
#include "omasports_tests.moc"
