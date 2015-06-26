/**
 * Copyright © 2015 Canonical, Ltd.
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

#include <unity-scopes.h>
#include <unity/scopes/testing/ScopeMetadataBuilder.h>

#include "scope-checker-facade.h"

class unity::scopes::internal::ScopeMetadataImpl {
public:
	ScopeMetadataImpl () { }
	virtual ~ScopeMetadataImpl () {}

};

class RegistryMock : public virtual unity::scopes::Registry
{
	std::shared_ptr<std::map<std::string, std::exception>> scopeExceptions;

public:
	RegistryMock (std::shared_ptr<std::map<std::string, std::exception>> inexceptions)
		: scopeExceptions(inexceptions)
	{
	}

	unity::scopes::ScopeMetadata get_metadata(std::string const& scope_id) override {
		std::cout << "Getting Metadata" << std::endl;
		try {
			auto exp = scopeExceptions->at(scope_id);
			throw exp;
		} catch (std::out_of_range e) {
			unity::scopes::testing::ScopeMetadataBuilder builder;
			builder.scope_id(scope_id);
			std::shared_ptr<unity::scopes::Scope> foo((unity::scopes::Scope *)5, [](unity::scopes::Scope *) { return; });
			builder.proxy(foo);
			builder.display_name("foo");
			builder.description("foo");
			builder.author("foo");
			return builder();
		}
	}

	/* I hate C++ so much right now */
	std::string endpoint() override { throw new std::invalid_argument("Not implemented"); }
	std::string identity() override { throw new std::invalid_argument("Not implemented"); }
	std::string target_category() override { throw new std::invalid_argument("Not implemented"); }
	std::string to_string() override { throw new std::invalid_argument("Not implemented"); }
	int64_t timeout() override { throw new std::invalid_argument("Not implemented"); }
	unity::scopes::MetadataMap list() override { throw new std::invalid_argument("Not implemented"); }
	unity::scopes::MetadataMap list_if(std::function<bool(unity::scopes::ScopeMetadata const& item)>) override { throw new std::invalid_argument("Not implemented"); }
	bool is_scope_running(std::string const&) override { throw new std::invalid_argument("Not implemented"); }
	core::ScopedConnection set_scope_state_callback(std::string const&, std::function<void(bool is_running)>) override { throw new std::invalid_argument("Not implemented"); }
	core::ScopedConnection set_list_update_callback(std::function<void()>) override { throw new std::invalid_argument("Not implemented"); }
};

class RuntimeMock : public RuntimeFacade
{
	std::shared_ptr<std::map<std::string, std::exception>> scopeExceptions;

public:
	RuntimeMock() {
		scopeExceptions = std::make_shared<std::map<std::string, std::exception>>();
	}

	unity::scopes::RegistryProxy registry () {
		return std::make_shared<RegistryMock>(scopeExceptions);
	}

	void clearExceptions() {
		scopeExceptions->clear();
	}

	void addException (std::string scopeid, std::exception exp) {
		(*scopeExceptions)[scopeid] = exp;
	}
};

