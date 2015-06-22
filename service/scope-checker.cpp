
extern "C" {
#include "scope-checker.h"
}

#include <unity-scopes.h>

ScopeChecker *
scope_checker_new ()
{
	auto uruntime = unity::scopes::Runtime::create();
	auto runtime = uruntime.release();
	return reinterpret_cast<ScopeChecker *>(runtime);
}

void
scope_checker_delete (ScopeChecker * checker)
{
	auto runtime = reinterpret_cast<unity::scopes::Runtime *>(checker);
	delete runtime;
}

int
scope_checker_is_scope (ScopeChecker * checker, const char * appid)
{
	if (checker == nullptr || appid == nullptr)
		return 0;

	auto runtime = reinterpret_cast<unity::scopes::Runtime *>(checker);

	try {
		auto registry = runtime->registry();
		registry->get_metadata(appid);
		return !0;
	} catch (...) {
		return 0;
	}
}
