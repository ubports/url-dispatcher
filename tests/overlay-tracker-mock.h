/**
 * Copyright © 2015 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 3, as published by
 * the Free Software Foundation.
 * * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once
#include "overlay-tracker-iface.h"
#include <utility>

class OverlayTrackerMock : public OverlayTrackerIface
{
	public:
		std::vector<std::tuple<std::string, unsigned long, std::string>> addedOverlays;

		bool addOverlay (const char * appid, unsigned long pid, const char * url) {
			addedOverlays.push_back(std::make_tuple(std::string(appid), pid, std::string(url)));
			return true;
		}
};
