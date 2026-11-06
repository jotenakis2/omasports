import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: win

    width: 1080
    height: 740
    minimumWidth: 860
    minimumHeight: 480
    visible: true
    title: "omasports"
    color: theme.background

    Material.theme: theme.dark ? Material.Dark : Material.Light
    Material.accent: theme.accent
    Material.roundedScale: theme.radius

    property bool helpVisible: false
    property bool settingsVisible: false
    readonly property bool noSports: backend.enabledSports.length === 0

    function stepLeague(delta) {
        sidebar.step(delta)
    }

    function closeOverlays() {
        win.helpVisible = false
        win.settingsVisible = false
    }

    Shortcut {
        sequences: ["j", "J"]
        context: Qt.ApplicationShortcut
        enabled: !sidebar.searching && !win.helpVisible && !win.settingsVisible
        onActivated: win.stepLeague(1)
    }
    Shortcut {
        sequences: ["k", "K"]
        context: Qt.ApplicationShortcut
        enabled: !sidebar.searching && !win.helpVisible && !win.settingsVisible
        onActivated: win.stepLeague(-1)
    }
    Shortcut {
        sequences: ["Up"]
        context: Qt.ApplicationShortcut
        enabled: !sidebar.searching && !win.helpVisible && !win.settingsVisible
        onActivated: win.stepLeague(-1)
    }
    Shortcut {
        sequences: ["Down"]
        context: Qt.ApplicationShortcut
        enabled: !sidebar.searching && !win.helpVisible && !win.settingsVisible
        onActivated: win.stepLeague(1)
    }
    Shortcut {
        sequences: ["h", "H", "Left"]
        context: Qt.ApplicationShortcut
        enabled: !sidebar.searching && !win.helpVisible && !win.settingsVisible && !backend.liveMode
        onActivated: backend.previousDay()
    }
    Shortcut {
        sequences: ["l", "L", "Right"]
        context: Qt.ApplicationShortcut
        enabled: !sidebar.searching && !win.helpVisible && !win.settingsVisible && !backend.liveMode
        onActivated: backend.nextDay()
    }
    Shortcut {
        sequences: ["t", "T"]
        context: Qt.ApplicationShortcut
        enabled: !sidebar.searching && !win.helpVisible && !win.settingsVisible && !backend.liveMode
        onActivated: backend.today()
    }
    Shortcut {
        sequences: ["d", "D"]
        context: Qt.ApplicationShortcut
        enabled: !sidebar.searching && !win.helpVisible && !win.settingsVisible
        onActivated: backend.toggleLive()
    }
    Shortcut {
        sequences: ["s", "S"]
        context: Qt.ApplicationShortcut
        enabled: !sidebar.searching && !win.helpVisible
        onActivated: win.settingsVisible = !win.settingsVisible
    }
    Shortcut {
        sequences: ["r", "R", "Space"]
        context: Qt.ApplicationShortcut
        enabled: !sidebar.searching && !win.helpVisible && !win.settingsVisible
        onActivated: backend.refresh()
    }
    Shortcut {
        sequence: "/"
        context: Qt.ApplicationShortcut
        enabled: !sidebar.searching && !win.helpVisible && !win.settingsVisible
        onActivated: sidebar.focusSearch()
    }
    Shortcut {
        sequences: ["q", "Q"]
        context: Qt.ApplicationShortcut
        enabled: !sidebar.searching && !win.helpVisible && !win.settingsVisible
        onActivated: Qt.quit()
    }
    Shortcut {
        sequence: "?"
        context: Qt.ApplicationShortcut
        enabled: !sidebar.searching && !win.settingsVisible
        onActivated: win.helpVisible = !win.helpVisible
    }
    Shortcut {
        sequence: "Escape"
        context: Qt.ApplicationShortcut
        enabled: win.helpVisible || win.settingsVisible
        onActivated: win.closeOverlays()
    }

    // Live games move on their own; the live view rescans every league, so it
    // runs more slowly than a single league's scoreboard.
    Timer {
        interval: backend.liveMode ? 60000 : 30000
        repeat: true
        running: (backend.liveMode || backend.liveCount > 0) && win.visible
                 && !win.helpVisible && !win.settingsVisible
        onTriggered: backend.refresh()
    }

    onActiveChanged: {
        if (active && (backend.liveMode || backend.liveCount > 0))
            backend.refresh()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            color: theme.background

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: theme.line
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 12
                spacing: 8

                LogoMark {
                    size: 22
                    Layout.alignment: Qt.AlignVCenter
                }

                Text {
                    text: "omasports"
                    color: theme.foreground
                    font.pixelSize: 15
                    font.bold: true
                    font.letterSpacing: 0.2
                }

                Item { Layout.fillWidth: true }

                RowLayout {
                    visible: !backend.liveMode
                    spacing: 4

                    ActionButton {
                        text: "‹"
                        onClicked: backend.previousDay()
                    }

                    Text {
                        Layout.preferredWidth: 152
                        text: backend.dateLabel
                        color: theme.foreground
                        font.pixelSize: 13
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    ActionButton {
                        text: "›"
                        onClicked: backend.nextDay()
                    }

                    ActionButton {
                        visible: !backend.isToday
                        text: qsTr("Today")
                        primary: true
                        onClicked: backend.today()
                    }
                }

                ActionButton {
                    text: qsTr("Live")
                    primary: backend.liveMode
                    onClicked: backend.toggleLive()
                }

                RowLayout {
                    visible: backend.liveCount > 0 && !backend.liveMode
                    spacing: 6

                    Rectangle {
                        Layout.alignment: Qt.AlignVCenter
                        width: 7
                        height: 7
                        radius: 3.5
                        color: theme.accent

                        SequentialAnimation on opacity {
                            running: backend.liveCount > 0 && !backend.liveMode
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
                onSettingsRequested: win.settingsVisible = true
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.topMargin: 24
                    anchors.bottomMargin: 16
                    width: Math.min(840, parent.width - 40)
                    spacing: 14

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3

                        Text {
                            Layout.fillWidth: true
                            text: backend.liveMode ? qsTr("Live")
                                  : win.noSports ? qsTr("Sports")
                                  : backend.league.name || ""
                            color: theme.foreground
                            font.pixelSize: 26
                            font.bold: true
                            elide: Text.ElideRight
                        }

                        Text {
                            Layout.fillWidth: true
                            text: {
                                translator.language
                                if (backend.liveMode)
                                    return qsTr("%n games in progress", "", backend.liveCount)
                                if (win.noSports)
                                    return qsTr("Choose the sports you want to follow.")
                                var count = backend.events.length
                                return backend.status === "ready" && count > 0
                                       ? backend.league.sportName + "  ·  " + qsTr("%n games", "", count)
                                       : backend.league.sportName
                            }
                            color: theme.mutedForeground
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32
                        visible: backend.status === "error" && backend.events.length > 0
                        color: theme.card
                        radius: Math.min(6, theme.radius)
                        clip: true

                        Rectangle {
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            width: 3
                            color: theme.danger
                        }

                        Text {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 10
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Refresh failed: %1").arg(backend.error)
                            color: theme.danger
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
                            section.property: backend.liveMode ? "sectionLabel" : ""
                            section.criteria: ViewSection.FullString

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

                            section.delegate: Item {
                                width: games.width - 12
                                height: 34

                                Rectangle {
                                    anchors.top: parent.top
                                    anchors.topMargin: 2
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    width: parent.width - 12
                                    height: 1
                                    color: theme.line
                                }

                                Text {
                                    anchors.left: parent.left
                                    anchors.bottom: parent.bottom
                                    anchors.bottomMargin: 4
                                    text: section
                                    color: theme.mutedForeground
                                    font.pixelSize: 10
                                    font.bold: true
                                    font.capitalization: Font.AllUppercase
                                    font.letterSpacing: 1
                                }
                            }

                            delegate: EventCard {
                                width: games.width - 12
                            }
                        }

                        ColumnLayout {
                            anchors.centerIn: parent
                            visible: !win.noSports && backend.events.length === 0
                                     && backend.status !== "ready" && backend.status !== "error"
                            spacing: 12

                            BusyIndicator {
                                Layout.alignment: Qt.AlignHCenter
                                running: parent.visible
                            }

                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: backend.liveMode ? qsTr("Scanning every league…")
                                                       : qsTr("Loading scores…")
                                color: theme.mutedForeground
                                font.pixelSize: 12
                            }
                        }

                        ColumnLayout {
                            anchors.centerIn: parent
                            width: Math.min(420, parent.width)
                            visible: !win.noSports && backend.events.length === 0
                                     && backend.status === "error"
                            spacing: 10

                            Text {
                                Layout.fillWidth: true
                                text: qsTr("Could not load scores")
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
                            visible: !win.noSports && !backend.liveMode && backend.events.length === 0
                                     && backend.status === "ready"
                            spacing: 12

                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: "—"
                                color: theme.mutedForeground
                                font.pixelSize: 40
                            }

                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: qsTr("No games on this day.")
                                color: theme.mutedForeground
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

                        ColumnLayout {
                            anchors.centerIn: parent
                            visible: !win.noSports && backend.liveMode && backend.events.length === 0
                                     && backend.status === "ready"
                            spacing: 12

                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: "—"
                                color: theme.mutedForeground
                                font.pixelSize: 40
                            }

                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: qsTr("No games in progress right now.")
                                color: theme.mutedForeground
                                font.pixelSize: 14
                            }

                            ActionButton {
                                Layout.alignment: Qt.AlignHCenter
                                text: qsTr("Refresh")
                                primary: true
                                onClicked: backend.refresh()
                            }
                        }

                        ColumnLayout {
                            anchors.centerIn: parent
                            width: Math.min(380, parent.width)
                            visible: win.noSports
                            spacing: 10

                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: qsTr("No sports selected")
                                color: theme.foreground
                                font.pixelSize: 16
                                font.bold: true
                            }

                            Text {
                                Layout.fillWidth: true
                                text: qsTr("Choose the sports you want to follow.")
                                color: theme.mutedForeground
                                font.pixelSize: 12
                                wrapMode: Text.WordWrap
                                horizontalAlignment: Text.AlignHCenter
                            }

                            ActionButton {
                                Layout.alignment: Qt.AlignHCenter
                                text: qsTr("Choose sports")
                                primary: true
                                onClicked: win.settingsVisible = true
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: theme.background

            Rectangle {
                anchors.top: parent.top
                width: parent.width
                height: 1
                color: theme.line
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 8

                Text {
                    text: qsTr("? Help")
                    color: theme.mutedForeground
                    font.pixelSize: 11
                }
                Text { text: "·"; color: theme.line; font.pixelSize: 11 }
                Text {
                    text: qsTr("J / K league")
                    color: theme.mutedForeground
                    font.pixelSize: 11
                }
                Text { text: "·"; color: theme.line; font.pixelSize: 11 }
                Text {
                    text: qsTr("H / L day")
                    color: theme.mutedForeground
                    font.pixelSize: 11
                }
                Text { text: "·"; color: theme.line; font.pixelSize: 11 }
                Text {
                    text: qsTr("D live")
                    color: theme.mutedForeground
                    font.pixelSize: 11
                }
                Text { text: "·"; color: theme.line; font.pixelSize: 11 }
                Text {
                    text: qsTr("S sports")
                    color: theme.mutedForeground
                    font.pixelSize: 11
                }

                Item { Layout.fillWidth: true }

                Text {
                    visible: backend.status === "ready" && backend.updatedLabel.length > 0
                    text: qsTr("Updated at %1").arg(backend.updatedLabel)
                    color: theme.mutedForeground
                    font.pixelSize: 11
                }

                Text {
                    text: "ESPN"
                    color: theme.mutedForeground
                    opacity: 0.65
                    font.pixelSize: 11
                }
            }
        }
    }

    SettingsOverlay {
        anchors.fill: parent
        visible: win.settingsVisible
        onClosed: win.settingsVisible = false
    }

    HelpOverlay {
        anchors.fill: parent
        visible: win.helpVisible
        onClosed: win.helpVisible = false
    }
}
