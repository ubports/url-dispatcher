
#include <unity-scopes.h>
#include <unity/scopes/testing/ScopeMetadataBuilder.h>

#include "scope-checker-facade.h"

class unity::scopes::internal::ScopeMetadataImpl {
public:
	ScopeMetadataImpl () { }
	virtual ~ScopeMetadataImpl () {}

};

class RegistryMock : public unity::scopes::RegistryProxy
{
	unity::scopes::testing::ScopeMetadataBuilder builder;

public:
	RegistryMock ( ) {
	}

	unity::scopes::ScopeMetadata get_metadata(std::string const& scope_id) {
		if (scope_id == "throw") {
			throw new unity::scopes::NotFoundException("foo", "bar");
		}
		return builder();
	}

};

class RuntimeMock : public RuntimeFacade
{
public:
	unity::scopes::RegistryProxy registry () {
		return RegistryMock();
	}
};

