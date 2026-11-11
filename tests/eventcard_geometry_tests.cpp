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
    Q_PROPERTY(QString accentForeground MEMBER m_accentForeground CONSTANT)
    Q_PROPERTY(QString mutedForeground MEMBER m_mutedForeground CONSTANT)
    Q_PROPERTY(QString selection MEMBER m_selection CONSTANT)
    Q_PROPERTY(QString line MEMBER m_line CONSTANT)
    Q_PROPERTY(QString monoFamily MEMBER m_monoFamily CONSTANT)
    Q_PROPERTY(qreal fontScale MEMBER m_fontScale NOTIFY fontScaleChanged)
    Q_PROPERTY(int scoreColumnWidth READ scoreColumnWidth NOTIFY fontScaleChanged)
    Q_PROPERTY(int dayBoxWidth READ dayBoxWidth NOTIFY fontScaleChanged)
    Q_PROPERTY(int fsTiny READ fsTiny NOTIFY fontScaleChanged)
    Q_PROPERTY(int fsCaption READ fsCaption NOTIFY fontScaleChanged)
    Q_PROPERTY(int fsSmall READ fsSmall NOTIFY fontScaleChanged)
    Q_PROPERTY(int fsBody READ fsBody NOTIFY fontScaleChanged)
    Q_PROPERTY(int fsBodyLarge READ fsBodyLarge NOTIFY fontScaleChanged)
    Q_PROPERTY(int fsName READ fsName NOTIFY fontScaleChanged)
    Q_PROPERTY(int fsNameLarge READ fsNameLarge NOTIFY fontScaleChanged)
    Q_PROPERTY(int fsSubtitle READ fsSubtitle NOTIFY fontScaleChanged)
    Q_PROPERTY(int fsSubtitleLarge READ fsSubtitleLarge NOTIFY fontScaleChanged)
    Q_PROPERTY(int fsScore READ fsScore NOTIFY fontScaleChanged)
    Q_PROPERTY(int fsTitle READ fsTitle NOTIFY fontScaleChanged)
    Q_PROPERTY(int fsHero READ fsHero NOTIFY fontScaleChanged)
    Q_PROPERTY(int fsDisplay READ fsDisplay NOTIFY fontScaleChanged)

public:
    int m_radius = 8;
    QString m_card = "#1a1a1f";
    QString m_foreground = "#f2f2f4";
    QString m_accent = "#FFD60A";
    QString m_accentForeground = "#000000";
    QString m_mutedForeground = "#888888";
    QString m_selection = "#2a2a31";
    QString m_line = "#3a3a40";
    QString m_monoFamily = QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
    qreal m_fontScale = 1.0;

    int scaled(int base) const { return qRound(base * m_fontScale); }
    int scoreColumnWidth() const { return scaled(60); }
    int dayBoxWidth() const { return scaled(168); }
    int fsTiny() const { return scaled(9); }
    int fsCaption() const { return scaled(10); }
    int fsSmall() const { return scaled(11); }
    int fsBody() const { return scaled(12); }
    int fsBodyLarge() const { return scaled(13); }
    int fsName() const { return scaled(14); }
    int fsNameLarge() const { return scaled(15); }
    int fsSubtitle() const { return scaled(16); }
    int fsSubtitleLarge() const { return scaled(17); }
    int fsScore() const { return scaled(20); }
    int fsTitle() const { return scaled(26); }
    int fsHero() const { return scaled(30); }
    int fsDisplay() const { return scaled(40); }

    void setScale(qreal scale)
    {
        if (qFuzzyCompare(m_fontScale, scale))
            return;
        m_fontScale = scale;
        emit fontScaleChanged();
    }

signals:
    void fontScaleChanged();
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

    void saveSnapshot(QQuickView &view, const QString &path);

private slots:
    void scoreGeometryIsStable();
    void asymmetricScoreGeometry();
    void threeDigitScoresAlignRightEdge();
    void controlSnapshots();
    void dayNavigationStaysFixed();
};

void EventCardGeometryTests::saveSnapshot(QQuickView &view, const QString &path)
{
    if (!view.isVisible())
        return;
    const QImage image = view.grabWindow();
    if (image.isNull())
        return;
    image.save(path);
}

