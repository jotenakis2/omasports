import QtQuick
import QtQuick.Layouts

// Day navigation row: previous, date label, next, Today.
// "Today" stays in the layout at all times so the date box never moves.
RowLayout {
    id: root

    property string dateLabel: ""
    property bool isToday: false

    signal previous()
    signal next()
    signal today()

    spacing: 4

    ActionButton {
        id: prevButton
        objectName: "prevButton"
        text: "‹"
        onClicked: root.previous()
    }

    Text {
        id: dateText
        objectName: "dateText"
        Layout.preferredWidth: theme.dayBoxWidth
        text: root.dateLabel
        color: theme.foreground
        font.pixelSize: theme.fsBodyLarge
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        maximumLineCount: 1
    }

    ActionButton {
        id: nextButton
        objectName: "nextButton"
        text: "›"
        onClicked: root.next()
    }

    ActionButton {
        id: todayButton
        objectName: "todayButton"
        text: qsTr("Today")
        primary: true
        opacity: root.isToday ? 0 : 1
        enabled: !root.isToday
        onClicked: root.today()
    }
}
