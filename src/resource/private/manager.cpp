#pragma once

#include "resource/manager.h"
#include "resource/converter.h"
#include "resource/factory.h"
#include "resource/private/converter_context.h"
#include "resource/private/factory_context.h"
#include "resource/private/path_resolver.h"
#include "resource/private/jobs_fileio.h"

#include "core/array.h"
#include "core/concurrency.h"
#include "core/file.h"
#include "core/library.h"
#include "core/map.h"
#include "core/misc.h"
#include "core/mpmc_bounded_queue.h"
#include "core/string.h"
#include "core/uuid.h"
#include "core/timer.h"

#include "job/basic_job.h"
#include "job/concurrency.h"
#include "job/manager.h"
#include "plugin/manager.h"
#include "serialization/serializer.h"

#include <algorithm>
#include <utility>

namespace Resource
{
	// TODO: Remove this and rely upon Resource::Database perhaps?
	struct ResourceEntry
	{
		void* resource_ = nullptr;
		Core::String sourceFile_;
		Core::String convertedFile_;
		Core::UUID name_;
		Core::UUID type_;
		volatile i32 converting_ = 0;
		volatile i32 loaded_ = 0;
		volatile i32 refCount_ = 0;

		/// @return If resource is out of date and needs reimporting.
		bool ResourceOutOfDate(Core::IFilePathResolver* pathResolver) const
		{
			Core::FileTimestamp sourceTimestamp;
			Core::FileTimestamp convertedTimestamp;
			bool sourceExists = false;
			if(pathResolver)
			{
				Core::Array<char, Core::MAX_PATH_LENGTH> resolvedSourcePath = {};
				pathResolver->ResolvePath(sourceFile_.c_str(), resolvedSourcePath.data(), resolvedSourcePath.size());
				sourceExists = Core::FileStats(resolvedSourcePath.data(), nullptr, &sourceTimestamp, nullptr);
			}
			else
			{
				sourceExists &= Core::FileStats(sourceFile_.c_str(), nullptr, &sourceTimestamp, nullptr);
			}
			bool convertedExists = Core::FileStats(convertedFile_.c_str(), nullptr, &convertedTimestamp, nullptr);
			if(sourceExists && convertedExists)
				return convertedTimestamp < sourceTimestamp;
			return !convertedExists;
		}
	};

	using ResourceList = Core::Vector<ResourceEntry*>;
}

namespace Core
{
	u32 Hash(u32 input, const Core::Pair<Core::UUID, Core::UUID>& pair)
	{
		return HashCRC32(input, &pair, sizeof(pair));
	}

	u32 Hash(u32 input, Resource::ResourceEntry* resourceEntry)
	{
		return HashCRC32(input, &resourceEntry, sizeof(resourceEntry));
	}
} // namespace Core

namespace Resource
{
	struct ResourceLoadJob;
	struct ResourceConvertJob;
	struct ResourceTimestampJob;

	struct ManagerImpl
	{
		static const i32 MAX_READ_JOBS = 128;
		static const i32 MAX_WRITE_JOBS = 128;

		/// Plugins.
		Core::Vector<ConverterPlugin> converterPlugins_;

		/// Read job queue.
		Core::MPMCBoundedQueue<FileIOJob> readJobs_;
		/// Signalled when read job is waiting.
		Core::Semaphore readJobSem_;
		/// Thread to use for blocking reads.
		Core::Thread readThread_;

		/// Write job queue.
		Core::MPMCBoundedQueue<FileIOJob> writeJobs_;
		/// Signalled when write job is waiting.
		Core::Semaphore writeJobSem_;
		/// Thread to use for blocking reads.
		Core::Thread writeThread_;

		/// Path resolver.
		PathResolver pathResolver_;

		/// Is resource manager active? true from initialize, false at finalize.
		bool isActive_ = false;

		/// Number of conversions running.
		volatile i32 numConversionJobs_ = 0;
		volatile i32 numReloadJobs_ = 0;

		/// Resources.
		volatile i32 pendingResourceJobs_ = 0;
		ResourceList resourceList_;
		ResourceList releasedResourceList_;
		Job::RWLock resourceRWLock_;

		// Read/write lock used to allow reloading logic to wait until it's safe,
		// and to be blocked whilst everything is ticking.
		Job::RWLock reloadRWLock_;

