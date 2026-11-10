import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Rectangle {
    id: sidebar

    color: theme.panel
    property string query: ""
    property bool searching: search.activeFocus
    signal settingsRequested()

    Material.theme: theme.dark ? Material.Dark : Material.Light
    Material.accent: theme.accent
    Material.roundedScale: theme.radius

    // Every league the user follows, filtered by the search box. The catalog
    // order is the order they appear in, grouped by sport.
    property var model: {
        translator.language
        var enabled = backend.enabledLeagues
        var all = backend.leagues
        var needle = query.trim().toLowerCase()
        var result = []
        for (var i = 0; i < all.length; ++i) {
            var league = all[i]
            if (enabled.indexOf(league.sport + "/" + league.league) < 0)
                continue
            if (needle.length > 0
                    && league.name.toLowerCase().indexOf(needle) < 0
                    && league.sportName.toLowerCase().indexOf(needle) < 0)
                continue
            result.push(league)
        }
        return result
    }

    function positionOf(index) {
        for (var i = 0; i < model.length; ++i)
            if (model[i].index === index)
                return i
        return -1
    }

    function scrollToCurrent() {
        var i = positionOf(backend.leagueIndex)
        if (i >= 0)
            list.positionViewAtIndex(i, ListView.Contain)
    }

    function step(delta) {
        if (model.length === 0)
            return
        var i = positionOf(backend.leagueIndex)
        if (i < 0)
            i = 0
        else
            i = Math.max(0, Math.min(model.length - 1, i + delta))
        backend.selectLeague(model[i].index)
    }

    function focusSearch() {
        search.forceActiveFocus()
        search.selectAll()
    }

    onModelChanged: Qt.callLater(scrollToCurrent)

    Connections {
        target: backend
        function onLeagueChanged() { sidebar.scrollToCurrent() }
    }

    Component.onCompleted: scrollToCurrent()

    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 1
        color: theme.line
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 18
            Layout.rightMargin: 12
            Layout.topMargin: 14
            Layout.bottomMargin: 6
            spacing: 8

            Text {
                Layout.fillWidth: true
                text: qsTr("Leagues")
                color: theme.mutedForeground
                font.pixelSize: theme.fsCaption
                font.bold: true
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 1
            }

            Rectangle {
                Layout.preferredWidth: 22
                Layout.preferredHeight: 22
                radius: Math.min(4, theme.radius)
                color: gearHover.hovered ? theme.card : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "⚙"
                    color: gearHover.hovered ? theme.foreground : theme.mutedForeground
                    font.pixelSize: theme.fsBodyLarge
                }

                HoverHandler { id: gearHover }
                TapHandler { onTapped: sidebar.settingsRequested() }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.preferredHeight: 34

            TextField {
                id: search
                anchors.fill: parent
                placeholderText: qsTr("Search leagues")
                placeholderTextColor: theme.mutedForeground
                color: theme.foreground
                font.pixelSize: theme.fsBodyLarge
                leftPadding: 11
                rightPadding: 30
                selectByMouse: true
                verticalAlignment: TextInput.AlignVCenter

                background: Rectangle {
                    radius: Math.min(6, theme.radius)
                    color: theme.card
                    border.width: 1
                    border.color: search.activeFocus ? theme.accent : theme.line
                }

                onTextChanged: sidebar.query = text

                Keys.onEscapePressed: {
                    text = ""
                    sidebar.query = ""
                    focus = false
                }

                Keys.onReturnPressed: selectFirst()
                Keys.onEnterPressed: selectFirst()

                function selectFirst() {
                    if (sidebar.model.length > 0)
                        backend.selectLeague(sidebar.model[0].index)
                    focus = false
                }
            }

            Rectangle {
                visible: search.text.length === 0 && !search.activeFocus
                anchors.right: parent.right
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                width: 18
                height: 18
                radius: Math.min(4, theme.radius)
                color: theme.panel
                border.width: 1
                border.color: theme.line

                Text {
                    anchors.centerIn: parent
                    text: "/"
                    color: theme.mutedForeground
                    font.pixelSize: theme.fsCaption
                    font.bold: true
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: sidebar.focusSearch()
                }
            }
        }

        Item { Layout.preferredHeight: 8 }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: list
                anchors.fill: parent
                clip: true
                spacing: 1
                boundsBehavior: Flickable.StopAtBounds
                model: sidebar.model
                section.property: "sportName"
                section.criteria: ViewSection.FullString

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                    width: 6
                    background: Rectangle { color: "transparent" }
                    contentItem: Rectangle {
                        implicitWidth: 4
                        radius: 2
                        color: theme.foreground
                        opacity: 0.22
                    }
                }

                section.delegate: Item {
                    width: list.width
                    height: 40

                    Rectangle {
                        anchors.top: parent.top
                        anchors.topMargin: 2
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: parent.width - 24
                        height: 1
                        color: theme.line
                    }

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 18
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 6
                        text: section
                        color: theme.mutedForeground
                        font.pixelSize: theme.fsCaption
                        font.bold: true
                        font.capitalization: Font.AllUppercase
                        font.letterSpacing: 1
                    }
                }

                delegate: Rectangle {
                    id: row
                    required property var modelData

                    readonly property bool current: backend.leagueIndex === modelData.index
                                                     && !backend.liveMode

                    width: list.width
                    height: 34
                    color: "transparent"

                    Rectangle {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 16
                        radius: Math.min(theme.radius, 6)
                        color: row.current ? theme.selection : theme.card
                        opacity: row.current ? 1 : (hover.hovered ? 0.7 : 0)
                        Behavior on opacity {
                            NumberAnimation { duration: 90 }
                        }
                    }

                    Rectangle {
                        visible: row.current
                        anchors.left: parent.left
                        anchors.leftMargin: 4
                        anchors.verticalCenter: parent.verticalCenter
                        width: 3
                        height: 16
                        radius: 1.5
                        color: theme.accent
                    }

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 20
                        anchors.right: parent.right
                        anchors.rightMargin: 24
                        anchors.verticalCenter: parent.verticalCenter
                        text: row.modelData.name
                        color: theme.foreground
                        opacity: row.current ? 1 : 0.82
                        font.pixelSize: theme.fsBodyLarge
                        font.bold: row.current
                        elide: Text.ElideRight
                    }

                    HoverHandler { id: hover }
                    TapHandler { onTapped: backend.selectLeague(row.modelData.index) }
                }
            }

            Text {
                anchors.centerIn: parent
                width: Math.max(120, list.width - 44)
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                visible: list.count === 0
                text: sidebar.query.length > 0
                      ? qsTr("No leagues match “%1”").arg(sidebar.query)
                      : qsTr("No leagues selected")
                color: theme.mutedForeground
                font.pixelSize: theme.fsBody
            }
        }
    }
}
