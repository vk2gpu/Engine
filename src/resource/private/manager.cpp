#pragma once

#include "resource/manager.h"
#include "resource/converter.h"
#include "resource/factory.h"
#include "resource/private/converter_context.h"
#include "resource/private/database.h"
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

#include "Remotery.h"

#include <algorithm>
#include <utility>

namespace Resource
{

	Core::Vector<Core::String> LoadDependencies(Core::IFilePathResolver* pathResolver, const char* sourceFile)
	{
		Core::Vector<Core::String> deps;
		Core::Array<char, Core::MAX_PATH_LENGTH> metaDataFilename;
		if(pathResolver->ResolvePath(sourceFile, metaDataFilename.data(), metaDataFilename.size()))
		{
			strcat_s(metaDataFilename.data(), metaDataFilename.size(), ".metadata");

			auto metaDataFile = Core::File(metaDataFilename.data(), Core::FileFlags::DEFAULT_READ);
			if(metaDataFile)
			{
				auto metaDataSer = Serialization::Serializer(metaDataFile, Serialization::Flags::TEXT);
				if(auto object = metaDataSer.Object("$internal"))
				{
					metaDataSer.Serialize("dependencies", deps);
				}
			}
		}
		return deps;
	}

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

		Core::Vector<Core::String> dependencies_;

		/// @return If resource is out of date and needs reimporting.
		bool ResourceOutOfDate(Core::IFilePathResolver* pathResolver) const
		{
			Core::FileTimestamp convertedTimestamp;
			bool sourceExists = false;
			bool convertedExists = Core::FileStats(convertedFile_.c_str(), nullptr, &convertedTimestamp, nullptr);
			if(convertedExists)
			{
				for(const auto& dep : dependencies_)
				{
					Core::FileTimestamp depTimestamp;
					if(pathResolver)
					{
						Core::Array<char, Core::MAX_PATH_LENGTH> resolvedSourcePath = {};
						pathResolver->ResolvePath(dep.c_str(), resolvedSourcePath.data(), resolvedSourcePath.size());
						sourceExists = Core::FileStats(resolvedSourcePath.data(), nullptr, &depTimestamp, nullptr);
					}
					else
					{
						sourceExists &= Core::FileStats(dep.c_str(), nullptr, &depTimestamp, nullptr);
					}

					if(sourceExists)
						if(convertedTimestamp < depTimestamp)
							return true;
				}
			}
			return !convertedExists;
		}
	};

	using ResourceList = Core::Vector<ResourceEntry*>;
}

namespace Core
{
	u64 Hash(u64 input, const Core::Pair<Core::UUID, Core::UUID>& pair)
	{
		return HashFNV1a(input, &pair, sizeof(pair));
	}

	u64 Hash(u64 input, Resource::ResourceEntry* resourceEntry)
	{
		return HashFNV1a(input, &resourceEntry, sizeof(resourceEntry));
	}
} // namespace Core

namespace Resource
{
	/// Resource load job.
	struct ResourceLoadJob : public Job::BasicJob
	{
		ResourceLoadJob(IFactory* factory, ResourceEntry* entry, Core::UUID type, const char* name, Core::File&& file);
		virtual ~ResourceLoadJob();
		void OnWork(i32 param) override;
		void OnCompleted() override;

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
		ResourceConvertJob(ResourceEntry* entry, Core::UUID type, const char* name, const char* convertedPath);
		virtual ~ResourceConvertJob();
		void OnWork(i32 param) override;
		void OnCompleted() override;

		ResourceEntry* entry_ = nullptr;
		Core::UUID type_;
		Core::String name_;
		Core::String convertedPath_;
		bool success_ = false;
		ResourceLoadJob* loadJob_ = nullptr;
	};

	struct ManagerImpl
	{
		static const i32 MAX_READ_JOBS = 128;
		static const i32 MAX_WRITE_JOBS = 128;

		/// Is resource manager active? true from initialize, false at finalize.
		bool isActive_ = false;

		/// Plugins.
		Core::Vector<ConverterPlugin> converterPlugins_;

		/// Resource database.
		Database* database_ = nullptr;

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

		/// Signalled to kick off timestamp checking.
		Core::Semaphore timestampJobSem_;
		/// Timestamp thread.
		Core::Thread timestampThread_;

