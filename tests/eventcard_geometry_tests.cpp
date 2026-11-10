#include <QtTest>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QImage>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QVariantList>
#include <QVariantMap>

class StubTheme : public QObject {
    Q_OBJECT
    Q_PROPERTY(int radius MEMBER m_radius CONSTANT)
    Q_PROPERTY(QString card MEMBER m_card CONSTANT)
    Q_PROPERTY(QString foreground MEMBER m_foreground CONSTANT)
    Q_PROPERTY(QString accent MEMBER m_accent CONSTANT)
    Q_PROPERTY(QString mutedForeground MEMBER m_mutedForeground CONSTANT)
    Q_PROPERTY(QString selection MEMBER m_selection CONSTANT)
    Q_PROPERTY(QString monoFamily MEMBER m_monoFamily CONSTANT)
public:
    int m_radius = 8;
    QString m_card = "#1a1a1f";
    QString m_foreground = "#f2f2f4";
    QString m_accent = "#FFD60A";
    QString m_mutedForeground = "#888888";
    QString m_selection = "#2a2a31";
    QString m_monoFamily = QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
};

class StubBackend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString dateIso MEMBER m_dateIso CONSTANT)
    Q_PROPERTY(QVariantMap league READ league CONSTANT)
public:
    QString m_dateIso = "2026-11-10";
    QVariantMap league() const
    {
        return {{"sport", "soccer"}, {"league", "fra.1"}, {"name", "Ligue 1"}};
    }
    Q_INVOKABLE QString recordLabel(const QString &, const QString &record) const { return record; }
};

class StubTranslator : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString language MEMBER m_language CONSTANT)
public:
    QString m_language = "en";
};

class EventCardGeometryTests : public QObject {
    Q_OBJECT

private:
    QVariantMap team(const QString &name, const QString &score, const QString &record,
                     const QString &logo) const
    {
        return {{"name", name},
                {"abbreviation", name.left(3).toUpper()},
                {"score", score},
                {"record", record},
                {"logo", logo},
                {"color", "123456"},
                {"winner", false}};
    }

    QVariantMap model(const QString &homeName, const QString &awayName, const QString &homeRecord,
                      const QString &awayRecord, const QString &homeScore, const QString &awayScore,
                      const QString &logo, const QVariantList &goals) const
    {
        return {{"name", homeName + " v " + awayName},
                {"state", "post"},
                {"shortDetail", "FT"},
                {"detail", "Full Time"},
                {"start", "2026-11-10T20:00:00Z"},
                {"sport", "soccer"},
                {"venue", "Stadium"},
                {"goals", goals},
                {"teams", QVariantList{team(homeName, homeScore, homeRecord, logo),
                                       team(awayName, awayScore, awayRecord, logo)}}};
    }

    QVariantMap model(const QString &homeName, const QString &awayName, const QString &homeRecord,
                      const QString &awayRecord, const QString &logo, const QVariantList &goals) const
    {
        return model(homeName, awayName, homeRecord, awayRecord, "2", "1", logo, goals);
    }

    QVariantMap model(const QString &homeName, const QString &awayName, const QString &record,
                      const QString &logo, const QVariantList &goals) const
    {
        return model(homeName, awayName, record, record, logo, goals);
    }

    struct RowGeom {
        qreal scoreX = -1;
        qreal scoreY = -1;
        qreal scoreWidth = -1;
        qreal nameY = -1;
        qreal scoreBaselineY = -1;
        qreal nameBaselineY = -1;
    };

