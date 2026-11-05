import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Rectangle {
    id: sidebar

    color: "#111114"
    property string query: ""
    property bool searching: search.activeFocus

    // Every league in the catalog, filtered by the search box. The catalog
    // order is the order they appear in, grouped by sport.
    property var model: {
        translator.language
        var all = backend.leagues
        var needle = query.trim().toLowerCase()
        var result = []
        for (var i = 0; i < all.length; ++i) {
            var league = all[i]
            if (needle.length === 0
                    || league.name.toLowerCase().indexOf(needle) >= 0
                    || league.sportName.toLowerCase().indexOf(needle) >= 0)
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

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Text {
            Layout.fillWidth: true
            Layout.leftMargin: 18
            Layout.topMargin: 16
            Layout.bottomMargin: 8
            text: qsTr("Leagues")
            color: "#77777f"
            font.pixelSize: 10
            font.bold: true
            font.capitalization: Font.AllUppercase
            font.letterSpacing: 1
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
                placeholderTextColor: "#6f6f78"
                color: "#f2f2f4"
                font.pixelSize: 13
                leftPadding: 11
                rightPadding: 30
                selectByMouse: true
                verticalAlignment: TextInput.AlignVCenter

                background: Rectangle {
                    radius: Math.min(6, theme.radius)
                    color: "#17171b"
                    border.width: 1
                    border.color: search.activeFocus ? theme.accent : "#2a2a31"
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
                color: "#232329"

                Text {
                    anchors.centerIn: parent
                    text: "/"
                    color: "#8a8a93"
                    font.pixelSize: 10
                    font.bold: true
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: sidebar.focusSearch()
                }
            }
        }

        Item { Layout.preferredHeight: 10 }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 1
            boundsBehavior: Flickable.StopAtBounds
            model: sidebar.model
            section.property: "sportName"
            section.criteria: ViewSection.FullString
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            section.delegate: Item {
                width: list.width
                height: 30

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 18
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 2
                    text: section
                    color: "#77777f"
                    font.pixelSize: 10
                    font.bold: true
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: 1
                }
            }

            delegate: Rectangle {
                id: row
                required property var modelData

                readonly property bool current: backend.leagueIndex === modelData.index

                width: list.width
                height: 34
                color: "transparent"

                Rectangle {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    radius: Math.min(theme.radius, 6)
                    color: theme.accent
                    opacity: row.current ? 0.16 : (hover.hovered ? 0.05 : 0)
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
                    anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    text: row.modelData.name
                    color: row.current ? "#ffffff" : "#c9c9cf"
                    font.pixelSize: 13
                    font.bold: row.current
                    elide: Text.ElideRight
                }

                HoverHandler { id: hover }
                TapHandler { onTapped: backend.selectLeague(row.modelData.index) }
            }
        }
    }
}
