import QtQuick
import QtQuick.Layouts

// One game: the state on the left, both sides with their scores, and for
// tournaments and races, which have no two sides, the event itself.
Rectangle {
    id: card

    required property var modelData

    readonly property bool live: modelData.state === "in"
    readonly property bool paired: modelData.teams && modelData.teams.length === 2

    implicitHeight: paired ? 84 : 68
    radius: theme.radius
    color: theme.card

    Rectangle {
        visible: card.live
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: 12
        anchors.bottomMargin: 12
        width: 3
        radius: 1.5
        color: theme.accent
    }

    RowLayout {
        visible: card.paired
        anchors.fill: parent
        anchors.leftMargin: 18
        anchors.rightMargin: 16
        anchors.topMargin: 10
        anchors.bottomMargin: 10
        spacing: 14

        Item {
            Layout.preferredWidth: 78
            Layout.fillHeight: true

            Column {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                spacing: 3

                Text {
                    width: parent.width
                    text: card.statusText()
                    horizontalAlignment: Text.AlignHCenter
                    color: card.live ? theme.accent : theme.mutedForeground
                    font.pixelSize: 11
                    font.bold: card.live
                    wrapMode: Text.WordWrap
                }

                Text {
                    visible: card.live
                    width: parent.width
                    text: qsTr("Live")
                    horizontalAlignment: Text.AlignHCenter
                    color: theme.accent
                    font.pixelSize: 9
                    font.bold: true
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 6

            Repeater {
                model: card.modelData.teams

                delegate: RowLayout {
                    required property var modelData

                    Layout.fillWidth: true
                    spacing: 9

                    TeamBadge {
                        team: modelData
                        Layout.preferredWidth: 24
                        Layout.preferredHeight: 24
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        Text {
                            Layout.fillWidth: true
                            text: modelData.name
                            color: theme.foreground
                            font.pixelSize: 14
                            elide: Text.ElideRight
                        }

                        Text {
                            Layout.fillWidth: true
                            visible: text.length > 0
                            text: modelData.record
                            color: theme.mutedForeground
                            font.pixelSize: 10
                            elide: Text.ElideRight
                        }
                    }

                    Text {
                        Layout.preferredWidth: 46
                        Layout.alignment: Qt.AlignVCenter
                        horizontalAlignment: Text.AlignRight
                        text: card.modelData.state === "pre" ? "" : modelData.score
                        color: modelData.winner ? theme.accent : theme.foreground
                        font.pixelSize: 20
                        font.bold: true
                    }
                }
            }
        }
    }

    ColumnLayout {
        visible: !card.paired
        anchors.fill: parent
        anchors.margins: 15
        spacing: 4

        Text {
            Layout.fillWidth: true
            text: card.modelData.name
            color: theme.foreground
            font.pixelSize: 15
            font.bold: true
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            text: card.detailText()
            color: card.live ? theme.accent : theme.mutedForeground
            font.pixelSize: 11
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            visible: text.length > 0
            text: card.modelData.venue
            color: theme.mutedForeground
            font.pixelSize: 10
            elide: Text.ElideRight
        }
    }

    function statusText() {
        if (modelData.state === "post")
            return qsTr("Final")
        if (modelData.state === "in")
            return detailText()
        return startTime()
    }

    function detailText() {
        var detail = modelData.shortDetail || modelData.detail
        if (!detail)
            return modelData.state === "in" ? qsTr("Live") : ""
        if (detail === "Halftime")
            return qsTr("Halftime")
        return detail
    }

    function startTime() {
        // The clock format follows the language, so re-read it on a switch.
        translator.language
        var start = new Date(modelData.start)
        if (isNaN(start.getTime()))
            return modelData.shortDetail || ""
        var time = start.toLocaleTimeString(Qt.locale(), Locale.ShortFormat)
        // ESPN groups a day by US time, so a late game can start on the next
        // local day; name that day rather than show an hour out of context.
        if (isoDay(start) === backend.dateIso)
            return time
        return start.toLocaleDateString(Qt.locale(), "ddd") + " " + time
    }

    function isoDay(value) {
        var month = value.getMonth() + 1
        var day = value.getDate()
        return value.getFullYear() + "-" + (month < 10 ? "0" : "") + month + "-"
                + (day < 10 ? "0" : "") + day
    }
}
