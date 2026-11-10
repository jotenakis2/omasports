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
    Q_PROPERTY(qreal fontScale READ fontScale WRITE setFontScale NOTIFY fontScaleChanged)
    Q_PROPERTY(int scoreColumnWidth READ scoreColumnWidth NOTIFY fontScaleChanged)
    // Font size roles (at fontScale 1.0): fsTiny=9, fsCaption=10, fsSmall=11,
    // fsBody=12, fsBodyLarge=13, fsName=14, fsNameLarge=15, fsSubtitle=16,
    // fsSubtitleLarge=17, fsScore=20, fsTitle=26, fsHero=30, fsDisplay=40.
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
    qreal fontScale() const { return m_fontScale; }
    void setFontScale(qreal scale);
    int scoreColumnWidth() const;

    // Font size roles: historical values at scale 1.0.
    int fsTiny() const;          // 9
    int fsCaption() const;       // 10
    int fsSmall() const;         // 11
    int fsBody() const;          // 12
    int fsBodyLarge() const;     // 13
    int fsName() const;          // 14
    int fsNameLarge() const;     // 15
    int fsSubtitle() const;      // 16
    int fsSubtitleLarge() const; // 17
    int fsScore() const;         // 20
    int fsTitle() const;         // 26
    int fsHero() const;          // 30
    int fsDisplay() const;       // 40

    // Every `key = "#rrggbb"` line of an Omarchy theme's colors.toml.
    static QHash<QString, QString> readPalette(const QString &path);

signals:
    void changed();
    void fontScaleChanged();

private:
    int scaled(int base) const;
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
    qreal m_fontScale = 1.0;
    QProcess m_roundingProcess;
    QTimer m_roundingPoll, m_roundingTimeout;
};
