
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
	std::shared_ptr<std::map<std::string, std::exception>> scopeExceptions;

public:
	RegistryMock (std::shared_ptr<std::map<std::string, std::exception>> inexceptions)
		: scopeExceptions(inexceptions)
	{
	}

	unity::scopes::ScopeMetadata get_metadata(std::string const& scope_id) {
		try {
			auto exp = (*scopeExceptions)[scope_id];
			throw exp;
		} catch (std::out_of_range e) {
			return builder();
		}
	}

};

class RuntimeMock : public RuntimeFacade
{
	std::shared_ptr<std::map<std::string, std::exception>> scopeExceptions;

public:
	unity::scopes::RegistryProxy registry () {
		return RegistryMock(scopeExceptions);
	}

	void clearExceptions() {
		scopeExceptions->clear();
	}

	void addException (std::string scopeid, std::exception exp) {
		(*scopeExceptions)[scopeid] = exp;
	}
};

