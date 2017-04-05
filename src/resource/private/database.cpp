#include "resource/private/database.h"

#include "core/file.h"
#include "core/map.h"
#include "core/misc.h"
#include "core/set.h"
#include "core/uuid.h"

#include <utility>
#include <string.h>

namespace Resource
{
	struct DatabaseImpl
	{
		using DependencySet = Core::Set<Core::UUID>;

		/**
		 * A single resource entry.
		 */
		struct Entry
		{
			/// Resource name.
			Core::Vector<char> name_;
			/// Resource data.
			void* data_ = nullptr;

			/// Resource which this entry is dependent upon.
			DependencySet dependencies_;
			/// Resource which depend on this resource.
			DependencySet dependents_;
		};

		Core::Map<Core::UUID, Entry> entries_;

		/**
		 * Recurse dependencies and call the specified function for each.
		 */
		template<typename CALLABLE>
		void RecurseDependencies(const Core::UUID& start, CALLABLE callable)
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

		/**
		 * Recurse dependents and call the specified function for each.
		 */
		template<typename CALLABLE>
		void RecurseDependents(const Core::UUID& start, CALLABLE callable)
		{
			auto it = entries_.find(start);
			if(it != entries_.end())
			{
				for(auto dep : it->second.dependents_)
				{
					callable(dep);
					RecurseDependents(dep, callable);
				}
			}
		}

