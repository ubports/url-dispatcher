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

#include "mir-mock.h"

#include <iostream>
#include <thread>

MirPromptSession * mir_mock_valid_trust_session = (MirPromptSession *)"In the circle of trust";
static bool valid_trust_connection = true;
static int trusted_fd = 1234;
MirPromptSession * mir_mock_last_released_session = NULL;
pid_t mir_mock_last_trust_pid = 0;
void (*mir_mock_last_trust_func)(MirPromptSession *, MirPromptSessionState, void*data) = NULL;
void * mir_mock_last_trust_data = NULL;

MirPromptSession *
mir_connection_create_prompt_session_sync(MirConnection * connection, pid_t pid, void (*func)(MirPromptSession *, MirPromptSessionState, void*data), void * context) {
	mir_mock_last_trust_pid = pid;
	mir_mock_last_trust_func = func;
	mir_mock_last_trust_data = context;

	if (valid_trust_connection) {
		return mir_mock_valid_trust_session;
	} else {
		return nullptr;
	}
}

void
mir_prompt_session_release_sync (MirPromptSession * session)
{
	mir_mock_last_released_session = session;
	if (session != mir_mock_valid_trust_session) {
		std::cerr << "Releasing a Mir Trusted Prompt that isn't valid" << std::endl;
		exit(1);
	}
}

MirWaitHandle *
mir_prompt_session_new_fds_for_prompt_providers (MirPromptSession * session, unsigned int numfds, mir_client_fd_callback cb, void * data) {
	if (session != mir_mock_valid_trust_session) {
		std::cerr << "Releasing a Mir Trusted Prompt that isn't valid" << std::endl;
		exit(1);
	}

	/* TODO: Put in another thread to be more mir like */
	std::thread * thread = new std::thread([session, numfds, cb, data]() {
		int fdlist[numfds];

		for (unsigned int i = 0; i < numfds; i++) 
			fdlist[i] = trusted_fd;

		cb(session, numfds, fdlist, data);
	});

	return reinterpret_cast<MirWaitHandle *>(thread);
}

void
mir_wait_for (MirWaitHandle * wait)
{
	auto thread = reinterpret_cast<std::thread *>(wait);

	if (thread->joinable())
		thread->join();

	delete thread;
}

static const char * valid_connection_str = "Valid Mir Connection";
static std::pair<std::string, std::string> last_connection;
static bool valid_connection = true;

void
mir_mock_connect_return_valid (bool valid)
{
	valid_connection = valid;
}

std::pair<std::string, std::string>
mir_mock_connect_last_connect (void)
{
	return last_connection;
}

MirConnection *
mir_connect_sync (char const * server, char const * appname)
{
	last_connection = std::pair<std::string, std::string>(server, appname);

	if (valid_connection) {
		return (MirConnection *)(valid_connection_str);
	} else {
		return nullptr;
	}
}

void
mir_connection_release (MirConnection * con)
{
	if (reinterpret_cast<char *>(con) != valid_connection_str) {
		std::cerr << "Releasing a Mir Connection that isn't valid" << std::endl;
		exit(1);
	}
}

void mir_mock_set_trusted_fd (int fd)
{
	trusted_fd = fd;
}
