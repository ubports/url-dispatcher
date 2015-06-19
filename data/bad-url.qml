
import QtQuick 2.0
import Ubuntu.Components 1.0
import Ubuntu.Components.Popups 1.0

MainView {
	applicationName: "com.canonical.url-dispatcher.bad-url-prompt"
	automaticOrientation: true
	backgroundColor: "transparent"
	anchors.fill: parent

	Component {
		id: dialog
		Dialog {
			title: "Unrecognized Address"
			text: "Ubuntu can't open addresses of type \"\"."

			Button {
				text: "OK"
				color: UbuntuColors.orange
				onClicked: Qt.quit()
			}
		}
	}

	Component.onCompleted: PopupUtils.open(dialog)
}