    QList<RowGeom> measure(QQuickItem *card) const
    {
        QList<RowGeom> rows;
        QObject *repeater = nullptr;
        for (auto *o : card->findChildren<QObject *>()) {
            if (QString(o->metaObject()->className()).contains("Repeater")) {
                repeater = o;
                break;
            }
        }
        if (!repeater)
            return rows;
        const int count = repeater->property("count").toInt();
        for (int i = 0; i < count; ++i) {
            QQuickItem *item = nullptr;
            QMetaObject::invokeMethod(repeater, "itemAt", Q_RETURN_ARG(QQuickItem *, item),
                                      Q_ARG(int, i));
            if (!item)
                continue;
            const auto names = item->findChildren<QQuickItem *>("nameText");
            const auto scores = item->findChildren<QQuickItem *>("scoreText");
            if (names.isEmpty() || scores.isEmpty())
                continue;
            RowGeom row;
            row.scoreX = scores.first()->mapToItem(card, 0, 0).x();
            row.scoreY = scores.first()->mapToItem(card, 0, 0).y();
            row.scoreWidth = scores.first()->width();
            row.nameY = names.first()->mapToItem(card, 0, 0).y();
            row.scoreBaselineY = row.scoreY + scores.first()->property("baselineOffset").toReal();
            row.nameBaselineY = row.nameY + names.first()->property("baselineOffset").toReal();
            rows.append(row);
        }
        return rows;
    }

    void settle(QQuickView &view)
    {
        if (auto *root = view.rootObject())
            root->polish();
        QCoreApplication::processEvents();
        QTest::qWait(50);
        QCoreApplication::processEvents();
    }

    void saveAsymmetricSnapshot(QQuickView &view);

private slots:
    void scoreGeometryIsStable();
    void asymmetricScoreGeometry();
    void threeDigitScoresAlignRightEdge();
};

void EventCardGeometryTests::saveAsymmetricSnapshot(QQuickView &view)
{
    if (!view.isVisible())
        return;
    const QImage image = view.grabWindow();
    if (image.isNull())
        return;
    image.save(QStringLiteral("/tmp/omas-card-asym.png"));
}

void EventCardGeometryTests::scoreGeometryIsStable()
{
    StubTheme theme;
    StubBackend backend;
    StubTranslator translator;

    const QVariantList noGoals;
    const QVariantList goals = QVariantList{
        QVariantMap{{"clock", "12'"}, {"scorer", "Scorer"}, {"teamAbbreviation", "HOM"},
                    {"ownGoal", false}, {"penalty", false}, {"type", "Goal"}}};
    const QVariantMap base = model("Home FC", "Away FC", "", "", noGoals);

    QQuickView view;
    view.engine()->rootContext()->setContextProperty("theme", &theme);
    view.engine()->rootContext()->setContextProperty("backend", &backend);
    view.engine()->rootContext()->setContextProperty("translator", &translator);
    view.setResizeMode(QQuickView::SizeViewToRootObject);
    view.setInitialProperties({{"modelData", base}, {"width", 420}});
    view.setSource(QUrl(QStringLiteral("qrc:/EventCard.qml")));
    QVERIFY2(view.status() == QQuickView::Ready,
             qPrintable(view.errors().isEmpty() ? QStringLiteral("load failed")
                                                : view.errors().first().toString()));

    auto *root = view.rootObject();
    QVERIFY(root);
    view.show();
    settle(view);
    const QList<RowGeom> baseRows = measure(root);
    QCOMPARE(baseRows.size(), 2);

    const QString crest = QStringLiteral("qrc:/crest.png");

    const QList<QVariantMap> configs = {
        model("Home FC", "Away FC", "3-1-5", "", noGoals), // avec/sans record
        model("Home FC", "Away FC", "", crest, noGoals),    // avec/sans badge/logo
        model("A very very very long team name that must elide",
              "Another extremely long visiting team name", "", "", noGoals),
        model("Home FC", "Away FC", "", "", goals), // présence de buteurs
    };

    for (int c = 0; c < configs.size(); ++c) {
        root->setProperty("modelData", configs.at(c));
        settle(view);
        const QList<RowGeom> rows = measure(root);
        QCOMPARE(rows.size(), 2);
        for (int i = 0; i < rows.size(); ++i) {
            QVERIFY2(qAbs(rows.at(i).scoreX - baseRows.at(i).scoreX) <= 1.0,
                     qPrintable(QString("config %1 row %2 scoreX %3 vs %4")
                                    .arg(c).arg(i).arg(rows.at(i).scoreX)
                                    .arg(baseRows.at(i).scoreX)));
            QVERIFY2(qAbs(rows.at(i).scoreBaselineY - rows.at(i).nameBaselineY) <= 1.0,
                     qPrintable(QString("config %1 row %2 scoreBaselineY %3 vs nameBaselineY %4")
                                    .arg(c).arg(i).arg(rows.at(i).scoreBaselineY)
                                    .arg(rows.at(i).nameBaselineY)));
        }
        if (c == 1) {
            int readyBadges = 0;
            QObject *repeater = nullptr;
            for (auto *o : root->findChildren<QObject *>()) {
                if (QString(o->metaObject()->className()).contains("Repeater")) {
                    repeater = o;
                    break;
                }
            }
            QVERIFY(repeater);
            for (int attempt = 0; attempt < 40 && readyBadges < 2; ++attempt) {
                readyBadges = 0;
                const int count = repeater->property("count").toInt();
                for (int i = 0; i < count; ++i) {
                    QQuickItem *item = nullptr;
                    QMetaObject::invokeMethod(repeater, "itemAt", Q_RETURN_ARG(QQuickItem *, item),
                                              Q_ARG(int, i));
                    if (!item)
                        continue;
                    for (auto *obj : item->findChildren<QObject *>()) {
                        const QVariant srcVar = obj->property("source");
                        if (!srcVar.isValid())
                            continue;
                        auto *img = qobject_cast<QQuickItem *>(obj);
                        if (srcVar.toString() == crest && img && img->width() > 0
                            && obj->property("status").toInt() == 1)
                            ++readyBadges;
                    }
                }
                if (readyBadges < 2) {
                    QCoreApplication::processEvents();
                    QTest::qWait(25);
                }
            }
            QVERIFY2(readyBadges >= 2,
                     qPrintable(QString("expected 2 rendered badges, got %1").arg(readyBadges)));
        }
    }
}