		Entry* GetEntry(const Core::UUID& resourceUuid)
		{
			auto it = entries_.find(resourceUuid);
			if(it == entries_.end())
			{
				char uuidStr[38];
				resourceUuid.AsString(uuidStr);
				DBG_LOG("Resource UUID %s is invalid.\n", uuidStr);
				return nullptr;
			}
			return &it->second;
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

	bool Database::RemoveResource(const Core::UUID& resourceUuid, bool removeDependents)
	{
		auto entry = impl_->GetEntry(resourceUuid);
		if(entry)
		{
			i32 numDeps =
			    Core::Max(GetDependents(nullptr, 0, resourceUuid), GetDependencies(nullptr, 0, resourceUuid, false));
			Core::Vector<Core::UUID> deps;
			deps.resize(numDeps);

			// Gather resources depenent upon this resource.
			numDeps = GetDependents(deps.data(), deps.size(), resourceUuid);
			RemoveDependency(resourceUuid, deps.data(), numDeps);
			if(removeDependents)
				for(const auto& dep : deps)
					RemoveResource(dep, removeDependents);

			// Remove direct dependencies.
			numDeps = GetDependencies(deps.data(), deps.size(), resourceUuid, false);
			for(auto dep : deps)
				RemoveDependency(dep, &resourceUuid, 1);

			// Remove this resource.
			impl_->entries_.erase(impl_->entries_.find(resourceUuid));

			return true;
		}
		return false;
	}

	bool Database::AddDependencies(const Core::UUID& resourceUuid, const Core::UUID* depUuids, i32 numDeps)
	{
		DBG_ASSERT(impl_);

		auto entry = impl_->GetEntry(resourceUuid);
		if(entry == nullptr)
			return false;

		bool haveFailure = false;
		for(i32 i = 0; i < numDeps; ++i)
		{
			const Core::UUID& depUuid = depUuids[i];

			auto depEntry = impl_->GetEntry(depUuid);
			if(depEntry == nullptr)
			{
				haveFailure = true;
				continue;
			}

			// Check the dependency does not reference the resource it is to become a dependency of
			// anywhere in the hierarchy.
			bool haveCircularDep = false;
			impl_->RecurseDependencies(depUuid,
			    [resourceUuid, &haveCircularDep](Core::UUID& uuid) { haveCircularDep |= (uuid == resourceUuid); });
			impl_->RecurseDependents(
			    resourceUuid, [depUuid, &haveCircularDep](Core::UUID& uuid) { haveCircularDep |= (uuid == depUuid); });

			haveFailure |= haveCircularDep;

			if(haveCircularDep)
			{
				char uuidStr1[38];
				char uuidStr2[38];
				resourceUuid.AsString(uuidStr1);
				depUuid.AsString(uuidStr2);
				DBG_LOG("Resource UUID %s (%s) is a dependency of UUID %s (%s)\n", uuidStr1, entry->name_.data(),
				    uuidStr2, depEntry->name_.data());
			}
			else
			{
				entry->dependencies_.insert(depUuid);
				depEntry->dependents_.insert(resourceUuid);
			}
		}
		return !haveFailure;
	}

	bool Database::RemoveDependency(const Core::UUID& depUuid, const Core::UUID* resourceUuids, i32 numResources)
	{
		auto depEntry = impl_->GetEntry(depUuid);
		if(depEntry == nullptr)
			return false;

		for(i32 i = 0; i < numResources; ++i)
		{
			const auto& resourceUuid = resourceUuids[i];
			auto resourceEntry = impl_->GetEntry(resourceUuid);
			DBG_ASSERT(resourceEntry);

			auto it = resourceEntry->dependencies_.find(depUuid);
			if(it != resourceEntry->dependencies_.end())
				resourceEntry->dependencies_.erase(it);

			it = depEntry->dependents_.find(resourceUuid);
			if(it != depEntry->dependents_.end())
				depEntry->dependents_.erase(it);
		}
		return true;
	}

	i32 Database::GetDependencies(
	    Core::UUID* outDeps, i32 maxDeps, const Core::UUID& resourceUuid, bool recursivelyGather) const
	{
		DBG_ASSERT(impl_);
		Core::Set<Core::UUID> gatheredDeps;
		auto entry = impl_->GetEntry(resourceUuid);
		if(entry == nullptr)
			return 0;
		for(auto dep : entry->dependencies_)
			gatheredDeps.insert(dep);

		if(recursivelyGather)
		{
			impl_->RecurseDependencies(resourceUuid, [this, &gatheredDeps](Core::UUID& uuid) {
				auto entry = impl_->GetEntry(uuid);
				DBG_ASSERT(entry);
				for(auto dep : entry->dependencies_)
					gatheredDeps.insert(dep);
			});
		}

		if(outDeps)
		{
			auto it = gatheredDeps.begin();
			for(i32 i = 0; i < Core::Min(maxDeps, gatheredDeps.size()); ++i)
				outDeps[i] = *it++;
		}

		return gatheredDeps.size();
	}

	i32 Database::GetDependents(Core::UUID* outDeps, i32 maxDeps, const Core::UUID& resourceUuid) const
	{
		DBG_ASSERT(impl_);
		Core::Set<Core::UUID> gatheredDeps;
		auto entry = impl_->GetEntry(resourceUuid);
		if(entry == nullptr)
			return 0;

		for(auto dep : entry->dependents_)
			gatheredDeps.insert(dep);

		impl_->RecurseDependents(resourceUuid, [this, &gatheredDeps](Core::UUID& uuid) {
			auto entry = impl_->GetEntry(uuid);
			DBG_ASSERT(entry);
			for(auto dep : entry->dependents_)
				gatheredDeps.insert(dep);
		});

		if(outDeps)
		{
			auto it = gatheredDeps.begin();
			for(i32 i = 0; i < Core::Min(maxDeps, gatheredDeps.size()); ++i)
				outDeps[i] = *it++;
		}

		return gatheredDeps.size();
	}

	bool Database::SetResourceData(void* inData, const Core::UUID& resourceUuid)
	{
		DBG_ASSERT(impl_);
		auto entry = impl_->GetEntry(resourceUuid);
		if(entry == nullptr)
			return false;

		entry->data_ = inData;
		return true;
	}

	bool Database::GetResourceData(void** outData, const Core::UUID& resourceUuid) const
	{
		DBG_ASSERT(impl_);

		auto entry = impl_->GetEntry(resourceUuid);
		if(entry == nullptr)
			return false;

		if(outData)
			*outData = entry->data_;
		return true;
	}


} // namespace Resource
