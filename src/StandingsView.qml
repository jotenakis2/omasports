import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// The table for the selected league, grouped when the feed splits it into
// conferences or pools.
Item {
    id: view

    Material.theme: theme.dark ? Material.Dark : Material.Light
    Material.accent: theme.accent
    Material.roundedScale: theme.radius

    Flickable {
        id: scroller
        anchors.fill: parent
        visible: backend.standings.length > 0
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        contentWidth: width
        contentHeight: tables.implicitHeight

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

        Column {
            id: tables
            width: scroller.width - 12
            spacing: 18

            Repeater {
                model: backend.standings

                delegate: Column {
                    id: groupColumn
                    required property var modelData

                    width: tables.width
                    spacing: 6

                    Text {
                        visible: groupColumn.modelData.name.length > 0
                        text: groupColumn.modelData.name.length > 0
                              ? qsTr(groupColumn.modelData.name) : ""
                        color: theme.mutedForeground
                        font.pixelSize: 10
                        font.bold: true
                        font.capitalization: Font.AllUppercase
                        font.letterSpacing: 1
                    }

                    Rectangle {
                        width: parent.width
                        height: table.implicitHeight + 16
                        radius: theme.radius
                        color: theme.card

                        Column {
                            id: table
                            x: 10
                            y: 8
                            width: parent.width - 20
                            spacing: 0

                            RowLayout {
                                width: table.width
                                height: 26
                                spacing: 0

                                Text {
                                    Layout.preferredWidth: 28
                                    text: "#"
                                    color: theme.mutedForeground
                                    font.pixelSize: 10
                                    font.bold: true
                                }

                                Item { Layout.preferredWidth: 27 }

                                Text {
                                    Layout.fillWidth: true
                                    text: qsTr("Team")
                                    color: theme.mutedForeground
                                    font.pixelSize: 10
                                    font.bold: true
                                }

                                Repeater {
                                    model: backend.standingsHeaders

                                    delegate: Text {
                                        required property string modelData

                                        Layout.preferredWidth: 44
                                        horizontalAlignment: Text.AlignRight
                                        text: modelData
                                        color: theme.mutedForeground
                                        font.pixelSize: 10
                                        font.bold: true
                                    }
                                }
                            }

                            Rectangle {
                                width: table.width
                                height: 1
                                color: theme.line
                            }

                            Repeater {
                                model: groupColumn.modelData.rows

                                delegate: Column {
                                    id: standingsRow
                                    required property var modelData

                                    width: table.width
                                    spacing: 0

                                    RowLayout {
                                        width: table.width
                                        height: 32
                                        spacing: 0

                                        Text {
                                            Layout.preferredWidth: 28
                                            text: standingsRow.modelData.rank
                                            color: theme.mutedForeground
                                            font.pixelSize: 11
                                        }

                                        TeamBadge {
                                            team: standingsRow.modelData.team
                                            Layout.preferredWidth: 20
                                            Layout.preferredHeight: 20
                                            Layout.rightMargin: 7
                                        }

                                        Text {
                                            Layout.fillWidth: true
                                            text: standingsRow.modelData.team.name
                                            color: theme.foreground
                                            font.pixelSize: 12
                                            elide: Text.ElideRight
                                        }

                                        Repeater {
                                            model: standingsRow.modelData.cells

                                            delegate: Text {
                                                required property string modelData

                                                Layout.preferredWidth: 44
                                                horizontalAlignment: Text.AlignRight
                                                text: modelData
                                                color: theme.foreground
                                                font.pixelSize: 11
                                            }
                                        }
                                    }

                                    Rectangle {
                                        width: table.width
                                        height: 1
                                        color: theme.line
                                        opacity: 0.6
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        visible: backend.standings.length === 0
                 && backend.status !== "ready" && backend.status !== "error"
        spacing: 12

        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            running: parent.visible
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Loading standings…")
            color: theme.mutedForeground
            font.pixelSize: 12
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(420, parent.width)
        visible: backend.standings.length === 0 && backend.status === "error"
        spacing: 10

        Text {
            Layout.fillWidth: true
            text: qsTr("Could not load standings")
            color: theme.foreground
            font.pixelSize: 16
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            Layout.fillWidth: true
            text: backend.error
            color: theme.mutedForeground
            font.pixelSize: 11
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        ActionButton {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Retry")
            primary: true
            onClicked: backend.refresh()
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        visible: backend.standings.length === 0 && backend.status === "ready"
        spacing: 12

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "—"
            color: theme.mutedForeground
            font.pixelSize: 40
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("No standings for this competition.")
            color: theme.mutedForeground
            font.pixelSize: 14
        }
    }
}
