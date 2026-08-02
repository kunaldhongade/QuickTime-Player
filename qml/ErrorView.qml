import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GlassPanel {
    id: root

    required property string filename
    required property string message
    signal openRequested()

    implicitHeight: content.implicitHeight + 54
    radius: 24

    ColumnLayout {
        id: content
        anchors.fill: parent
        anchors.margins: 27
        spacing: 12

        Label {
            Layout.fillWidth: true
            text: root.filename.length > 0
                  ? qsTr("Couldn’t open %1").arg(root.filename)
                  : qsTr("FrameViewer couldn’t start")
            color: Theme.textPrimary
            font.pixelSize: 19
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }

        Label {
            Layout.fillWidth: true
            text: root.message
            color: Theme.textSecondary
            font.pixelSize: 14
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }

        PanelButton {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Choose Another Video")
            mediaStyle: false
            accent: true
            onClicked: root.openRequested()
        }
    }
}