		void AcquireResourceEntry(ResourceEntry* entry)
		{
			DBG_ASSERT(entry);
			Core::AtomicInc(&entry->refCount_);
		}

		void UnsafeDoReleaseResourceEntry(ResourceEntry* entry)
		{
			releasedResourceList_.push_back(entry);
			auto it = std::find_if(resourceList_.begin(), resourceList_.end(),
			    [entry](ResourceEntry* listEntry) { return entry == listEntry; });
			DBG_ASSERT(it != resourceList_.end());
			resourceList_.erase(it);
		}

		bool ReleaseResourceEntry(ResourceEntry* entry)
		{
			if(Core::AtomicDec(&entry->refCount_) == 0)
			{
				Job::ScopedWriteLock lock(resourceRWLock_);
				UnsafeDoReleaseResourceEntry(entry);
				return true;
			}
			return false;
		}

		ResourceEntry* AcquireResourceEntry(const char* sourceFile, const char* convertedFile, const Core::UUID& type)
		{
			Job::ScopedWriteLock lock(resourceRWLock_);
			ResourceEntry* entry = nullptr;
			Core::UUID name = sourceFile;
			auto it =
			    std::find_if(resourceList_.begin(), resourceList_.end(), [&name, &type](ResourceEntry* listEntry) {
				    return listEntry->name_ == name && listEntry->type_ == type;
				});
			if(it == resourceList_.end())
			{
				// Add resource to db.
				entry = new ResourceEntry();
				entry->sourceFile_ = sourceFile;
				entry->convertedFile_ = convertedFile;
				entry->name_ = name;
				entry->type_ = type;
				resourceList_.push_back(entry);
			}
			else
			{
				entry = *it;
			}
			DBG_ASSERT(entry);
			Core::AtomicInc(&entry->refCount_);
			return entry;
		}

		/// @return true if this was the last reference.
		bool ReleaseResourceEntry(void* resource, const Core::UUID& type)
		{
			Job::ScopedWriteLock lock(resourceRWLock_);
			auto it =
			    std::find_if(resourceList_.begin(), resourceList_.end(), [resource, &type](ResourceEntry* listEntry) {
				    return listEntry->resource_ == resource && listEntry->type_ == type;
				});
			DBG_ASSERT(it != resourceList_.end());
			UnsafeDoReleaseResourceEntry(*it);
			return true;
		}

		/// @return if resource is ready.
		bool IsResourceReady(void* resource, const Core::UUID& type)
		{
			Job::ScopedWriteLock lock(resourceRWLock_);
			auto it =
			    std::find_if(resourceList_.begin(), resourceList_.end(), [resource, &type](ResourceEntry* listEntry) {
				    return listEntry->resource_ == resource && listEntry->type_ == type;
				});
			DBG_ASSERT(it != resourceList_.end());
			return (*it)->loaded_ != 0;
		}

		/// Factories.
		using Factories = Core::Map<Core::UUID, IFactory*>;
		Factories factories_;
		Core::Mutex factoriesMutex_;

		IFactory* GetFactory(const Core::UUID& type)
		{
			Core::ScopedMutex lock(factoriesMutex_);
			auto it = factories_.find(type);
			if(it == factories_.end())
			{
				char uuidStr[38];
				type.AsString(uuidStr);
				DBG_LOG("Fsctory does not exist for type %s\n", uuidStr);
				return nullptr;
			}
			return it->second;
		}


		void ProcessReleasedResources()
		{
			ResourceList releasedResourceList;
			{
				Job::ScopedWriteLock lock(resourceRWLock_);
				releasedResourceList = releasedResourceList_;
				releasedResourceList_.clear();
			}

			FactoryContext factoryContext;

			for(auto entry : releasedResourceList)
			{
				DBG_ASSERT(entry->loaded_);
				if(auto factory = GetFactory(entry->type_))
				{
					bool retVal = factory->DestroyResource(factoryContext, &entry->resource_, entry->type_);
					delete entry;
					DBG_ASSERT(retVal);
				}
			}
		}

