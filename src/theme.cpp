// Theme watching follows Omacut (MIT, David Heinemeier Hansson).
#include "theme.h"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <cmath>

Theme::Theme(const QString &directory, QObject *parent, const QString &hyprctl)
    : QObject(parent),
      m_directory(directory.isEmpty() ? QDir::homePath() + "/.local/state/omarchy/current" : directory),
      m_monoFamily(QFontDatabase::systemFont(QFontDatabase::FixedFont).family()) {
    if (m_monoFamily.isEmpty())
        m_monoFamily = QStringLiteral("monospace");
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(80);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] { m_debounce.start(); });
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this] { m_debounce.start(); });
    connect(&m_debounce, &QTimer::timeout, this, &Theme::reload);

    // Corners follow Hyprland's effective rounding, which themes and personal
    // overrides can both set, so ask hyprctl rather than reading its config.
    // Without Hyprland the corners stay sharp, like Omarchy's default.
    m_hyprctl = !hyprctl.isEmpty() ? hyprctl
              : directory.isEmpty() && !qEnvironmentVariableIsEmpty("HYPRLAND_INSTANCE_SIGNATURE")
                  ? QStandardPaths::findExecutable("hyprctl") : QString();
    m_roundingTimeout.setSingleShot(true);
    m_roundingTimeout.setInterval(500);
    connect(&m_roundingTimeout, &QTimer::timeout, &m_roundingProcess, &QProcess::kill);
    connect(&m_roundingProcess, &QProcess::errorOccurred, this, [this] { m_roundingTimeout.stop(); });
    connect(&m_roundingProcess, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        m_roundingTimeout.stop();
        const auto value = QJsonDocument::fromJson(m_roundingProcess.readAllStandardOutput()).object().value("int");
        if (code != 0 || status != QProcess::NormalExit || !value.isDouble() || value.toInt(-1) < 0)
            return;
        if (value.toInt() != m_radius) {
            m_radius = value.toInt();
            emit changed();
        }
    });
    m_roundingPoll.setInterval(1000);
    connect(&m_roundingPoll, &QTimer::timeout, this, &Theme::refreshRounding);
    if (!m_hyprctl.isEmpty())
        m_roundingPoll.start();
    reload();
}

Theme::~Theme() {
    if (m_roundingProcess.state() != QProcess::NotRunning) {
        m_roundingProcess.kill();
        m_roundingProcess.waitForFinished(1000);
    }
}

void Theme::refreshRounding() {
    if (m_hyprctl.isEmpty() || m_roundingProcess.state() != QProcess::NotRunning)
        return;
    m_roundingProcess.start(m_hyprctl, {"-j", "getoption", "decoration:rounding"});
    m_roundingTimeout.start();
}

QHash<QString, QString> Theme::readPalette(const QString &path) {
    QHash<QString, QString> values;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return values;
    static const QRegularExpression line(R"re(^\s*([a-z_]+)\s*=\s*"([^"]*)"\s*$)re",
                                         QRegularExpression::MultilineOption);
    auto matches = line.globalMatch(QString::fromUtf8(file.readAll()));
    while (matches.hasNext()) {
        const auto match = matches.next();
        values.insert(match.captured(1), match.captured(2));
    }
    return values;
}

// Black or white, whichever reads on the accent (WCAG relative luminance).
QString Theme::accentForeground() const {
    const QColor c(m_accent);
    auto linear = [](double v) { return v <= .04045 ? v / 12.92 : std::pow((v + .055) / 1.055, 2.4); };
    const double luminance = .2126 * linear(c.redF()) + .7152 * linear(c.greenF()) + .0722 * linear(c.blueF());
    return luminance > .179 ? "black" : "white";
}

namespace {
QString withAlpha(const QString &color, qreal alpha) {
    QColor value(color);
    value.setAlphaF(alpha);
    return value.name(QColor::HexArgb);
}
} // namespace

// Secondary text and hairlines derive from the foreground, so they read on
// both the light and the dark themes.
QString Theme::mutedForeground() const { return withAlpha(m_foreground, 0.55); }
QString Theme::line() const { return withAlpha(m_foreground, 0.14); }

void Theme::reload() {
    refreshRounding();
    const auto paths = m_watcher.files() + m_watcher.directories();
    if (!paths.isEmpty())
        m_watcher.removePaths(paths);

    // A theme switch replaces symlinks and files, so watch the parents too and
    // re-arm after every change. Watching the nearest existing ancestor means
    // installing Omarchy later works without restarting the app.
    QString ancestor = m_directory;
    while (!QFileInfo::exists(ancestor) && ancestor != "/")
        ancestor = QFileInfo(ancestor).absolutePath();
    QStringList candidates{ancestor, QFileInfo(ancestor).absolutePath(), m_directory,
                           m_directory + "/theme", m_directory + "/theme/colors.toml"};
    candidates.removeDuplicates();
    for (const auto &path : candidates)
        if (QFileInfo::exists(path))
            m_watcher.addPath(path);

    const QHash<QString, QString> palette = readPalette(m_directory + "/theme/colors.toml");
    auto color = [&palette](const char *key, const QString &fallback) {
        const QString value = palette.value(QString::fromLatin1(key));
        static const QRegularExpression hex("^#[0-9a-fA-F]{6}$");
        return hex.match(value).hasMatch() ? value : fallback;
    };

    const QString accent = color("accent", "#FFD60A");
    const QString background = color("background", "#0e0e10");
    const QString panel = color("dark_background", "#131316");
    const QString card = color("lighter_background", "#1a1a1f");
    const QString foreground = color("foreground", "#f2f2f4");
    const QString selection = color("selection", "#2a2a31");
    const QString danger = color("red", "#ff6b6b");
    const bool dark = palette.value("mode", "dark") != "light";

    // The palette, not just the accent, decides the window, so a theme switch
    // has to repaint everything.
    const bool repaint = accent != m_accent || background != m_background
                      || panel != m_panel || card != m_card
                      || foreground != m_foreground || selection != m_selection
                      || danger != m_danger || dark != m_dark;
    m_accent = accent;
    m_background = background;
    m_panel = panel;
    m_card = card;
    m_foreground = foreground;
    m_selection = selection;
    m_danger = danger;
    m_dark = dark;
    if (repaint)
        emit changed();
}
