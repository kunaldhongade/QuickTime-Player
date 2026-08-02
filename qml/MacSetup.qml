import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root

    required property var controller

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(560, parent ? parent.width - 48 : 560)
    height: Math.min(570, parent ? parent.height - 48 : 570)
    modal: true
    dim: true
    closePolicy: Popup.NoAutoClose
    visible: controller.macSetupVisible
    padding: 0

    Overlay.modal: Rectangle {
        color: Theme.dark ? "#7a000000" : "#520c1524"
    }

    background: GlassPanel {
        radius: 28
    }

    contentItem: ColumnLayout {
        anchors.fill: parent
        spacing: 14
        anchors.margins: 30

        Image {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 88
            Layout.preferredHeight: 88
            source: Qt.resolvedUrl("../assets/icons/icon.png")
            fillMode: Image.PreserveAspectFit
            smooth: true
            mipmap: true
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("Welcome to FrameViewer")
            color: Theme.textPrimary
            font.pixelSize: 25
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("Finish the Mac setup, then open supported videos directly from Finder.")
            color: Theme.textSecondary
            font.pixelSize: 14
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 92
            radius: 17

            RowLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 14

                Label {
                    text: root.controller.installedInApplications ? "✓" : "1"
                    color: root.controller.installedInApplications ? "#42c977" : Theme.textPrimary
                    font.pixelSize: 20
                    font.weight: Font.Bold
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 3
                    Label {
                        text: qsTr("Install in Applications")
                        color: Theme.textPrimary
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                    }
                    Label {
                        Layout.fillWidth: true
                        text: root.controller.installedInApplications
                              ? qsTr("FrameViewer is installed correctly.")
                              : qsTr("Drag FrameViewer from the installer into Applications, then reopen it.")
                        color: Theme.textSecondary
                        font.pixelSize: 12
                        wrapMode: Text.Wrap
                    }
                }
                PanelButton {
                    text: qsTr("Open Folder")
                    mediaStyle: false
                    visible: !root.controller.installedInApplications
                    onClicked: root.controller.openApplicationsFolder()
                }
            }
        }

        GlassPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 106
            radius: 17

            RowLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 14

                Label {
                    text: root.controller.defaultVideoPlayer ? "✓" : "2"
                    color: root.controller.defaultVideoPlayer ? "#42c977" : Theme.textPrimary
                    font.pixelSize: 20
                    font.weight: Font.Bold
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 3
                    Label {
                        text: qsTr("Open videos with FrameViewer")
                        color: Theme.textPrimary
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                    }
                    Label {
                        Layout.fillWidth: true
                        text: root.controller.defaultVideoPlayer
                              ? qsTr("FrameViewer is registered as your video player.")
                              : qsTr("Make FrameViewer the preferred viewer for common video formats. You can change this later in Finder.")
                        color: Theme.textSecondary
                        font.pixelSize: 12
                        wrapMode: Text.Wrap
                    }
                }
                PanelButton {
                    text: root.controller.defaultVideoPlayer ? qsTr("Configured") : qsTr("Make Default")
                    mediaStyle: false
                    accent: !root.controller.defaultVideoPlayer
                    enabled: root.controller.installedInApplications
                             && !root.controller.defaultVideoPlayer
                    onClicked: root.controller.makeDefaultVideoPlayer()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Label {
                Layout.fillWidth: true
                text: Theme.dark ? qsTr("Appearance: Dark · follows macOS")
                                 : qsTr("Appearance: Light · follows macOS")
                color: Theme.textSecondary
                font.pixelSize: 12
            }

            PanelButton {
                text: qsTr("Continue")
                mediaStyle: false
                accent: true
                onClicked: root.controller.finishMacSetup()
            }
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("Tip: For one file type only, select a video in Finder, choose Get Info, then use Open with → Change All.")
            color: Theme.textSecondary
            opacity: 0.82
            font.pixelSize: 11
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }
    }
}