		/// Path resolver.
		PathResolver pathResolver_;

		/// Root path in project structure (where the 'res' folder is)
		Core::String rootPath_;

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
		bool ReleaseResourceEntry(void* resource)
		{
			Job::ScopedWriteLock lock(resourceRWLock_);
			auto it = std::find_if(resourceList_.begin(), resourceList_.end(),
			    [resource](ResourceEntry* listEntry) { return listEntry->resource_ == resource; });
			DBG_ASSERT(it != resourceList_.end());
			if(Core::AtomicDec(&(*it)->refCount_) == 0)
			{
				UnsafeDoReleaseResourceEntry(*it);
			}
			return true;
		}

		/// @return if resource is ready.
		bool IsResourceReady(void* resource)
		{
			Job::ScopedWriteLock lock(resourceRWLock_);
			auto it = std::find_if(resourceList_.begin(), resourceList_.end(),
			    [resource](ResourceEntry* listEntry) { return listEntry->resource_ == resource; });
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
				DBG_LOG("Factory does not exist for type %s\n", uuidStr);
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
		    : isActive_(true)
		    , readJobs_(MAX_READ_JOBS)
		    , readJobSem_(0, MAX_READ_JOBS, "Resmgr Read Sem")
		    , readThread_(ReadIOThread, this, 65536, "Resmgr Read Thread")
		    , writeJobs_(MAX_WRITE_JOBS)
		    , writeJobSem_(0, MAX_WRITE_JOBS, "Resmgr Write Sem")
		    , writeThread_(WriteIOThread, this, 65536, "Resmgr Write Thread")
		    , timestampJobSem_(0, 1, "Resmgr Timestamp Sem")
		    , timestampThread_(TimestampThread, this, 65536, "Resmgr Timestamp Thread")
		{
			// Get converter plugins.
			i32 found = Plugin::Manager::GetPlugins<ConverterPlugin>(nullptr, 0);
			converterPlugins_.resize(found);
			Plugin::Manager::GetPlugins<ConverterPlugin>(converterPlugins_.data(), converterPlugins_.size());

			// From current working directory find the "res" directory to add to the path resolver.
			Core::String rootPath = "";
			Core::String currRelativePath = "res";
			while(Core::FileExists(currRelativePath.c_str()) == false)
			{
				rootPath.Printf("../%s", rootPath.c_str());
				currRelativePath.Printf("../%s", currRelativePath.c_str());

				DBG_ASSERT_MSG(rootPath.size() < Core::MAX_PATH_LENGTH, "Unable to find \'res\' directory!");
			}

			pathResolver_.AddPath("./");
			pathResolver_.AddPath(currRelativePath.c_str());

			// Converter output folder should be along side "res".
			std::swap(rootPath_, rootPath);

			// Scan for resources.
			database_ = new Database(currRelativePath.c_str(), pathResolver_);
			database_->ScanResources();

			// Start timestamp checking job.
			timestampJobSem_.Signal(1);
		}

		~ManagerImpl()
		{
			// No longer active. Any pending jobs will complete.
			isActive_ = false;
			Core::Barrier();

			// Wait for pending resource jobs to complete.
			while(pendingResourceJobs_ > 0)
				Job::Manager::YieldCPU();


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

			timestampJobSem_.Signal(1);
			timestampThread_.Join();

			delete database_;
			database_ = nullptr;
		}

		static int ReadIOThread(void* userData)
		{
			auto* impl = reinterpret_cast<ManagerImpl*>(userData);

			FileIOJob ioJob;
			for(;;)
			{
				impl->readJobSem_.Wait();
				rmt_ScopedCPUSample(ResourceReadIO, RMTSF_None);

				if(impl->readJobs_.Dequeue(ioJob))
				{
					if(ioJob.file_ != nullptr)
						ioJob.DoRead();
					else
						return 0;
				}
			}
		}

		static int WriteIOThread(void* userData)
		{
			auto* impl = reinterpret_cast<ManagerImpl*>(userData);

			FileIOJob ioJob;
			for(;;)
			{
				impl->writeJobSem_.Wait();
				rmt_ScopedCPUSample(ResourceWriteIO, RMTSF_None);

				if(impl->writeJobs_.Dequeue(ioJob))
				{
					if(ioJob.file_ != nullptr)
						ioJob.DoWrite();
					else
						return 0;
				}
			}
		}

		static int TimestampThread(void* userData)
		{
			auto* impl = reinterpret_cast<ManagerImpl*>(userData);

			i32 idx = 0;
			Core::Vector<ResourceEntry*> convertList;
			Core::Timer convertTimer;
			const f64 CONVERT_WAIT_TIME = 0.01f;

			// Wait until signalled to start.
			impl->timestampJobSem_.Wait();

			while(impl->isActive_)
			{
				{
					rmt_ScopedCPUSample(ResourceTimestamp, RMTSF_None);

					{
						Job::ScopedReadLock lock(impl->resourceRWLock_);
						if(idx < impl->resourceList_.size())
						{
							auto* entry = impl->resourceList_[idx];
							if(entry->loaded_ && entry->ResourceOutOfDate(&impl->pathResolver_))
							{
								if(std::find(convertList.begin(), convertList.end(), entry) == convertList.end())
								{
									impl->AcquireResourceEntry(entry);
									convertList.push_back(entry);
									convertTimer.Mark();
								}
							}

							idx = (idx + 1) % impl->resourceList_.size();
						}
					}

					// Check if the appropriate amount of time has passed.
					if(convertTimer.GetTime() > CONVERT_WAIT_TIME && convertList.size() > 0)
					{
						for(auto* entry : convertList)
						{
							if(entry->converting_ == 0)
							{
								DBG_LOG("Resource \"%s\" is out of date.\n", entry->sourceFile_.c_str());

								if(auto factory = impl->GetFactory(entry->type_))
								{
									// Setup convert job.
									auto* convertJob = new ResourceConvertJob(
									    entry, entry->type_, entry->sourceFile_.c_str(), entry->convertedFile_.c_str());

									// Setup load job to chain.
									convertJob->loadJob_ = new ResourceLoadJob(
									    factory, entry, entry->type_, entry->sourceFile_.c_str(), Core::File());

									convertJob->RunSingle(0);
								}
							}

							impl->ReleaseResourceEntry(entry);
						}
						convertList.clear();
					}
				}

				// Wait on a semaphore for around 100ms once all files are checked.
				if(idx == 0)
					impl->timestampJobSem_.Wait(100);
			}

			for(auto* entry : convertList)
			{
				impl->ReleaseResourceEntry(entry);
			}
			convertList.clear();
			Core::AtomicDec(&impl->pendingResourceJobs_);
			return 0;
		}
	};

