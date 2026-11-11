import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// The upcoming schedule for the selected league, grouped by day.
Item {
    id: view

    Material.theme: theme.dark ? Material.Dark : Material.Light
    Material.accent: theme.accent
    Material.roundedScale: theme.radius

    Flickable {
        id: scroller
        anchors.fill: parent
        visible: backend.upcoming.length > 0
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        contentWidth: width
        contentHeight: days.implicitHeight

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
            id: days
            width: scroller.width - 12
            spacing: 14

            Repeater {
                model: backend.upcoming

                delegate: Column {
                    id: dayGroup
                    required property var modelData

                    width: days.width
                    spacing: 6

                    Text {
                        text: dayGroup.modelData.label
                        color: theme.mutedForeground
                        font.pixelSize: theme.fsCaption
                        font.bold: true
                        font.capitalization: Font.AllUppercase
                        font.letterSpacing: 1
                    }

                    Rectangle {
                        width: parent.width
                        height: matchesColumn.implicitHeight + 8
                        radius: theme.radius
                        color: theme.card

                        Column {
                            id: matchesColumn
                            x: 10
                            y: 4
                            width: parent.width - 20
                            spacing: 0

                            Repeater {
                                model: dayGroup.modelData.matches

                                delegate: Column {
                                    id: matchRow
                                    required property var modelData

                                    width: matchesColumn.width
                                    spacing: 0

                                    RowLayout {
                                        width: matchesColumn.width
                                        height: Math.max(34, venueText.visible ? 46 : 34)
                                        spacing: 8

                                        Text {
                                            Layout.preferredWidth: 52
                                            text: matchRow.modelData.timeLabel || ""
                                            color: theme.mutedForeground
                                            font.pixelSize: theme.fsSmall
                                            font.family: theme.monoFamily
                                        }

                                        Text {
                                            Layout.fillWidth: true
                                            text: {
                                                var teams = matchRow.modelData.teams || []
                                                var home = teams.length > 0 ? (teams[0].name || teams[0].shortName || "") : ""
                                                var away = teams.length > 1 ? (teams[1].name || teams[1].shortName || "") : ""
                                                return home + "  –  " + away
                                            }
                                            color: theme.foreground
                                            font.pixelSize: theme.fsBody
                                            elide: Text.ElideRight
                                        }

                                        Text {
                                            id: venueText
                                            Layout.preferredWidth: Math.min(implicitWidth, matchRow.width * 0.3)
                                            visible: (matchRow.modelData.venue || "").length > 0
                                            text: matchRow.modelData.venue || ""
                                            color: theme.mutedForeground
                                            font.pixelSize: theme.fsSmall
                                            elide: Text.ElideRight
                                            horizontalAlignment: Text.AlignRight
                                        }
                                    }

                                    Rectangle {
                                        width: matchesColumn.width
                                        height: 1
                                        color: theme.line
                                        opacity: 0.6
                                        visible: index < (dayGroup.modelData.matches.length - 1)
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
        visible: backend.upcomingLoading && backend.upcoming.length === 0
        spacing: 12

        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            running: parent.visible
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Loading schedule…")
            color: theme.mutedForeground
            font.pixelSize: theme.fsBody
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(420, parent.width)
        visible: !backend.upcomingLoading && backend.upcoming.length === 0
                 && backend.upcomingError.length > 0
        spacing: 10

        Text {
            Layout.fillWidth: true
            text: qsTr("Could not load schedule")
            color: theme.foreground
            font.pixelSize: theme.fsSubtitle
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            Layout.fillWidth: true
            text: backend.upcomingError
            color: theme.mutedForeground
            font.pixelSize: theme.fsSmall
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            Layout.fillWidth: true
            text: qsTr("Press r to retry.")
            color: theme.mutedForeground
            font.pixelSize: theme.fsSmall
            horizontalAlignment: Text.AlignHCenter
        }

        ActionButton {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Retry")
            primary: true
            onClicked: backend.refreshUpcoming()
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        visible: !backend.upcomingLoading && backend.upcoming.length === 0
                 && backend.upcomingError.length === 0
        spacing: 12

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "—"
            color: theme.mutedForeground
            font.pixelSize: theme.fsDisplay
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("No upcoming matches.")
            color: theme.mutedForeground
            font.pixelSize: theme.fsName
        }
    }
}
