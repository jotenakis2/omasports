import QtQuick

// A podium of scores, rising to the right: three solid bars on one baseline.
// In the window it follows the live accent; pkgbuild/omasports.svg carries
// the same mark for the launcher, where no palette is available.
Item {
    id: mark

    property int size: 22

    implicitWidth: size
    implicitHeight: size

    readonly property real bar: size * 0.22
    readonly property real gap: size * 0.09
    readonly property real baseY: size * 0.95
    readonly property real radius: Math.max(1, size * 0.05)

    Rectangle {
        x: (mark.size - (mark.bar * 3 + mark.gap * 2)) / 2
        y: mark.baseY - mark.size * 0.45
        width: mark.bar
        height: mark.size * 0.45
        radius: mark.radius
        color: theme.accent
    }

    Rectangle {
        x: (mark.size - (mark.bar * 3 + mark.gap * 2)) / 2 + mark.bar + mark.gap
        y: mark.baseY - mark.size * 0.66
        width: mark.bar
        height: mark.size * 0.66
        radius: mark.radius
        color: theme.accent
    }

    Rectangle {
        x: (mark.size - (mark.bar * 3 + mark.gap * 2)) / 2 + (mark.bar + mark.gap) * 2
        y: mark.baseY - mark.size * 0.87
        width: mark.bar
        height: mark.size * 0.87
        radius: mark.radius
        color: theme.accent
    }
}