	ManagerImpl* impl_ = nullptr;

	ResourceLoadJob::ResourceLoadJob(
	    IFactory* factory, ResourceEntry* entry, Core::UUID type, const char* name, Core::File&& file)
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

	ResourceLoadJob::~ResourceLoadJob() { Core::AtomicDec(&impl_->pendingResourceJobs_); }

	void ResourceLoadJob::OnWork(i32 param)
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
			entry_->dependencies_ = LoadDependencies(&impl_->pathResolver_, entry_->sourceFile_.c_str());
			Core::AtomicInc(&entry_->loaded_);
		}

		if(!success_)
		{
			Core::MessageBox("Resource Load Error",
			    Core::String().Printf("Unable to load resource \"%s\"", entry_->sourceFile_.c_str()).c_str(),
			    Core::MessageBoxType::OK, Core::MessageBoxIcon::ERROR);
		}

		if(isReload)
		{
			Core::AtomicDec(&impl_->numReloadJobs_);
		}
	}

	void ResourceLoadJob::OnCompleted()
	{
		impl_->ReleaseResourceEntry(entry_);
		delete this;
	}

	ResourceConvertJob::ResourceConvertJob(
	    ResourceEntry* entry, Core::UUID type, const char* name, const char* convertedPath)
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

	ResourceConvertJob::~ResourceConvertJob()
	{
		Core::AtomicDec(&entry_->converting_);
		Core::AtomicDec(&impl_->pendingResourceJobs_);
	}

	void ResourceConvertJob::OnWork(i32 param)
	{
		success_ = false;
		Core::AtomicInc(&impl_->numConversionJobs_);
		for(auto converterPlugin : impl_->converterPlugins_)
		{
			auto* converter = converterPlugin.CreateConverter();
			if(converter->SupportsFileType(nullptr, type_))
			{
				ConverterContext converterContext(&impl_->pathResolver_);
				success_ = converterContext.Convert(converter, name_.c_str(), convertedPath_.c_str());

				if(!success_ && Core::IsDebuggerAttached())
				{
					DBG_ASSERT(false);
				}
			}
			converterPlugin.DestroyConverter(converter);
			if(success_)
				break;
		}
		Core::AtomicDec(&impl_->numConversionJobs_);
	}

	void ResourceConvertJob::OnCompleted()
	{
		// If conversion was a failure, we need to try again.
		if(!success_)
		{
			RunSingle(0);
			return;
		}

		// If conversion was successful and there is a load job to chain, run it but block untll completion.
		if(success_ && loadJob_)
		{
			loadJob_->file_ = Core::File(convertedPath_.data(), Core::FileFlags::DEFAULT_READ);
			DBG_ASSERT_MSG(loadJob_->file_, "Can't load converted file \"%s\"", convertedPath_.data());

			Job::Counter* counter = nullptr;
			loadJob_->RunSingle(0, &counter);
			Job::Manager::WaitForCounter(counter, 0);
		}
		delete this;
	}

	void Manager::Initialize()
	{
		DBG_ASSERT(impl_ == nullptr);
		DBG_ASSERT(Job::Manager::IsInitialized());
		DBG_ASSERT(Plugin::Manager::IsInitialized());
		impl_ = new ManagerImpl();

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
		DBG_ASSERT(name != nullptr);
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
		sprintf_s(convertedPath.data(), convertedPath.size(), "%s.converter_output", impl_->rootPath_.data());

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
						    Core::File(convertedPath.data(), Core::FileFlags::DEFAULT_READ));

						jobData->RunSingle(0);
					}
				}
			}

			outResource = entry->resource_;
			return true;
		}
		return false;
	}

	bool Manager::RequestResource(void*& outResource, const Core::UUID& uuid, const Core::UUID& type)
	{
		DBG_ASSERT(IsInitialized());
		Core::String name = impl_->database_->GetPathRescan(uuid);
		if(name.size() > 0)
			return RequestResource(outResource, name.c_str(), type);
		return false;
	}

	bool Manager::ReleaseResource(void*& inResource)
	{
		DBG_ASSERT(IsInitialized());
		WaitForResource(inResource);
		if(impl_->ReleaseResourceEntry(inResource))
		{
			impl_->ProcessReleasedResources();
		}
		inResource = nullptr;
		return true;
	}

	bool Manager::IsResourceReady(void* inResource)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(inResource != nullptr);
		return impl_->IsResourceReady(inResource);
	}

	void Manager::WaitForResource(void* inResource)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(inResource != nullptr);
		f64 maxWaitTime = Core::IsDebuggerAttached() ? 120.0 : 10.0;
		f64 startTime = Core::Timer::GetAbsoluteTime();
		while(!IsResourceReady(inResource))
		{
			Job::Manager::YieldCPU();
			if((Core::Timer::GetAbsoluteTime() - startTime) > maxWaitTime)
			{
				auto retVal = Core::MessageBox("Resource load timeout.",
				    "Timed out waiting for resource load. Continue waiting?", Core::MessageBoxType::OK_CANCEL);
				if(retVal == Core::MessageBoxReturn::CANCEL)
					DBG_ASSERT(false);
				startTime = Core::Timer::GetAbsoluteTime();
				maxWaitTime *= 2.0;
			}
		}
	}

	bool Manager::RegisterFactory(const Core::UUID& type, IFactory* factory)
	{
		DBG_ASSERT(IsInitialized());

		if(impl_->factories_.find(type) != impl_->factories_.end())
			return false;

		impl_->factories_.insert(type, factory);

		// Load settings.
		if(auto file = Core::File("settings.json", Core::FileFlags::DEFAULT_READ, &impl_->pathResolver_))
		{
			if(auto ser = Serialization::Serializer(file, Serialization::Flags::TEXT))
			{
				if(auto object = ser.Object("resources"))
				{
					factory->SerializeSettings(ser);
				}
			}
		}

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
		DBG_ASSERT(Core::ContainsAllFlags(file.GetFlags(), Core::FileFlags::DEFAULT_READ));
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
