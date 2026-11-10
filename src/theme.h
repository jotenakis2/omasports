#pragma once

#include <QFileSystemWatcher>
#include <QHash>
#include <QObject>
#include <QProcess>
#include <QTimer>

// Follows the Omarchy theme live: the accent, the palette to draw on, and
// Hyprland's window rounding. Tests construct it with a temp directory and a
// fake hyprctl.
class Theme : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString accent READ accent NOTIFY changed)
    Q_PROPERTY(QString accentForeground READ accentForeground NOTIFY changed)
    Q_PROPERTY(QString background READ background NOTIFY changed)
    Q_PROPERTY(QString panel READ panel NOTIFY changed)
    Q_PROPERTY(QString card READ card NOTIFY changed)
    Q_PROPERTY(QString foreground READ foreground NOTIFY changed)
    Q_PROPERTY(QString mutedForeground READ mutedForeground NOTIFY changed)
    Q_PROPERTY(QString selection READ selection NOTIFY changed)
    Q_PROPERTY(QString line READ line NOTIFY changed)
    Q_PROPERTY(QString danger READ danger NOTIFY changed)
    Q_PROPERTY(bool dark READ dark NOTIFY changed)
    Q_PROPERTY(int radius READ radius NOTIFY changed)
    Q_PROPERTY(QString monoFamily READ monoFamily CONSTANT)

public:
    explicit Theme(const QString &currentDirectory = {}, QObject *parent = nullptr,
                   const QString &hyprctl = {});
    ~Theme() override;

    QString accent() const { return m_accent; }
    QString accentForeground() const;
    QString background() const { return m_background; }
    QString panel() const { return m_panel; }
    QString card() const { return m_card; }
    QString foreground() const { return m_foreground; }
    QString mutedForeground() const;
    QString selection() const { return m_selection; }
    QString line() const;
    QString danger() const { return m_danger; }
    bool dark() const { return m_dark; }
    int radius() const { return m_radius; }
    QString monoFamily() const { return m_monoFamily; }

    // Every `key = "#rrggbb"` line of an Omarchy theme's colors.toml.
    static QHash<QString, QString> readPalette(const QString &path);

signals:
    void changed();

private:
    void reload();
    void refreshRounding();

    QString m_directory;
    QString m_accent = "#FFD60A";
    QString m_background = "#0e0e10";
    QString m_panel = "#131316";
    QString m_card = "#1a1a1f";
    QString m_foreground = "#f2f2f4";
    QString m_selection = "#2a2a31";
    QString m_danger = "#ff6b6b";
    bool m_dark = true;
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
    QString m_hyprctl;
    QString m_monoFamily = "monospace";
    int m_radius = 0;
    QProcess m_roundingProcess;
    QTimer m_roundingPoll, m_roundingTimeout;
};