void EventCardGeometryTests::scoreGeometryIsStable()
{
    for (qreal scale : {0.9, 1.0, 1.2}) {
        StubTheme theme;
        theme.setScale(scale);
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
            model("Home FC", "Away FC", "3-1-5", "", noGoals),
            model("Home FC", "Away FC", "", crest, noGoals),
            model("A very very very long team name that must elide",
                  "Another extremely long visiting team name", "", "", noGoals),
            model("Home FC", "Away FC", "", "", goals),
        };

        for (int c = 0; c < configs.size(); ++c) {
            root->setProperty("modelData", configs.at(c));
            settle(view);
            const QList<RowGeom> rows = measure(root);
            QCOMPARE(rows.size(), 2);
            for (int i = 0; i < rows.size(); ++i) {
                QVERIFY2(qAbs(rows.at(i).scoreX - baseRows.at(i).scoreX) <= 1.0,
                         qPrintable(QString("scale %1 config %2 row %3 scoreX %4 vs %5")
                                        .arg(scale).arg(c).arg(i).arg(rows.at(i).scoreX)
                                        .arg(baseRows.at(i).scoreX)));
                QVERIFY2(qAbs(rows.at(i).scoreBaselineY - rows.at(i).nameBaselineY) <= 1.0,
                         qPrintable(QString("scale %1 config %2 row %3 scoreBaselineY %4 vs nameBaselineY %5")
                                        .arg(scale).arg(c).arg(i).arg(rows.at(i).scoreBaselineY)
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
                         qPrintable(QString("scale %1 expected 2 rendered badges, got %2").arg(scale).arg(readyBadges)));
            }
        }
    }
}

void EventCardGeometryTests::asymmetricScoreGeometry()
{
    for (qreal scale : {0.9, 1.0, 1.2}) {
        StubTheme theme;
        theme.setScale(scale);
        StubBackend backend;
        StubTranslator translator;

        const QVariantList noGoals;
        const QVariantList goals = QVariantList{
            QVariantMap{{"clock", "12'"}, {"scorer", "Scorer"}, {"teamAbbreviation", "HOM"},
                        {"ownGoal", false}, {"penalty", false}, {"type", "Goal"}}};

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

        if (qFuzzyCompare(scale, 1.0))
            saveSnapshot(view, QStringLiteral("/tmp/omas-card-asym.png"));

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
                         qPrintable(QString("scale %1 %2: ligne %3 scoreBaselineY=%4 nameBaselineY=%5 (delta=%6)")
                                        .arg(scale).arg(cfg.first).arg(i)
                                        .arg(rows.at(i).scoreBaselineY)
                                        .arg(rows.at(i).nameBaselineY)
                                        .arg(delta)));
            }

            QVERIFY2(qAbs(rel0 - rel1) <= 1.0,
                     qPrintable(QString("scale %1 %2: delta relatif ligne0=%3 ligne1=%4 (ecart=%5)")
                                    .arg(scale).arg(cfg.first)
                                    .arg(rel0).arg(rel1).arg(rel0 - rel1)));
        }
    }
}

void EventCardGeometryTests::threeDigitScoresAlignRightEdge()
{
    for (qreal scale : {0.9, 1.0, 1.2}) {
        StubTheme theme;
        theme.setScale(scale);
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
                 qPrintable(QStringLiteral("scale %1 right edges %2 vs %3").arg(scale).arg(right0).arg(right1)));

        for (int i = 0; i < rows.size(); ++i) {
            QVERIFY2(qAbs(rows.at(i).scoreWidth - baseRows.at(i).scoreWidth) <= 1.0,
                     qPrintable(QStringLiteral("scale %1 row %2 width %3 vs %4")
                                    .arg(scale)
                                    .arg(i)
                                    .arg(rows.at(i).scoreWidth)
                                    .arg(baseRows.at(i).scoreWidth)));
        }
    }
}

void EventCardGeometryTests::controlSnapshots()
{
    const QList<QPair<qreal, QString>> scales = {
        {0.9, QStringLiteral("/tmp/omas-card-compact.png")},
        {1.0, QStringLiteral("/tmp/omas-card-normal.png")},
        {1.2, QStringLiteral("/tmp/omas-card-large.png")},
    };
    for (const auto &entry : scales) {
        StubTheme theme;
        theme.setScale(entry.first);
        StubBackend backend;
        StubTranslator translator;

        const QVariantList goals = QVariantList{
            QVariantMap{{"clock", "12'"}, {"scorer", "Scorer"}, {"teamAbbreviation", "HOM"},
                        {"ownGoal", false}, {"penalty", false}, {"type", "Goal"}}};
        const QVariantMap sample = model("Borussia Moenchengladbach", "Wolverhampton Wanderers", "",
                                         "4-3-3", "10", "123", QString(), goals);

        QQuickView view;
        view.engine()->rootContext()->setContextProperty("theme", &theme);
        view.engine()->rootContext()->setContextProperty("backend", &backend);
        view.engine()->rootContext()->setContextProperty("translator", &translator);
        view.setResizeMode(QQuickView::SizeViewToRootObject);
        view.setInitialProperties({{"modelData", sample}, {"width", 430}});
        view.setSource(QUrl(QStringLiteral("qrc:/EventCard.qml")));
        QVERIFY(view.status() == QQuickView::Ready);
        view.show();
        settle(view);
        saveSnapshot(view, entry.second);
    }
}

