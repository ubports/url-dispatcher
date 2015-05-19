
#ifndef MIR_MOCK_H
#define MIR_MOCK_H 1

#include <string>
#include <utility>

#include <mir_toolkit/mir_connection.h>
#include <mir_toolkit/mir_prompt_session.h>

void mir_mock_connect_return_valid (bool valid);
std::pair<std::string, std::string> mir_mock_connect_last_connect (void);
void mir_mock_set_trusted_fd (int fd);

extern pid_t mir_mock_last_trust_pid;
extern MirPromptSession * mir_mock_last_released_session;

#endif // MIR_MOCK_H
