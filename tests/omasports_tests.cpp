#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
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

        const QVariantMap away = event.value("teams").toList().first().toMap();
        const QVariantMap home = event.value("teams").toList().last().toMap();
        // The home side comes first in the feed; the card wants away first.
        QCOMPARE(away.value("name").toString(), QStringLiteral("Orlando Magic"));
        QCOMPARE(away.value("score").toString(), QStringLiteral("56"));
        QCOMPARE(away.value("record").toString(), QStringLiteral("3-4"));
        QCOMPARE(away.value("logo").toString(), QStringLiteral("orl.png"));
        QCOMPARE(home.value("name").toString(), QStringLiteral("Phoenix Suns"));
        QCOMPARE(home.value("score").toString(), QStringLiteral("58"));
        QVERIFY(home.value("winner").toBool());

        QCOMPARE(Backend::liveCountOf(events), 1);
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

        const QVariantList live = Backend::liveEventsFrom(scoreboard, "basketball",
                                                          "Basketball", "NBA");
        QCOMPARE(live.size(), 1);
        const QVariantMap event = live.first().toMap();
        QCOMPARE(event.value("sport").toString(), QStringLiteral("basketball"));
        QCOMPARE(event.value("leagueName").toString(), QStringLiteral("NBA"));
        QCOMPARE(event.value("sectionLabel").toString(), QStringLiteral("Basketball · NBA"));
        QCOMPARE(Backend::liveCountOf(live), 1);

        // A schedule with nothing in progress contributes no live events.
        const QVariantList upcoming = Backend::parseScoreboard(tournamentPayload, &error);
        QVERIFY(error.isEmpty());
        QVERIFY(Backend::liveEventsFrom(upcoming, "golf", "Golf", "PGA Tour").isEmpty());
    }

    void leaguesStartEnabledAndPersist()
    {
        QTemporaryDir dir;
        const QString settings = dir.filePath("settings.ini");

        {
            Backend backend(settings, false);
            QCOMPARE(backend.leagues().size(), 33);
            QCOMPARE(backend.enabledLeagues().size(), 33);
            QCOMPARE(backend.settingsRows().size(), 46);
            QVERIFY(backend.enabledLeagues().contains("soccer/eng.1"));

            backend.setLeagueEnabled("soccer", "eng.1", false);
            QVERIFY(!backend.enabledLeagues().contains("soccer/eng.1"));
        }

        Backend reopened(settings, false);
        QVERIFY(!reopened.enabledLeagues().contains("soccer/eng.1"));
        QVERIFY(reopened.enabledLeagues().contains("soccer/esp.1"));
        QCOMPARE(reopened.enabledLeagues().size(), 32);
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

        QCOMPARE(backend.enabledLeagues().size(), 20);
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
        QCOMPARE(backend.enabledLeagues().size(), 33);
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
        QCOMPARE(reopened.enabledLeagues().size(), 33);
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
