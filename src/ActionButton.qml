import QtQuick

// A flat button in the theme's own colors, used in headers and empty states.
Rectangle {
    id: control

    property string text: ""
    property bool primary: false
    signal clicked()

    implicitWidth: label.implicitWidth + 24
    implicitHeight: 30
    radius: Math.min(6, theme.radius)
    color: primary ? theme.accent : (hover.hovered ? theme.selection : theme.card)
    border.width: primary ? 0 : 1
    border.color: theme.line
    opacity: enabled ? 1 : 0.4

    Text {
        id: label
        anchors.centerIn: parent
        text: control.text
        color: control.primary ? theme.accentForeground : theme.foreground
        font.pixelSize: 12
        font.bold: control.primary
    }

    HoverHandler {
        id: hover
        enabled: control.enabled
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        enabled: control.enabled
        cursorShape: Qt.PointingHandCursor
        onClicked: control.clicked()
    }

    scale: mouse.pressed ? 0.97 : 1
    Behavior on scale {
        NumberAnimation { duration: 80 }
    }
}
