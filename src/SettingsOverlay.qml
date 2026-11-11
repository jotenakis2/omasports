import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// Pick which leagues appear in the sidebar and in the live view. A sport row
// toggles every league under it; a league row toggles one. Changes land
// immediately and are saved by the backend.
Item {
    id: overlay

    signal closed()

    // The backend replaces settingsRows on every toggle, which resets the
    // ListView to the top. Keep the user on the row they just clicked.
    function withPreservedScroll(action) {
        var y = choices.contentY
        action()
        choices.contentY = y
        Qt.callLater(function() { choices.contentY = y })
    }

    function toggleSport(sport, enabled) {
        withPreservedScroll(function() { backend.setSportLeaguesEnabled(sport, enabled) })
    }

    function toggleLeague(sport, league, enabled) {
        withPreservedScroll(function() { backend.setLeagueEnabled(sport, league, enabled) })
    }

    function setAllLeagues(enabled) {
        withPreservedScroll(function() { backend.setAllLeaguesEnabled(enabled) })
    }

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
        width: Math.min(480, parent.width - 48)
        height: Math.min(620, parent.height - 64)
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
                    text: qsTr("Settings")
                    color: theme.foreground
                    font.pixelSize: theme.fsSubtitleLarge
                    font.bold: true
                }
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("Choose the leagues you want to follow.")
                color: theme.mutedForeground
                font.pixelSize: theme.fsSmall
                wrapMode: Text.WordWrap
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: theme.line
            }

            ListView {
                id: choices
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 1
                boundsBehavior: Flickable.StopAtBounds
                model: backend.settingsRows

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

                delegate: Item {
                    id: choiceRow
                    required property var modelData
                    required property int index
                    readonly property bool sport: modelData.type === "sport"
                    readonly property bool checked: sport ? modelData.allEnabled : modelData.enabled
                    readonly property bool partial: sport && modelData.enabledCount > 0
                                                     && !modelData.allEnabled

                    width: choices.width - 10
                    height: sport ? 38 : 34

                    Rectangle {
                        // The section separator already sits above the first
                        // sport; only later groups need their own lighter rule.
                        visible: choiceRow.sport && choiceRow.index > 0
                        anchors.top: parent.top
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: parent.width - 24
                        height: 1
                        color: theme.line
                        opacity: 0.35
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: Math.min(6, theme.radius)
                        color: theme.selection
                        opacity: hover.hovered ? 0.55 : 0
                        Behavior on opacity {
                            NumberAnimation { duration: 90 }
                        }
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: choiceRow.sport ? 8 : 28
                        anchors.rightMargin: 10
                        spacing: 10

                        Rectangle {
                            Layout.preferredWidth: 16
                            Layout.preferredHeight: 16
                            Layout.alignment: Qt.AlignVCenter
                            radius: Math.min(4, theme.radius)
                            color: choiceRow.checked ? theme.accent : "transparent"
                            border.width: 1
                            border.color: (choiceRow.checked || choiceRow.partial)
                                          ? theme.accent
                                          : theme.line

                            Text {
                                anchors.centerIn: parent
                                visible: choiceRow.checked || choiceRow.partial
                                text: choiceRow.partial ? "–" : "✓"
                                color: choiceRow.checked ? theme.accentForeground : theme.accent
                                font.pixelSize: theme.fsSmall
                                font.bold: true
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: choiceRow.modelData.name
                            color: theme.foreground
                            font.pixelSize: choiceRow.sport ? theme.fsSmall : theme.fsBodyLarge
                            font.bold: choiceRow.sport
                            font.capitalization: choiceRow.sport
                                                 ? Font.AllUppercase : Font.MixedCase
                            font.letterSpacing: choiceRow.sport ? 0.8 : 0
                            elide: Text.ElideRight
                        }

                        Text {
                            visible: choiceRow.sport
                            text: qsTr("%n leagues", "", Number(choiceRow.modelData.leagues))
                            color: theme.mutedForeground
                            font.pixelSize: theme.fsCaption
                        }
                    }

                    HoverHandler { id: hover }
                    TapHandler {
                        onTapped: {
                            if (choiceRow.sport) {
                                overlay.toggleSport(choiceRow.modelData.sport,
                                                    !choiceRow.modelData.allEnabled)
                            } else {
                                overlay.toggleLeague(choiceRow.modelData.sport,
                                                     choiceRow.modelData.league,
                                                     !choiceRow.modelData.enabled)
                            }
                        }
                    }
                }
            }

            Flow {
                Layout.fillWidth: true
                spacing: 8

                ActionButton {
                    text: qsTr("Show all leagues")
                    onClicked: overlay.setAllLeagues(true)
                }

                ActionButton {
                    text: qsTr("Hide all leagues")
                    onClicked: overlay.setAllLeagues(false)
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: theme.line
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                Text {
                    text: qsTr("Text size")
                    color: theme.mutedForeground
                    font.pixelSize: theme.fsSmall
                    font.bold: true
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 8

                    ActionButton {
                        text: qsTr("Small")
                        primary: backend.fontScaleLevel === 0
                        onClicked: backend.setFontScale(0)
                    }

                    ActionButton {
                        text: qsTr("Compact")
                        primary: backend.fontScaleLevel === 1
                        onClicked: backend.setFontScale(1)
                    }

                    ActionButton {
                        text: qsTr("Normal")
                        primary: backend.fontScaleLevel === 2
                        onClicked: backend.setFontScale(2)
                    }

                    ActionButton {
                        text: qsTr("Large")
                        primary: backend.fontScaleLevel === 3
                        onClicked: backend.setFontScale(3)
                    }

                    ActionButton {
                        text: qsTr("Very large")
                        primary: backend.fontScaleLevel === 4
                        onClicked: backend.setFontScale(4)
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                Text {
                    text: qsTr("League logos")
                    color: theme.mutedForeground
                    font.pixelSize: theme.fsSmall
                    font.bold: true
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 8

                    ActionButton {
                        text: qsTr("Sidebar logos")
                        primary: backend.sidebarLogos
                        onClicked: backend.setSidebarLogos(!backend.sidebarLogos)
                    }

                    ActionButton {
                        text: qsTr("Header logos")
                        primary: backend.headerLogos
                        onClicked: backend.setHeaderLogos(!backend.headerLogos)
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
