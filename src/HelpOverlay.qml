import QtQuick
import QtQuick.Layouts

Item {
    id: overlay

    signal closed()

    property var rows: {
        translator.language
        return [
            { keys: "?", action: qsTr("Show this help") },
            { keys: "J / K", action: qsTr("Next / previous league") },
            { keys: "H / L", action: qsTr("Previous / next day") },
            { keys: "T", action: qsTr("Back to today") },
            { keys: "Space / R", action: qsTr("Refresh scores") },
            { keys: "/", action: qsTr("Search leagues") },
            { keys: "Esc", action: qsTr("Close") },
            { keys: "Q", action: qsTr("Quit") }
        ]
    }

    Rectangle {
        anchors.fill: parent
        color: "#05050a"
        opacity: 0.72

        MouseArea {
            anchors.fill: parent
            onClicked: overlay.closed()
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(520, parent.width - 48)
        height: content.implicitHeight + 44
        radius: theme.radius
        color: "#17171b"
        border.width: 1
        border.color: "#2c2c34"

        ColumnLayout {
            id: content
            anchors.fill: parent
            anchors.margins: 22
            spacing: 11

            Text {
                Layout.fillWidth: true
                text: qsTr("Keyboard shortcuts")
                color: "#f2f2f4"
                font.pixelSize: 17
                font.bold: true
            }

            Item { Layout.preferredHeight: 2 }

            Repeater {
                model: overlay.rows

                delegate: RowLayout {
                    required property var modelData

                    Layout.fillWidth: true
                    spacing: 16

                    Text {
                        Layout.preferredWidth: 100
                        text: modelData.keys
                        color: theme.accent
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Text {
                        Layout.fillWidth: true
                        text: modelData.action
                        color: "#c9c9cf"
                        font.pixelSize: 12
                    }
                }
            }

            Item { Layout.preferredHeight: 2 }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: "#26262d"
            }

            Text {
                text: qsTr("Scores from ESPN")
                color: "#6f6f78"
                font.pixelSize: 10
            }
        }
    }
}
