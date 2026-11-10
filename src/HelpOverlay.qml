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
            { keys: "D", action: qsTr("All live scores") },
            { keys: "C", action: qsTr("Standings") },
            { keys: "S", action: qsTr("Choose leagues") },
            { keys: "Esc", action: qsTr("Close") },
            { keys: "Q", action: qsTr("Quit") }
        ]
    }

    Rectangle {
        anchors.fill: parent
        color: theme.background
        opacity: 0.82

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
        color: theme.card
        border.width: 1
        border.color: theme.line

        ColumnLayout {
            id: content
            anchors.fill: parent
            anchors.margins: 22
            spacing: 11

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                LogoMark { size: 18; Layout.alignment: Qt.AlignVCenter }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Keyboard shortcuts")
                    color: theme.foreground
                    font.pixelSize: theme.fsSubtitleLarge
                    font.bold: true
                }
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
                        font.pixelSize: theme.fsBody
                        font.bold: true
                    }

                    Text {
                        Layout.fillWidth: true
                        text: modelData.action
                        color: theme.foreground
                        opacity: 0.82
                        font.pixelSize: theme.fsBody
                    }
                }
            }

            Item { Layout.preferredHeight: 2 }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: theme.line
            }

            Text {
                text: qsTr("Scores from ESPN")
                color: theme.mutedForeground
                font.pixelSize: theme.fsCaption
            }
        }
    }
}
