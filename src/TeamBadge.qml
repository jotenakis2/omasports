import QtQuick

// The team or athlete crest, with the abbreviation as a fallback while the
// logo loads or when the feed has none.
Item {
    id: badge

    property var team
    property int size: 24

    implicitWidth: size
    implicitHeight: size

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: badge.team && badge.team.color ? "#" + badge.team.color : "#2a2a31"
    }

    Text {
        anchors.centerIn: parent
        text: badge.initials()
        visible: logo.status !== Image.Ready
        color: "white"
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
