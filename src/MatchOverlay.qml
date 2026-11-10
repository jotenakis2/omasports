import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// The full sheet for one match: the score, the scorers, the cards, and the
// key statistics. It loads ESPN's summary on demand, so opening a card does
// not slow the scoreboard down.
Item {
    id: overlay

    signal closed()

    Material.theme: theme.dark ? Material.Dark : Material.Light
    Material.accent: theme.accent
    Material.roundedScale: theme.radius

    readonly property var teams: backend.match.teams || []
    readonly property var goals: backend.match.goals || []
    readonly property var cards: backend.match.cards || []
    readonly property var stats: backend.match.stats || []

    function team(home) {
        for (var i = 0; i < teams.length; ++i)
            if (teams[i].home === home)
                return teams[i]
        return null
    }

    readonly property var homeTeam: team(true) || (teams.length > 1 ? teams[1] : teams[0])
    readonly property var awayTeam: team(false) || (teams.length > 0 ? teams[0] : null)

    function statusText() {
        if (backend.match.state === "post")
            return qsTr("Final")
        if (backend.match.state === "in")
            return backend.match.shortDetail || backend.match.detail || qsTr("Live")
        var start = new Date(backend.match.start)
        if (!isNaN(start.getTime()))
            return start.toLocaleString(Qt.locale(), Locale.ShortFormat)
        return backend.match.shortDetail || ""
    }

    function goalNote(goal) {
        if (goal.ownGoal)
            return qsTr("Own goal")
        if (goal.penalty)
            return qsTr("Penalty")
        var type = goal.type || ""
        if (type.indexOf("Header") >= 0)
            return qsTr("Header")
        if (type.indexOf("Volley") >= 0)
            return qsTr("Volley")
        return ""
    }

    function cardLabel(card) {
        return card.red ? qsTr("Red card") : qsTr("Yellow card")
    }

    function recordLabel(team) {
        if (!team)
            return ""
        translator.language
        return backend.recordLabel(backend.match.sport || backend.league.sport, team.record)
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
        width: Math.min(640, parent.width - 48)
        height: Math.min(680, parent.height - 64)
        radius: theme.radius
        color: theme.card
        border.width: 1
        border.color: theme.line

        // Swallow clicks on the sheet itself.
        MouseArea {
            anchors.fill: parent
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 22
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Match")
                    color: theme.foreground
                    font.pixelSize: theme.fsSubtitleLarge
                    font.bold: true
                }

                ActionButton {
                    text: qsTr("Refresh")
                    onClicked: backend.refreshMatch()
                }

                ActionButton {
                    text: qsTr("Close")
                    primary: true
                    onClicked: overlay.closed()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 14

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    TeamBadge {
                        team: overlay.homeTeam
                        size: 34
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        Layout.fillWidth: true
                        text: overlay.homeTeam ? overlay.homeTeam.name : ""
                        color: theme.foreground
                        font.pixelSize: theme.fsName
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                    }

                    Text {
                        Layout.fillWidth: true
                        text: overlay.recordLabel(overlay.homeTeam)
                        color: theme.mutedForeground
                        font.pixelSize: theme.fsCaption
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                ColumnLayout {
                    spacing: 2

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: {
                            if (backend.match.state === "pre")
                                return "–"
                            var home = overlay.homeTeam ? overlay.homeTeam.score : ""
                            var away = overlay.awayTeam ? overlay.awayTeam.score : ""
                            return home + " – " + away
                        }
                        color: theme.foreground
                        font.family: theme.monoFamily
                        font.pixelSize: theme.fsHero
                        font.bold: true
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: overlay.statusText()
                        color: backend.match.state === "in" ? theme.accent : theme.mutedForeground
                        font.pixelSize: theme.fsSmall
                        font.bold: backend.match.state === "in"
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        visible: text.length > 0
                        text: backend.match.venue || ""
                        color: theme.mutedForeground
                        font.pixelSize: theme.fsCaption
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    TeamBadge {
                        team: overlay.awayTeam
                        size: 34
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        Layout.fillWidth: true
                        text: overlay.awayTeam ? overlay.awayTeam.name : ""
                        color: theme.foreground
                        font.pixelSize: theme.fsName
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                    }

                    Text {
                        Layout.fillWidth: true
                        text: overlay.recordLabel(overlay.awayTeam)
                        color: theme.mutedForeground
                        font.pixelSize: theme.fsCaption
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: theme.line
            }

            Flickable {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: width
                contentHeight: details.implicitHeight
                boundsBehavior: Flickable.StopAtBounds

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

                ColumnLayout {
                    id: details
                    width: parent.width - 12
                    spacing: 14

                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: backend.matchStatus !== "ready" && backend.matchStatus !== "error"
                        spacing: 8

                        BusyIndicator {
                            Layout.alignment: Qt.AlignHCenter
                            running: parent.visible
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("Loading match details…")
                            color: theme.mutedForeground
                            font.pixelSize: theme.fsSmall
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: backend.matchStatus === "error"
                        spacing: 8

                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Could not load match details")
                            color: theme.foreground
                            font.pixelSize: theme.fsName
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Text {
                            Layout.fillWidth: true
                            text: backend.matchError
                            color: theme.mutedForeground
                            font.pixelSize: theme.fsSmall
                            wrapMode: Text.WordWrap
                            horizontalAlignment: Text.AlignHCenter
                        }

                        ActionButton {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("Retry")
                            primary: true
                            onClicked: backend.refreshMatch()
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: backend.matchStatus === "ready" && overlay.goals.length > 0
                        spacing: 6

                        Text {
                            text: qsTr("Goals")
                            color: theme.mutedForeground
                            font.pixelSize: theme.fsCaption
                            font.bold: true
                            font.capitalization: Font.AllUppercase
                            font.letterSpacing: 1
                        }

                        Repeater {
                            model: overlay.goals

                            delegate: RowLayout {
                                required property var modelData

                                Layout.fillWidth: true
                                spacing: 8

                                Text {
                                    Layout.preferredWidth: 46
                                    text: modelData.clock
                                    color: theme.mutedForeground
                                    font.pixelSize: theme.fsSmall
                                }

                                Text {
                                    text: "⚽"
                                    font.pixelSize: theme.fsSmall
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 0

                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.scorer
                                        color: theme.foreground
                                        font.pixelSize: theme.fsBody
                                        elide: Text.ElideRight
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        visible: text.length > 0
                                        text: overlay.goalNote(modelData)
                                        color: theme.mutedForeground
                                        font.pixelSize: theme.fsCaption
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        visible: text.length > 0
                                        text: modelData.assist
                                                  ? qsTr("Assist: %1").arg(modelData.assist) : ""
                                        color: theme.mutedForeground
                                        font.pixelSize: theme.fsCaption
                                        elide: Text.ElideRight
                                    }
                                }

                                Text {
                                    text: modelData.teamAbbreviation || modelData.teamName || ""
                                    color: theme.mutedForeground
                                    font.pixelSize: theme.fsCaption
                                    font.bold: true
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: backend.matchStatus === "ready" && overlay.cards.length > 0
                        spacing: 6

                        Text {
                            text: qsTr("Cards")
                            color: theme.mutedForeground
                            font.pixelSize: theme.fsCaption
                            font.bold: true
                            font.capitalization: Font.AllUppercase
                            font.letterSpacing: 1
                        }

                        Repeater {
                            model: overlay.cards

                            delegate: RowLayout {
                                required property var modelData

                                Layout.fillWidth: true
                                spacing: 8

                                Text {
                                    Layout.preferredWidth: 46
                                    text: modelData.clock
                                    color: theme.mutedForeground
                                    font.pixelSize: theme.fsSmall
                                }

                                Rectangle {
                                    width: 7
                                    height: 11
                                    radius: 1
                                    color: modelData.red ? "#e5484d" : "#e7c400"
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.player
                                    color: theme.foreground
                                    font.pixelSize: theme.fsBody
                                    elide: Text.ElideRight
                                }

                                Text {
                                    text: overlay.cardLabel(modelData)
                                    color: theme.mutedForeground
                                    font.pixelSize: theme.fsCaption
                                }

                                Text {
                                    text: modelData.teamAbbreviation || modelData.teamName || ""
                                    color: theme.mutedForeground
                                    font.pixelSize: theme.fsCaption
                                    font.bold: true
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: backend.matchStatus === "ready" && overlay.stats.length > 0
                        spacing: 6

                        Text {
                            text: qsTr("Statistics")
                            color: theme.mutedForeground
                            font.pixelSize: theme.fsCaption
                            font.bold: true
                            font.capitalization: Font.AllUppercase
                            font.letterSpacing: 1
                        }

                        Repeater {
                            model: overlay.stats

                            delegate: RowLayout {
                                required property var modelData

                                Layout.fillWidth: true
                                spacing: 10

                                Text {
                                    Layout.preferredWidth: 62
                                    text: modelData.home
                                    color: theme.foreground
                                    font.pixelSize: theme.fsSmall
                                    font.bold: true
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: qsTr(modelData.label)
                                    color: theme.mutedForeground
                                    font.pixelSize: theme.fsSmall
                                    horizontalAlignment: Text.AlignHCenter
                                    elide: Text.ElideRight
                                }

                                Text {
                                    Layout.preferredWidth: 62
                                    text: modelData.away
                                    color: theme.foreground
                                    font.pixelSize: theme.fsSmall
                                    font.bold: true
                                    horizontalAlignment: Text.AlignRight
                                }
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: backend.matchStatus === "ready"
                                 && overlay.goals.length === 0
                                 && overlay.cards.length === 0
                                 && overlay.stats.length === 0
                        text: backend.match.state === "pre"
                              ? qsTr("Match not started.")
                              : backend.match.state === "in"
                                ? qsTr("No details yet.")
                                : qsTr("No details for this match.")
                        color: theme.mutedForeground
                        font.pixelSize: theme.fsSmall
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }
    }
}