		ManagerImpl()
		    : readJobs_(MAX_READ_JOBS)
		    , readJobSem_(0, MAX_READ_JOBS, "Resource Manager Read Semamphore")
		    , readThread_(ReadIOThread, this, 65536, "Resource Manager Read Thread")
		    , writeJobs_(MAX_WRITE_JOBS)
		    , writeJobSem_(0, MAX_WRITE_JOBS, "Resource Manager Write Semaphore")
		    , writeThread_(WriteIOThread, this, 65536, "Resource Manager Write Thread")
		    , isActive_(true)
		{
			// Get converter plugins.
			i32 found = Plugin::Manager::GetPlugins<ConverterPlugin>(nullptr, 0);
			converterPlugins_.resize(found);
			Plugin::Manager::GetPlugins<ConverterPlugin>(converterPlugins_.data(), converterPlugins_.size());

			// From current working directory find the "res" directory to add to the path resolver.
			Core::String currRelativePath = "res";
			while(Core::FileExists(currRelativePath.c_str()) == false)
			{
				currRelativePath.Printf("../%s", currRelativePath.c_str());

				DBG_ASSERT_MSG(currRelativePath.size() < Core::MAX_PATH_LENGTH, "Unable to find \'res\' directory!");
			}

			pathResolver_.AddPath(".");
			pathResolver_.AddPath(currRelativePath.c_str());
		}

		~ManagerImpl()
		{
			// No longer active. Any pending jobs will complete.
			isActive_ = false;

			// Wait for pending resource jobs to complete.
			while(pendingResourceJobs_ > 0)
			{
				Job::Manager::YieldCPU();
			}

			ProcessReleasedResources();

			// TODO: Mark jobs as cancelled.
			while(readJobs_.Enqueue(FileIOJob()) == false)
				Job::Manager::YieldCPU();
			readJobSem_.Signal(1);
			readThread_.Join();

			while(writeJobs_.Enqueue(FileIOJob()) == false)
				Job::Manager::YieldCPU();
			writeJobSem_.Signal(1);
			writeThread_.Join();
		}

		static int ReadIOThread(void* userData)
		{
			auto* impl = reinterpret_cast<ManagerImpl*>(userData);

			FileIOJob ioJob;
			for(;;)
			{
				if(impl->readJobs_.Dequeue(ioJob))
				{
					if(ioJob.file_ != nullptr)
						ioJob.DoRead();
					else
						return 0;
				}
				impl->readJobSem_.Wait();
			}
		}

		static int WriteIOThread(void* userData)
		{
			auto* impl = reinterpret_cast<ManagerImpl*>(userData);

			FileIOJob ioJob;
			for(;;)
			{
				if(impl->writeJobs_.Dequeue(ioJob))
				{
					if(ioJob.file_ != nullptr)
						ioJob.DoWrite();
					else
						return 0;
				}
				impl->writeJobSem_.Wait();
			}
		}
	};

	ManagerImpl* impl_ = nullptr;


	/// Resource load job.
	struct ResourceLoadJob : public Job::BasicJob
	{
		ResourceLoadJob(IFactory* factory, ResourceEntry* entry, Core::UUID type, const char* name, Core::File&& file)
		    : Job::BasicJob("ResourceLoadJob")
		    , factory_(factory)
		    , entry_(entry)
		    , type_(type)
		    , name_(name)
		    , file_(std::move(file))
		{
			impl_->AcquireResourceEntry(entry);
			Core::AtomicInc(&impl_->pendingResourceJobs_);
		}

		virtual ~ResourceLoadJob() { Core::AtomicDec(&impl_->pendingResourceJobs_); }

		void OnWork(i32 param) override
		{
			const bool isReload = entry_->loaded_ != 0;
			if(isReload)
			{
				Core::AtomicInc(&impl_->numReloadJobs_);
			}
			FactoryContext factoryContext;
			success_ = factory_->LoadResource(factoryContext, &entry_->resource_, type_, name_.c_str(), file_);
			if(success_ && !isReload)
			{
				Core::AtomicInc(&entry_->loaded_);
			}

			if(isReload)
			{
				Core::AtomicDec(&impl_->numReloadJobs_);
			}
		}

		void OnCompleted() override
		{
			impl_->ReleaseResourceEntry(entry_);
			delete this;
		}

		IFactory* factory_ = nullptr;
		ResourceEntry* entry_ = nullptr;
		Core::UUID type_;
		Core::String name_;
		Core::File file_;
		bool success_ = false;
	};

