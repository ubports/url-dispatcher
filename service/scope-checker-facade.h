
#pragma once

#include <unity-scopes.h>

class RuntimeFacade {
public:
	RuntimeFacade ();
	virtual ~RuntimeFacade ();
	virtual unity::scopes::RegistryProxy registry () = 0;
};
