/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
			title: i18n.tr("Unrecognized Address")
			text: i18n.tr("Ubuntu can't open addresses of type “%1”.").arg(Qt.application.arguments[2].split(':')[0])

			Button {
				text: i18n.tr("OK")
				color: UbuntuColors.orange
				onClicked: Qt.quit()
			}
		}
	}

	Component.onCompleted: PopupUtils.open(dialog)
}
