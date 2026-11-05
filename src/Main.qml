import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: win

    width: 1080
    height: 740
    minimumWidth: 720
    minimumHeight: 480
    visible: true
    title: "omasports"
    color: "#0e0e10"

    Material.theme: Material.Dark
    Material.accent: theme.accent
    Material.roundedScale: theme.radius

    property bool helpVisible: false

    function stepLeague(delta) {
        sidebar.step(delta)
    }

    Shortcut {
        sequences: ["j", "J"]
        context: Qt.ApplicationShortcut
        enabled: !sidebar.searching && !win.helpVisible
        onActivated: win.stepLeague(1)
    }
    Shortcut {
        sequences: ["k", "K"]
        context: Qt.ApplicationShortcut
        enabled: !sidebar.searching && !win.helpVisible
        onActivated: win.stepLeague(-1)
    }
    Shortcut {
        sequences: ["Up"]
        context: Qt.ApplicationShortcut
        enabled: !sidebar.searching && !win.helpVisible
        onActivated: win.stepLeague(-1)
    }
    Shortcut {
        sequences: ["Down"]
        context: Qt.ApplicationShortcut
        enabled: !sidebar.searching && !win.helpVisible
        onActivated: win.stepLeague(1)
    }
    Shortcut {
        sequences: ["h", "H", "Left"]
        context: Qt.ApplicationShortcut
        enabled: !sidebar.searching && !win.helpVisible
        onActivated: backend.previousDay()
    }
    Shortcut {
        sequences: ["l", "L", "Right"]
        context: Qt.ApplicationShortcut
        enabled: !sidebar.searching && !win.helpVisible
        onActivated: backend.nextDay()
    }
    Shortcut {
        sequences: ["t", "T"]
        context: Qt.ApplicationShortcut
        enabled: !sidebar.searching && !win.helpVisible
        onActivated: backend.today()
    }
    Shortcut {
        sequences: ["r", "R", "Space"]
        context: Qt.ApplicationShortcut
        enabled: !sidebar.searching && !win.helpVisible
        onActivated: backend.refresh()
    }
    Shortcut {
        sequence: "/"
        context: Qt.ApplicationShortcut
        enabled: !sidebar.searching && !win.helpVisible
        onActivated: sidebar.focusSearch()
    }
    Shortcut {
        sequences: ["q", "Q"]
        context: Qt.ApplicationShortcut
        enabled: !sidebar.searching && !win.helpVisible
        onActivated: Qt.quit()
    }
    Shortcut {
        sequence: "?"
        context: Qt.ApplicationShortcut
        enabled: !sidebar.searching
        onActivated: win.helpVisible = !win.helpVisible
    }
    Shortcut {
        sequence: "Escape"
        context: Qt.ApplicationShortcut
        enabled: win.helpVisible
        onActivated: win.helpVisible = false
    }

    // Live games move on their own; a quiet day only refreshes when asked.
    Timer {
        interval: 30000
        repeat: true
        running: backend.liveCount > 0 && win.visible && !win.helpVisible
        onTriggered: backend.refresh()
    }

    onActiveChanged: {
        if (active && backend.liveCount > 0)
            backend.refresh()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: "#111114"

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: "#232329"
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 14
                spacing: 8

                Text {
                    text: "omasports"
                    color: theme.accent
                    font.pixelSize: 17
                    font.bold: true
                    font.letterSpacing: 0.3
                }

                Item { Layout.fillWidth: true }

                ActionButton {
                    text: "‹"
                    onClicked: backend.previousDay()
                }

                Text {
                    Layout.preferredWidth: 160
                    text: backend.dateLabel
                    color: "#e6e6ea"
                    font.pixelSize: 13
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                }

                ActionButton {
                    text: "›"
                    onClicked: backend.nextDay()
                }

                ActionButton {
                    text: qsTr("Today")
                    primary: !backend.isToday
                    enabled: !backend.isToday
                    onClicked: backend.today()
                }

                Item { Layout.preferredWidth: 8 }

                RowLayout {
                    visible: backend.liveCount > 0
                    spacing: 6

                    Rectangle {
                        Layout.alignment: Qt.AlignVCenter
                        width: 7
                        height: 7
                        radius: 3.5
                        color: theme.accent

                        SequentialAnimation on opacity {
                            running: backend.liveCount > 0
                            loops: Animation.Infinite
                            NumberAnimation { to: 0.45; duration: 800; easing.type: Easing.InOutQuad }
                            NumberAnimation { to: 1; duration: 800; easing.type: Easing.InOutQuad }
                        }
                    }

                    Text {
                        text: qsTr("%n live", "", backend.liveCount)
                        color: theme.accent
                        font.pixelSize: 12
                        font.bold: true
                    }
                }

                ActionButton {
                    text: qsTr("Refresh")
                    onClicked: backend.refresh()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            LeagueSidebar {
                id: sidebar
                Layout.preferredWidth: 264
                Layout.fillHeight: true
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: "#232329"
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.topMargin: 22
                    anchors.bottomMargin: 16
                    width: Math.min(820, parent.width - 40)
                    spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            Layout.fillWidth: true
                            text: backend.league.name || ""
                            color: "#f2f2f4"
                            font.pixelSize: 24
                            font.bold: true
                            elide: Text.ElideRight
                        }

                        Text {
                            Layout.fillWidth: true
                            text: {
                                translator.language
                                var count = backend.events.length
                                return backend.status === "ready" && count > 0
                                       ? backend.league.sportName + "  ·  " + qsTr("%n games", "", count)
                                       : backend.league.sportName
                            }
                            color: "#77777f"
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32
                        visible: backend.status === "error" && backend.events.length > 0
                        color: "#2a1414"
                        radius: Math.min(6, theme.radius)

                        Text {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Refresh failed: %1").arg(backend.error)
                            color: "#f0a0a0"
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        ListView {
                            id: games
                            anchors.fill: parent
                            visible: backend.events.length > 0
                            clip: true
                            spacing: 8
                            model: backend.events
                            boundsBehavior: Flickable.StopAtBounds
                            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                            delegate: EventCard {
                                width: games.width
                            }
                        }

                        ColumnLayout {
                            anchors.centerIn: parent
                            visible: backend.events.length === 0
                                     && backend.status !== "ready" && backend.status !== "error"
                            spacing: 12

                            BusyIndicator {
                                Layout.alignment: Qt.AlignHCenter
                                running: parent.visible
                            }

                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: qsTr("Loading scores…")
                                color: "#8a8a93"
                                font.pixelSize: 12
                            }
                        }

                        ColumnLayout {
                            anchors.centerIn: parent
                            width: Math.min(420, parent.width)
                            visible: backend.events.length === 0 && backend.status === "error"
                            spacing: 10

                            Text {
                                Layout.fillWidth: true
                                text: qsTr("Could not load scores")
                                color: "#f2f2f4"
                                font.pixelSize: 16
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                            }

                            Text {
                                Layout.fillWidth: true
                                text: backend.error
                                color: "#8a8a93"
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
                            visible: backend.events.length === 0 && backend.status === "ready"
                            spacing: 12

                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: "—"
                                color: "#3a3a42"
                                font.pixelSize: 40
                            }

                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: qsTr("No games on this day.")
                                color: "#8a8a93"
                                font.pixelSize: 14
                            }

                            ActionButton {
                                Layout.alignment: Qt.AlignHCenter
                                visible: !backend.isToday
                                text: qsTr("Back to today")
                                primary: true
                                onClicked: backend.today()
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: "#0c0c0e"

            Rectangle {
                anchors.top: parent.top
                width: parent.width
                height: 1
                color: "#1d1d23"
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 8

                Text {
                    text: qsTr("? Help")
                    color: "#6f6f78"
                    font.pixelSize: 11
                }
                Text { text: "·"; color: "#3a3a42"; font.pixelSize: 11 }
                Text {
                    text: qsTr("J / K league")
                    color: "#6f6f78"
                    font.pixelSize: 11
                }
                Text { text: "·"; color: "#3a3a42"; font.pixelSize: 11 }
                Text {
                    text: qsTr("H / L day")
                    color: "#6f6f78"
                    font.pixelSize: 11
                }
                Text { text: "·"; color: "#3a3a42"; font.pixelSize: 11 }
                Text {
                    text: qsTr("R refresh")
                    color: "#6f6f78"
                    font.pixelSize: 11
                }

                Item { Layout.fillWidth: true }

                Text {
                    visible: backend.status === "ready" && backend.updatedLabel.length > 0
                    text: qsTr("Updated at %1").arg(backend.updatedLabel)
                    color: "#6f6f78"
                    font.pixelSize: 11
                }

                Text {
                    text: "ESPN"
                    color: "#4a4a52"
                    font.pixelSize: 11
                }
            }
        }
    }

    HelpOverlay {
        anchors.fill: parent
        visible: win.helpVisible
        onClosed: win.helpVisible = false
    }
}
