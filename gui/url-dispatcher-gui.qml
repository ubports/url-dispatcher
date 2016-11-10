import QtQuick 2.4
import Ubuntu.Components 1.3
import Ubuntu.Components.ListItems 1.3

MainView {
	applicationName: "url-dispatcher-gui"

	Page {
		header: PageHeader {
			title: "URL Dispatcher GUI"
			flickable: flickme
		}

		Flickable {
			id: flickme
			anchors.fill: parent

			Column {
				anchors.fill: parent

				ListItem {
					contentItem.anchors {
						leftMargin: units.gu(2)
						rightMargin: units.gu(2)
						topMargin: units.gu(1)
						bottomMargin: units.gu(1)
					}

					TextField {
						id: textbox
						anchors.fill: parent
						placeholderText: "URL (e.g. 'http://ubuntu.com')"
					}
				}

				ListItem {
					contentItem.anchors {
						leftMargin: units.gu(2)
						rightMargin: units.gu(2)
						topMargin: units.gu(1)
						bottomMargin: units.gu(1)
					}

					Button {
						anchors.fill: parent
						text: "Send URL"
						onClicked: {
							console.log("Sending URL: " + textbox.text)
							Qt.openUrlExternally(textbox.text)
						}
					}
				}
			}
		}

	}


}
