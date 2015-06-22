
#pragma once

#include <sys/types.h>

typedef struct _ScopeChecker ScopeChecker;

ScopeChecker * scope_checker_new ();
void scope_checker_delete (ScopeChecker * checker);
int scope_checker_is_scope (ScopeChecker * checker, const char * appid);
int scope_checker_is_scope_pid (ScopeChecker * checker, pid_t pid);