void EventCardGeometryTests::dayNavigationStaysFixed()
{
    const QStringList longLabels = {
        QStringLiteral("sam. 30 sept. 2026"),
        QStringLiteral("Wed. 30 Sept. 2026"),
    };

    for (qreal scale : {0.9, 1.0, 1.2}) {
        StubTheme theme;
        theme.setScale(scale);
        StubBackend backend;
        StubTranslator translator;

        QQuickView view;
        view.engine()->rootContext()->setContextProperty("theme", &theme);
        view.engine()->rootContext()->setContextProperty("backend", &backend);
        view.engine()->rootContext()->setContextProperty("translator", &translator);
        view.setResizeMode(QQuickView::SizeViewToRootObject);
        view.setInitialProperties({{"dateLabel", QStringLiteral("sam. 10 oct. 2026")},
                                   {"isToday", true}});
        view.setSource(QUrl(QStringLiteral("qrc:/DayNavigation.qml")));
        QVERIFY2(view.status() == QQuickView::Ready,
                 qPrintable(view.errors().isEmpty() ? QStringLiteral("load failed")
                                                    : view.errors().first().toString()));

        auto *root = view.rootObject();
        QVERIFY(root);
        view.show();
        settle(view);

        auto *dateText = root->findChild<QQuickItem *>(QStringLiteral("dateText"));
        auto *prevButton = root->findChild<QQuickItem *>(QStringLiteral("prevButton"));
        auto *nextButton = root->findChild<QQuickItem *>(QStringLiteral("nextButton"));
        auto *todayButton = root->findChild<QQuickItem *>(QStringLiteral("todayButton"));
        QVERIFY(dateText);
        QVERIFY(prevButton);
        QVERIFY(nextButton);
        QVERIFY(todayButton);
        // "Today" remains visible and usable even when the selected day is today.
        QVERIFY(todayButton->isVisible());
        QVERIFY(todayButton->isEnabled());
        QVERIFY(todayButton->opacity() > 0.99);

        const qreal dateXToday = dateText->mapToItem(root, 0, 0).x();
        const qreal prevXToday = prevButton->mapToItem(root, 0, 0).x();
        const qreal nextXToday = nextButton->mapToItem(root, 0, 0).x();
        const qreal baseWidth = dateText->width();
        const qreal baseHeight = dateText->height();
        qInfo().noquote() << "daynav scale" << scale << "today dateX" << dateXToday << "prevX" << prevXToday << "nextX" << nextXToday
                          << "width" << baseWidth;

        if (qFuzzyCompare(scale, 1.0))
            saveSnapshot(view, QStringLiteral("/tmp/omas-daynav-today.png"));

        root->setProperty("isToday", false);
        settle(view);

        const qreal dateXOther = dateText->mapToItem(root, 0, 0).x();
        const qreal prevXOther = prevButton->mapToItem(root, 0, 0).x();
        const qreal nextXOther = nextButton->mapToItem(root, 0, 0).x();
        qInfo().noquote() << "daynav scale" << scale << "other dateX" << dateXOther << "prevX" << prevXOther << "nextX" << nextXOther;

        QVERIFY2(qAbs(dateXToday - dateXOther) <= 1.0,
                 qPrintable(QStringLiteral("scale %1 date X %2 vs %3")
                                .arg(scale).arg(dateXToday).arg(dateXOther)));
        QVERIFY2(qAbs(prevXToday - prevXOther) <= 1.0,
                 qPrintable(QStringLiteral("scale %1 prev X %2 vs %3")
                                .arg(scale).arg(prevXToday).arg(prevXOther)));
        QVERIFY2(qAbs(nextXToday - nextXOther) <= 1.0,
                 qPrintable(QStringLiteral("scale %1 next X %2 vs %3")
                                .arg(scale).arg(nextXToday).arg(nextXOther)));
        QVERIFY2(qAbs(dateText->width() - baseWidth) <= 1.0,
                 qPrintable(QStringLiteral("scale %1 date width changed %2 vs %3")
                                .arg(scale).arg(baseWidth).arg(dateText->width())));

        if (qFuzzyCompare(scale, 1.0))
            saveSnapshot(view, QStringLiteral("/tmp/omas-daynav-other.png"));

        for (const QString &label : longLabels) {
            root->setProperty("dateLabel", label);
            settle(view);
            QVERIFY2(dateText->property("lineCount").toInt() == 1,
                     qPrintable(QStringLiteral("scale %1 label '%2' lineCount %3")
                                    .arg(scale, 0, 'f', 1)
                                    .arg(label)
                                    .arg(dateText->property("lineCount").toInt())));
            QVERIFY2(dateText->property("contentWidth").toReal() <= dateText->width() + 0.5,
                     qPrintable(QStringLiteral("scale %1 label '%2' contentWidth %3 > width %4")
                                    .arg(scale, 0, 'f', 1)
                                    .arg(label)
                                    .arg(dateText->property("contentWidth").toReal())
                                    .arg(dateText->width())));
            QVERIFY2(qAbs(dateText->height() - baseHeight) <= 1.0,
                     qPrintable(QStringLiteral("scale %1 label '%2' height %3 vs %4")
                                    .arg(scale, 0, 'f', 1)
                                    .arg(label)
                                    .arg(dateText->height())
                                    .arg(baseHeight)));
        }
    }
}

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    QGuiApplication app(argc, argv);
    EventCardGeometryTests tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "eventcard_geometry_tests.moc"
