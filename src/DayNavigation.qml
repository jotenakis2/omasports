import QtQuick
import QtQuick.Layouts

// Day navigation row: previous, date label, next, Today.
// The row and its Today button stay visible and usable in every view.
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
        onClicked: root.today()
    }
}
