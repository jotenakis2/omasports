// omasports — scores and fixtures for every sport, live, from the keyboard.

#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>

#include "backend.h"
#include "theme.h"
#include "translator.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("omasports");
    app.setApplicationVersion("1.0.0");

    // Matches the window to omasports.desktop, so the compositor (Wayland
    // app_id) and the launcher pick up the installed icon.
    app.setDesktopFileName("omasports");
    app.setWindowIcon(QIcon::fromTheme("omasports"));

    QQuickStyle::setStyle("Material");

    // Installed before anything is translated, and every qsTr() binding is
    // re-evaluated when the Omarchy language changes.
    Translator translator;
    app.installTranslator(&translator);

    Theme theme;
    Backend backend;

    theme.setFontScale(backend.fontScale());
    QObject::connect(&backend, &Backend::fontScaleChanged, &theme,
                     [&theme, &backend] { theme.setFontScale(backend.fontScale()); });

    QQmlApplicationEngine engine;
    QObject::connect(&translator, &Translator::changed, &engine, &QQmlEngine::retranslate);
    // Sport names come from C++, so the catalog is rebuilt on a language
    // switch as well.
    QObject::connect(&translator, &Translator::changed, &backend, &Backend::retranslate);
    engine.rootContext()->setContextProperty("translator", &translator);
    engine.rootContext()->setContextProperty("theme", &theme);
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.load(QUrl("qrc:/Main.qml"));
    if (engine.rootObjects().isEmpty())
        return 1;

    backend.refresh();
    return app.exec();
}
