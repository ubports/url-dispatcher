/**
 * Copyright © 2016 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <sys/types.h>
#include <stdbool.h>

typedef struct _ScopeChecker ScopeChecker;

ScopeChecker * scope_checker_new ();
void scope_checker_delete (ScopeChecker * checker);
bool scope_checker_is_scope (ScopeChecker * checker, const char * appid);
bool scope_checker_is_scope_pid (ScopeChecker * checker, pid_t pid);

