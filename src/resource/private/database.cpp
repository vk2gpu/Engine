#include "resource/private/database.h"

#include "core/file.h"
#include "core/map.h"
#include "core/set.h"
#include "core/uuid.h"

#include <utility>
#include <string.h>

namespace Resource
{
	struct DatabaseImpl
	{
		struct Entry
		{
			/// Resource name.
			Core::Vector<char> name_;
			/// Resource dependencies.
			Core::Set<Core::UUID> dependencies_;
		};

		Core::Map<Core::UUID, Entry> entries_;

		/**
		 * Recurse dependencies and call the specified function for each.
		 */
		template<typename CALLABLE>
		void RecurseDependencies(Core::UUID start, CALLABLE callable)
		{
			auto it = entries_.find(start);
			if(it != entries_.end())
			{
				for(auto dep : it->second.dependencies_)
				{
					callable(dep);
					RecurseDependencies(dep, callable);
				}
			}
		}
	};


	Database::Database()
	{ //
		impl_ = new DatabaseImpl();
	}

	Database::~Database()
	{ //
		delete impl_;
	}

	bool Database::AddResource(Core::UUID& outResourceUuid, const char* name)
	{
		DBG_ASSERT(impl_);
		Core::UUID resourceUuid(name);

		auto it = impl_->entries_.find(resourceUuid);

		// Add if missing.
		if(it == impl_->entries_.end())
		{
			it = impl_->entries_.insert(resourceUuid, DatabaseImpl::Entry());
			it->second.name_.resize((i32)strlen(name) + 1);
			strcpy_s(it->second.name_.data(), it->second.name_.size(), name);
		}

		// Check that name matches.
		if(strcmp(name, it->second.name_.data()) != 0)
		{
			char uuidStr[38];
			resourceUuid.AsString(uuidStr);
			DBG_LOG("Resource UUID %s collision. \"%s\" expected, \"%s\" is stored.\n", uuidStr, name,
			    it->second.name_.data());
			return false;
		}

		outResourceUuid = resourceUuid;
		return true;
	}

	bool Database::AddDependencies(const Core::UUID& resourceUuid, const Core::UUID* depUuids, i32 numDeps)
	{
		DBG_ASSERT(impl_);

		auto it = impl_->entries_.find(resourceUuid);
		if(it == impl_->entries_.end())
		{
			char uuidStr[38];
			resourceUuid.AsString(uuidStr);
			DBG_LOG("Resource UUID %s is invalid.\n", uuidStr);
			return false;
		}

		bool haveFailure = false;
		for(i32 i = 0; i < numDeps; ++i)
		{
			const Core::UUID& depUuid = depUuids[i];

			auto depIt = impl_->entries_.find(depUuid);
			if(depIt == impl_->entries_.end())
			{
				char uuidStr[38];
				depUuid.AsString(uuidStr);
				DBG_LOG("Resource UUID %s is invalid.\n", uuidStr);
				haveFailure = true;
				continue;
			}

			// Check the dependency does not reference the resource it is to become a dependency of
			// anywhere in the hierarchy.
			bool haveCircularDep = false;
			impl_->RecurseDependencies(depUuid,
			    [resourceUuid, &haveCircularDep](Core::UUID& uuid) { haveCircularDep |= (uuid == resourceUuid); });
			haveFailure |= haveCircularDep;

			if(haveCircularDep)
			{
				char uuidStr1[38];
				char uuidStr2[38];
				resourceUuid.AsString(uuidStr1);
				depUuid.AsString(uuidStr2);
				DBG_LOG("Resource UUID %s (%s) already references UUID %s (%s)\n", uuidStr1, it->second.name_.data(),
				    uuidStr2, depIt->second.name_.data());
			}
			else
			{
				it->second.dependencies_.insert(depUuid);
			}
		}
		return !haveFailure;
	}


} // namespace Resource