void EventCardGeometryTests::asymmetricScoreGeometry()
{
    StubTheme theme;
    StubBackend backend;
    StubTranslator translator;

    const QVariantList noGoals;
    const QVariantList goals = QVariantList{
        QVariantMap{{"clock", "12'"}, {"scorer", "Scorer"}, {"teamAbbreviation", "HOM"},
                    {"ownGoal", false}, {"penalty", false}, {"type", "Goal"}}};

    // Reproduction fidèle : ligne 2 seule avec record (430 px).
    const QVariantMap base = model("Borussia Moenchengladbach", "Wolverhampton Wanderers", "", "4-3-3",
                                   QString(), noGoals);

    QQuickView view;
    view.engine()->rootContext()->setContextProperty("theme", &theme);
    view.engine()->rootContext()->setContextProperty("backend", &backend);
    view.engine()->rootContext()->setContextProperty("translator", &translator);
    view.setResizeMode(QQuickView::SizeViewToRootObject);
    view.setInitialProperties({{"modelData", base}, {"width", 430}});
    view.setSource(QUrl(QStringLiteral("qrc:/EventCard.qml")));
    QVERIFY2(view.status() == QQuickView::Ready,
             qPrintable(view.errors().isEmpty() ? QStringLiteral("load failed")
                                                : view.errors().first().toString()));

    auto *root = view.rootObject();
    QVERIFY(root);
    view.show();
    settle(view);

    // Écrit à chaque exécution (cas « record sur une seule ligne »), sans échec si impossible.
    saveAsymmetricSnapshot(view);

    const QList<QPair<QString, QVariantMap>> configs = {
        {"record sur la ligne 2 seulement", base},
        {"record sur la ligne 1 seulement",
         model("Borussia Moenchengladbach", "Wolverhampton Wanderers", "4-3-3", "", QString(), noGoals)},
        {"hauteurs différentes (buteurs) et record asymétrique",
         model("Borussia Moenchengladbach", "Wolverhampton Wanderers", "", "4-3-3", QString(), goals)},
        {"hauteurs différentes (buteurs) et record sur la ligne 1",
         model("Borussia Moenchengladbach", "Wolverhampton Wanderers", "4-3-3", "", QString(), goals)},
    };

    for (const auto &cfg : configs) {
        root->setProperty("modelData", cfg.second);
        settle(view);
        const QList<RowGeom> rows = measure(root);
        QCOMPARE(rows.size(), 2);

        const qreal rel0 = rows.at(0).scoreBaselineY - rows.at(0).nameBaselineY;
        const qreal rel1 = rows.at(1).scoreBaselineY - rows.at(1).nameBaselineY;

        for (int i = 0; i < rows.size(); ++i) {
            const qreal delta = rows.at(i).scoreBaselineY - rows.at(i).nameBaselineY;
            QVERIFY2(qAbs(delta) <= 1.0,
                     qPrintable(QString("%1: ligne %2 scoreBaselineY=%3 nameBaselineY=%4 (delta=%5)")
                                    .arg(cfg.first)
                                    .arg(i)
                                    .arg(rows.at(i).scoreBaselineY)
                                    .arg(rows.at(i).nameBaselineY)
                                    .arg(delta)));
        }

        QVERIFY2(qAbs(rel0 - rel1) <= 1.0,
                 qPrintable(QString("%1: delta relatif ligne0=%2 ligne1=%3 (ecart=%4)")
                                .arg(cfg.first)
                                .arg(rel0)
                                .arg(rel1)
                                .arg(rel0 - rel1)));
    }
}