	/// Job to convert resource, and chain load if required.
	struct ResourceConvertJob : public Job::BasicJob
	{
		ResourceConvertJob(ResourceEntry* entry, Core::UUID type, const char* name, const char* convertedPath)
		    : Job::BasicJob("ResourceConvertJob")
		    , entry_(entry)
		    , type_(type)
		    , name_(name)
		    , convertedPath_(convertedPath)
		{
			Core::AtomicInc(&impl_->pendingResourceJobs_);
			i32 val = Core::AtomicInc(&entry->converting_);
			DBG_ASSERT(val == 1);
		}

		virtual ~ResourceConvertJob()
		{
			Core::AtomicDec(&entry_->converting_);
			Core::AtomicDec(&impl_->pendingResourceJobs_);
		}

		void OnWork(i32 param) override
		{
			success_ = Manager::ConvertResource(name_.c_str(), convertedPath_.c_str(), type_);
		}

		void OnCompleted() override
		{
			// If conversion was successful and there is a load job to chain, run it but block untll completion.
			if(success_ && loadJob_)
			{
				loadJob_->file_ = Core::File(convertedPath_.data(), Core::FileFlags::READ);
				DBG_ASSERT_MSG(loadJob_->file_, "Can't load converted file \"%s\"", convertedPath_.data());

				Job::Counter* counter = nullptr;
				loadJob_->RunSingle(0, &counter);
				Job::Manager::WaitForCounter(counter, 0);
			}
			delete this;
		}

		ResourceEntry* entry_ = nullptr;
		Core::UUID type_;
		Core::String name_;
		Core::String convertedPath_;
		bool success_ = false;
		ResourceLoadJob* loadJob_ = nullptr;
	};

	///
	struct ResourceTimestampJob : public Job::BasicJob
	{
		ResourceTimestampJob()
		    : Job::BasicJob("ResourceTimestampJob")
		{
			Core::AtomicInc(&impl_->pendingResourceJobs_);
		}

		virtual ~ResourceTimestampJob()
		{
			for(auto* entry : convertList_)
			{
				impl_->ReleaseResourceEntry(entry);
			}
			convertList_.clear();
			Core::AtomicDec(&impl_->pendingResourceJobs_);
		}

		void OnWork(i32 param) override
		{
			// Check time stamp of a single resource.
			{
				Job::ScopedReadLock lock(impl_->resourceRWLock_);
				if(idx_ < impl_->resourceList_.size())
				{
					auto* entry = impl_->resourceList_[idx_];
					if(entry->loaded_ && entry->ResourceOutOfDate(&impl_->pathResolver_))
					{
						if(std::find(convertList_.begin(), convertList_.end(), entry) == convertList_.end())
						{
							impl_->AcquireResourceEntry(entry);
							convertList_.push_back(entry);
							convertTimer_.Mark();
						}
					}

					idx_ = (idx_ + 1) % impl_->resourceList_.size();
				}
			}

			// Check if the appropriate amount of time has passed.
			if(convertTimer_.GetTime() > CONVERT_WAIT_TIME && convertList_.size() > 0)
			{
				for(auto* entry : convertList_)
				{
					if(entry->converting_ == 0)
					{
						DBG_LOG("Resource \"%s\" is out of date.\n", entry->sourceFile_.c_str());

						// Setup convert job.
						auto* convertJob = new ResourceConvertJob(
						    entry, entry->type_, entry->sourceFile_.c_str(), entry->convertedFile_.c_str());

						// Setup load job to chain.
						if(auto factory = impl_->GetFactory(entry->type_))
						{
							convertJob->loadJob_ = new ResourceLoadJob(
							    factory, entry, entry->type_, entry->sourceFile_.c_str(), Core::File());
							convertJob->RunSingle(0);
						}
					}

					impl_->ReleaseResourceEntry(entry);
				}
				convertList_.clear();
			}
		}

		void OnCompleted() override
		{
			// If active, reschedule job to check timestamps of the next file.
			if(impl_->isActive_)
				RunSingle(0);
			else
				delete this;
		}

		i32 idx_ = 0;
		Core::Vector<ResourceEntry*> convertList_;
		Core::Timer convertTimer_;
		const f64 CONVERT_WAIT_TIME = 0.f;
	};

