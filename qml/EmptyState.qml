import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root
    signal openRequested()

    spacing: 18

    Rectangle {
        Layout.alignment: Qt.AlignHCenter
        width: 58
        height: 48
        radius: 12
        color: "#151515"
        border.width: 1
        border.color: "#2e2e2e"

        Image {
            anchors.centerIn: parent
            width: 25
            height: 25
            source: Qt.resolvedUrl("../assets/icons/open.svg")
        }
    }

    Label {
        Layout.alignment: Qt.AlignHCenter
        text: Qt.platform.os === "osx"
              ? qsTr("Drop a video here or press ⌘O")
              : qsTr("Drop a video here or press Ctrl+O")
        color: "#c9c9c9"
        font.pixelSize: 16
    }

    Button {
        Layout.alignment: Qt.AlignHCenter
        text: qsTr("Open Video")
        onClicked: root.openRequested()

        contentItem: Text {
            text: parent.text
            color: "white"
            font.pixelSize: 14
            font.weight: Font.Medium
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            implicitWidth: 112
            implicitHeight: 38
            radius: 9
            color: parent.down ? "#3277bf" : parent.hovered ? "#438ed7" : "#3a82cc"
        }
    }
}
