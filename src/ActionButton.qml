import QtQuick

// A flat button that follows the theme, used for headers and empty states.
Rectangle {
    id: control

    property string text: ""
    property bool primary: false
    signal clicked()

    implicitWidth: label.implicitWidth + 24
    implicitHeight: 30
    radius: Math.min(6, theme.radius)
    color: primary ? theme.accent : (hover.hovered ? "#232329" : "#1a1a1f")
    border.width: primary ? 0 : 1
    border.color: "#2a2a31"
    opacity: enabled ? 1 : 0.4

    Text {
        id: label
        anchors.centerIn: parent
        text: control.text
        color: control.primary ? theme.accentForeground : "#d8d8de"
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
