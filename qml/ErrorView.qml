import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root

    required property string filename
    required property string message
    signal openRequested()

    spacing: 12

    Label {
        Layout.fillWidth: true
        text: filename.length > 0
              ? qsTr("Couldn’t open %1").arg(filename)
              : qsTr("FrameViewer couldn’t start")
        color: "white"
        font.pixelSize: 19
        font.weight: Font.DemiBold
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
    }

    Label {
        Layout.fillWidth: true
        text: root.message
        color: "#bdbdbd"
        font.pixelSize: 14
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
    }

    Button {
        Layout.alignment: Qt.AlignHCenter
        text: qsTr("Choose Another Video")
        onClicked: root.openRequested()
    }
}