	void Manager::Initialize()
	{
		DBG_ASSERT(impl_ == nullptr);
		DBG_ASSERT(Job::Manager::IsInitialized());
		DBG_ASSERT(Plugin::Manager::IsInitialized());
		impl_ = new ManagerImpl();

		// Create timestamp job to monitor resources.
		auto* timeStampJob = new ResourceTimestampJob();
		timeStampJob->RunSingle(0);

		impl_->reloadRWLock_.BeginRead();
	}

	void Manager::Finalize()
	{
		DBG_ASSERT(impl_);
		impl_->reloadRWLock_.EndRead();
		delete impl_;
		impl_ = nullptr;
	}

	bool Manager::IsInitialized() { return !!impl_; }

	void Manager::WaitOnReload()
	{
		impl_->reloadRWLock_.EndRead();
		Job::Manager::YieldCPU();
		impl_->reloadRWLock_.BeginRead();
	}

	Job::ScopedWriteLock Manager::TakeReloadLock() { return Job::ScopedWriteLock(impl_->reloadRWLock_); }

	bool Manager::RequestResource(void*& outResource, const char* name, const Core::UUID& type)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(outResource == nullptr);

		Core::Array<char, Core::MAX_PATH_LENGTH> path = {};
		Core::Array<char, Core::MAX_PATH_LENGTH> fileName = {};
		Core::Array<char, Core::MAX_PATH_LENGTH> ext = {};
		if(!Core::FileSplitPath(
		       name, path.data(), path.size(), fileName.data(), fileName.size(), ext.data(), ext.size()))
		{
			DBG_LOG("Unable to split file \"%s\"\n", name);
			return false;
		}

		// Build converted filename.
		Core::Array<char, Core::MAX_PATH_LENGTH> convertedFileName = {};
		Core::Array<char, Core::MAX_PATH_LENGTH> convertedPath = {};
		sprintf_s(convertedFileName.data(), convertedFileName.size(), "%s.%s.converted", fileName.data(), ext.data());
		sprintf_s(convertedPath.data(), convertedPath.size(), "converter_output");

		Core::FileCreateDir(convertedPath.data());

		Core::FileAppendPath(convertedPath.data(), convertedPath.size(), path.data());
		Core::FileAppendPath(convertedPath.data(), convertedPath.size(), convertedFileName.data());

