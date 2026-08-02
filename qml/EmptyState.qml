import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GlassPanel {
    id: root

    signal openRequested()

    implicitWidth: 430
    implicitHeight: 286
    radius: 28

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 30
        spacing: 16

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            width: 92
            height: 92
            radius: 22
            color: Theme.dark ? "#f4f3f0" : "#ffffff"
            border.width: 1
            border.color: Theme.glassBorder
            clip: true

            Image {
                anchors.fill: parent
                source: Qt.resolvedUrl("../assets/icons/icon.png")
                fillMode: Image.PreserveAspectFit
                smooth: true
                mipmap: true
            }
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("FrameViewer")
            color: Theme.textPrimary
            font.pixelSize: 22
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
        }

        Label {
            Layout.fillWidth: true
            text: Qt.platform.os === "osx"
                  ? qsTr("Drop a video here or press ⌘O")
                  : qsTr("Drop a video here or press Ctrl+O")
            color: Theme.textSecondary
            font.pixelSize: 14
            horizontalAlignment: Text.AlignHCenter
        }

        PanelButton {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Open Video")
            mediaStyle: false
            accent: true
            onClicked: root.openRequested()
        }
    }
}
