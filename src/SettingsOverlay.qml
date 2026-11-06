import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// Pick which sports appear in the sidebar and in the live view. Changes land
// immediately and are saved by the backend.
Item {
    id: overlay

    signal closed()

    Material.theme: theme.dark ? Material.Dark : Material.Light
    Material.accent: theme.accent
    Material.roundedScale: theme.radius

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
        width: Math.min(460, parent.width - 48)
        height: Math.min(600, parent.height - 64)
        radius: theme.radius
        color: theme.card
        border.width: 1
        border.color: theme.line

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 22
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                LogoMark { size: 18; Layout.alignment: Qt.AlignVCenter }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Sports")
                    color: theme.foreground
                    font.pixelSize: 17
                    font.bold: true
                }
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("Choose the sports you want to follow.")
                color: theme.mutedForeground
                font.pixelSize: 11
                wrapMode: Text.WordWrap
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: theme.line
            }

            ListView {
                id: sports
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 2
                boundsBehavior: Flickable.StopAtBounds
                model: backend.sports

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                    width: 6
                    background: Rectangle { color: "transparent" }
                    contentItem: Rectangle {
                        implicitWidth: 4
                        radius: 2
                        color: theme.foreground
                        opacity: 0.24
                    }
                }

                delegate: Rectangle {
                    id: sportRow
                    required property var modelData

                    width: sports.width - 10
                    height: 42
                    radius: Math.min(6, theme.radius)
                    color: hover.hovered ? theme.selection : "transparent"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 11

                        Rectangle {
                            Layout.preferredWidth: 18
                            Layout.preferredHeight: 18
                            Layout.alignment: Qt.AlignVCenter
                            radius: Math.min(4, theme.radius)
                            color: sportRow.modelData.enabled ? theme.accent : "transparent"
                            border.width: 1
                            border.color: sportRow.modelData.enabled ? theme.accent : theme.line

                            Text {
                                anchors.centerIn: parent
                                visible: sportRow.modelData.enabled
                                text: "✓"
                                color: theme.accentForeground
                                font.pixelSize: 12
                                font.bold: true
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0

                            Text {
                                Layout.fillWidth: true
                                text: sportRow.modelData.name
                                color: theme.foreground
                                font.pixelSize: 13
                                elide: Text.ElideRight
                            }

                            Text {
                                Layout.fillWidth: true
                                text: qsTr("%n leagues", "", sportRow.modelData.leagues)
                                color: theme.mutedForeground
                                font.pixelSize: 10
                            }
                        }
                    }

                    HoverHandler { id: hover }
                    TapHandler {
                        onTapped: backend.setSportEnabled(sportRow.modelData.slug,
                                                          !sportRow.modelData.enabled)
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: theme.line
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                ActionButton {
                    text: qsTr("Show all")
                    onClicked: backend.setAllSportsEnabled(true)
                }

                ActionButton {
                    text: qsTr("Hide all")
                    onClicked: backend.setAllSportsEnabled(false)
                }

                Item { Layout.fillWidth: true }

                ActionButton {
                    text: qsTr("Close")
                    primary: true
                    onClicked: overlay.closed()
                }
            }
        }
    }
}