		// Get factory for resource.
		if(auto factory = impl_->GetFactory(type))
		{
			// Acquire resource, create if required.
			ResourceEntry* entry = impl_->AcquireResourceEntry(name, convertedPath.data(), type);
			if(entry->resource_ == nullptr)
			{
				FactoryContext factoryContext;

				// First create resource.
				if(!factory->CreateResource(factoryContext, &entry->resource_, type))
					return false;

				// Setup job to create.
				{
					// Check if converted file exists.
					bool shouldConvert = !Core::FileExists(convertedPath.data());

					// If it does, check against metadata timestamp to see if we need to reimport.
					if(!shouldConvert)
					{
						// Setup metadata path.
						char srcPath[Core::MAX_PATH_LENGTH] = {0};
						char metaPath[Core::MAX_PATH_LENGTH] = {0};
						if(impl_->pathResolver_.ResolvePath(name, srcPath, sizeof(srcPath)))
						{
							strcpy_s(metaPath, sizeof(metaPath), srcPath);
							strcat_s(metaPath, sizeof(metaPath), ".metadata");

							Core::FileTimestamp srcTimestamp;
							Core::FileTimestamp metaTimestamp;
							if(Core::FileStats(srcPath, nullptr, &srcTimestamp, nullptr))
							{
								if(Core::FileStats(metaPath, nullptr, &metaTimestamp, nullptr))
								{
									if(metaTimestamp < srcTimestamp)
									{
										shouldConvert = true;
									}
								}
								else
								{
									shouldConvert = true;
								}
							}
						}
					}

					// If converted file doesn't exist, convert now.
					if(shouldConvert)
					{
						// Setup convert job.
						auto* convertJob = new ResourceConvertJob(entry, type, name, convertedPath.data());

						// Setup load job to chain.
						convertJob->loadJob_ = new ResourceLoadJob(factory, entry, type, fileName.data(), Core::File());

						convertJob->RunSingle(0);
					}
					else
					{

						auto* jobData = new ResourceLoadJob(factory, entry, type, fileName.data(),
						    Core::File(convertedPath.data(), Core::FileFlags::READ));

						jobData->RunSingle(0);
					}
				}
			}

			outResource = entry->resource_;
			return true;
		}
		return false;
	}

	bool Manager::ReleaseResource(void*& inResource, const Core::UUID& type)
	{
		DBG_ASSERT(IsInitialized());
		if(impl_->ReleaseResourceEntry(inResource, type))
		{
			impl_->ProcessReleasedResources();
		}
		inResource = nullptr;
		return true;
	}

	bool Manager::IsResourceReady(void* inResource, const Core::UUID& type)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(inResource != nullptr);
		return impl_->IsResourceReady(inResource, type);
	}

	void Manager::WaitForResource(void* inResource, const Core::UUID& type)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(inResource != nullptr);
		while(!IsResourceReady(inResource, type))
		{
			Job::Manager::YieldCPU();
		}
	}

	bool Manager::ConvertResource(const char* name, const char* convertedName, const Core::UUID& type)
	{
		DBG_ASSERT(IsInitialized());
		bool retVal = false;
		Core::AtomicInc(&impl_->numConversionJobs_);
		for(auto converterPlugin : impl_->converterPlugins_)
		{
			auto* converter = converterPlugin.CreateConverter();
			if(converter->SupportsFileType(nullptr, type))
			{
				ConverterContext converterContext(&impl_->pathResolver_);
				retVal = converterContext.Convert(converter, name, convertedName);
			}
			converterPlugin.DestroyConverter(converter);
			if(retVal)
				break;
		}
		Core::AtomicDec(&impl_->numConversionJobs_);
		return retVal;
	}

	bool Manager::RegisterFactory(const Core::UUID& type, IFactory* factory)
	{
		DBG_ASSERT(IsInitialized());

		if(impl_->factories_.find(type) != impl_->factories_.end())
			return false;

		impl_->factories_.insert(type, factory);
		return true;
	}

	bool Manager::UnregisterFactory(IFactory* factory)
	{
		DBG_ASSERT(IsInitialized());
		bool success = false;
		for(auto it = impl_->factories_.begin(); it != impl_->factories_.end();)
		{
			if(it->second == factory)
			{
				it = impl_->factories_.erase(it);
				success = true;
			}
			else
				++it;
		}

		return success;
	}

	Result Manager::ReadFileData(Core::File& file, i64 offset, i64 size, void* dest, AsyncResult* result)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(Core::ContainsAllFlags(file.GetFlags(), Core::FileFlags::READ));
		DBG_ASSERT(offset >= 0);
		DBG_ASSERT(size > 0);
		DBG_ASSERT(dest != nullptr);

		Result outResult = Result::PENDING;
		if(result)
		{
			auto oldResult = (Result)Core::AtomicExchg((volatile i32*)&result->result_, (i32)outResult);
			DBG_ASSERT(oldResult == Result::INITIAL);
		}

		FileIOJob job;
		job.file_ = &file;
		job.offset_ = offset;
		job.size_ = size;
		job.addr_ = dest;
		job.result_ = result;

		if(result)
		{
			Core::AtomicAddAcq(&result->workRemaining_, size);
			impl_->readJobs_.Enqueue(job);
			impl_->readJobSem_.Signal(1);
		}
		else
		{
			outResult = job.DoRead();
		}
		return outResult;
	}

	Result Manager::WriteFileData(Core::File& file, i64 size, void* src, AsyncResult* result)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(Core::ContainsAllFlags(file.GetFlags(), Core::FileFlags::WRITE));
		DBG_ASSERT(size > 0);
		DBG_ASSERT(src != nullptr);
		DBG_ASSERT(!result || (result->result_ == Result::INITIAL && result->workRemaining_ == 0));

		Result outResult = Result::PENDING;
		if(result)
		{
			auto oldResult = (Result)Core::AtomicExchg((volatile i32*)&result->result_, (i32)outResult);
			DBG_ASSERT(oldResult == Result::INITIAL);
		}

		FileIOJob job;
		job.file_ = &file;
		job.offset_ = 0;
		job.size_ = size;
		job.addr_ = src;
		job.result_ = result;

		if(result)
		{
			Core::AtomicAddAcq(&result->workRemaining_, size);
			impl_->writeJobs_.Enqueue(job);
			impl_->writeJobSem_.Signal(1);
		}
		else
		{
			outResult = job.DoWrite();
		}
		return outResult;
	}


} // namespace Resource