void EventCardGeometryTests::threeDigitScoresAlignRightEdge()
{
    StubTheme theme;
    StubBackend backend;
    StubTranslator translator;

    const QVariantList noGoals;
    const QVariantMap base = model("Home FC", "Away FC", "", "", "2", "1", QString(), noGoals);

    QQuickView view;
    view.engine()->rootContext()->setContextProperty("theme", &theme);
    view.engine()->rootContext()->setContextProperty("backend", &backend);
    view.engine()->rootContext()->setContextProperty("translator", &translator);
    view.setResizeMode(QQuickView::SizeViewToRootObject);
    view.setInitialProperties({{"modelData", base}, {"width", 420}});
    view.setSource(QUrl(QStringLiteral("qrc:/EventCard.qml")));
    QVERIFY2(view.status() == QQuickView::Ready,
             qPrintable(view.errors().isEmpty() ? QStringLiteral("load failed")
                                                : view.errors().first().toString()));

    auto *root = view.rootObject();
    QVERIFY(root);
    view.show();
    settle(view);
    const QList<RowGeom> baseRows = measure(root);
    QCOMPARE(baseRows.size(), 2);

    root->setProperty("modelData",
                      model("Home FC", "Away FC", "", "", "10", "123", QString(), noGoals));
    settle(view);
    const QList<RowGeom> rows = measure(root);
    QCOMPARE(rows.size(), 2);

    const qreal right0 = rows.at(0).scoreX + rows.at(0).scoreWidth;
    const qreal right1 = rows.at(1).scoreX + rows.at(1).scoreWidth;
    QVERIFY2(qAbs(right0 - right1) <= 1.0,
             qPrintable(QStringLiteral("right edges %1 vs %2").arg(right0).arg(right1)));

    for (int i = 0; i < rows.size(); ++i) {
        QVERIFY2(qAbs(rows.at(i).scoreWidth - baseRows.at(i).scoreWidth) <= 1.0,
                 qPrintable(QStringLiteral("row %1 width %2 vs %3")
                                .arg(i)
                                .arg(rows.at(i).scoreWidth)
                                .arg(baseRows.at(i).scoreWidth)));
    }

    saveAsymmetricSnapshot(view);
}

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    QGuiApplication app(argc, argv);
    EventCardGeometryTests tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "eventcard_geometry_tests.moc"
