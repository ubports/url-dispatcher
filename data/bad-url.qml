
import QtQuick 2.0
import Ubuntu.Components 1.0
import Ubuntu.Components.Popups 1.0

MainView {
	applicationName: "com.canonical.url-dispatcher.bad-url-prompt"
	automaticOrientation: true
	backgroundColor: "transparent"

	Component {
		id: dialog
		Dialog {
			title: "Bad URL"
			text: "This won't work"

			Button {
				text: "Sucks doesn't it"
				color: UbuntuColors.orange
				onClicked: qt.quit()
			}
		}
	}

	Component.onCompleted: PopupUtils.open(dialog)
}
