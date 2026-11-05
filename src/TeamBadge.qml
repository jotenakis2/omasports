import QtQuick

// The crest, with the abbreviation behind it while the logo loads or when the
// feed has none. The initials switch between black and white to stay readable
// on the team color.
Item {
    id: badge

    property var team
    property int size: 24

    implicitWidth: size
    implicitHeight: size

    readonly property string teamColor: team && team.color ? "#" + team.color : theme.selection
    readonly property bool lightColor: {
        var color = probe.color
        return 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b > 0.55
    }

    Rectangle {
        id: probe
        visible: false
        width: 0
        height: 0
        color: badge.teamColor
    }

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: badge.teamColor
    }

    Text {
        anchors.centerIn: parent
        text: badge.initials()
        visible: logo.status !== Image.Ready
        color: badge.lightColor ? "black" : "white"
        font.pixelSize: Math.max(8, badge.size * 0.34)
        font.bold: true
    }

    Image {
        id: logo
        anchors.fill: parent
        source: badge.team && badge.team.logo ? badge.team.logo : ""
        sourceSize: Qt.size(64, 64)
        fillMode: Image.PreserveAspectFit
        asynchronous: true
        cache: true
    }

    function initials() {
        if (!team)
            return ""
        if (team.abbreviation)
            return team.abbreviation
        var words = (team.name || "").split(" ")
        var letters = ""
        for (var i = 0; i < words.length && letters.length < 3; ++i)
            if (words[i].length > 0)
                letters += words[i][0]
        return letters.toUpperCase()
    }
}
