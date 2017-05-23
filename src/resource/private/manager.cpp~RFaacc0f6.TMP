#pragma once

#include "resource/manager.h"
#include "resource/converter.h"
#include "resource/factory.h"

#include "core/array.h"
#include "core/concurrency.h"
#include "core/file.h"
#include "core/library.h"
#include "core/map.h"
#include "core/misc.h"
#include "core/mpmc_bounded_queue.h"
#include "core/uuid.h"

#include "job/manager.h"
#include "plugin/manager.h"
#include "serialization/serializer.h"

#include <algorithm>
#include <utility>

namespace Resource
{
	struct ResourceEntry
	{
		void* resource_ = nullptr;
		Core::UUID name_;
		Core::UUID type_;
		volatile i32 loaded_ = 0;
		volatile i32 refCount_ = 0;
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
	/// Converter context to use during resource conversion.
	class ConverterContext : public Resource::IConverterContext
	{
	public:
		ConverterContext() {}

		virtual ~ConverterContext() {}

		void AddDependency(const char* fileName) override
		{ //
			Core::Log("AddDependency: %s\n", fileName);
		}

		void AddOutput(const char* fileName) override
		{ //
			Core::Log("AddOutput: %s\n", fileName);
		}

		void AddError(const char* errorFile, int errorLine, const char* errorMsg) override
		{
			if(errorFile)
			{
				Core::Log("%s(%d): %s\n", errorFile, errorLine, errorMsg);
			}
			else
			{
				Core::Log("%s\n", errorMsg);
			}
		}

		Core::IFilePathResolver* GetPathResolver() override
		{
			class PathResolver : public Core::IFilePathResolver
			{
			public:
				PathResolver() {}

				virtual ~PathResolver() {}

				bool ResolvePath(const char* inPath, char* outPath, i32 maxOutPath)
				{
					const char* paths[] = {"", "../../../../res"};

					char intPath[Core::MAX_PATH_LENGTH];
					for(i32 i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i)
					{
						memset(intPath, 0, sizeof(intPath));
						Core::FileAppendPath(intPath, sizeof(intPath), paths[i]);
						Core::FileAppendPath(intPath, sizeof(intPath), inPath);
						if(Core::FileExists(intPath))
						{
							strcpy_s(outPath, maxOutPath, intPath);
							return true;
						}
					}

					return false;
				}
			};

			static PathResolver pathResolver;
			return &pathResolver;
		}

		bool Convert(IConverter* converter, const char* sourceFile, const char* destPath)
		{
			// Setup metadata path.
			if(GetPathResolver()->ResolvePath(sourceFile, metaDataFileName_, sizeof(metaDataFileName_)))
			{
				strcat_s(metaDataFileName_, sizeof(metaDataFileName_), ".metadata");
			}

			// Do conversion.
			return converter->Convert(*this, sourceFile, destPath);
		}

		void SetMetaData(MetaDataCb callback, void* metaData) override
		{
			metaDataFile_ = Core::File(metaDataFileName_, Core::FileFlags::CREATE | Core::FileFlags::WRITE);
			metaDataSer_ = Serialization::Serializer(metaDataFile_, Serialization::Flags::TEXT);

			if(callback)
			{
				callback(metaDataSer_, metaData);
			}

			AddDependency(metaDataFileName_);

			metaDataSer_ = Serialization::Serializer();
			metaDataFile_ = Core::File();
		}

		void GetMetaData(MetaDataCb callback, void* metaData) override
		{
			// If we don't have a metadata file for red, attempt to open for read.
			if(!Core::ContainsAnyFlags(metaDataFile_.GetFlags(), Core::FileFlags::READ) &&
			    Core::FileExists(metaDataFileName_))
			{
				metaDataFile_ = Core::File(metaDataFileName_, Core::FileFlags::READ);
				metaDataSer_ = Serialization::Serializer(metaDataFile_, Serialization::Flags::TEXT);

				if(callback)
				{
					callback(metaDataSer_, metaData);
				}
			}
		}

	private:
		char metaDataFileName_[Core::MAX_PATH_LENGTH] = {0};
		Core::File metaDataFile_;
		Serialization::Serializer metaDataSer_;
	};

	/// Factory context to use during creation of resources.
	class FactoryContext : public Resource::IFactoryContext
	{
	public:
		FactoryContext() {}
		virtual ~FactoryContext() {}
	};

	/// File IO Job. Can be read or write.
	struct FileIOJob
	{
		static const i64 READ_CHUNK_SIZE = 8 * 1024 * 1024;  // TODO: Possibly have this configurable?
		static const i64 WRITE_CHUNK_SIZE = 8 * 1024 * 1024; // TODO: Possibly have this configurable?

		Core::File* file_ = nullptr;
		i64 offset_ = 0;
		i64 size_ = 0;
		void* addr_ = nullptr;
		AsyncResult* result_ = nullptr;

		Result DoRead()
		{
			DBG_ASSERT(file_);
			DBG_ASSERT((offset_ + size_) <= file_->Size());

			if(result_)
			{
				auto oldResult = (Result)Core::AtomicExchg((volatile i32*)&result_->result_, (i32)Result::RUNNING);
				DBG_ASSERT(oldResult == Result::PENDING);
			}

			file_->Seek(offset_);

			// Read file in chunks.
			char* dest = (char*)addr_;
			i64 sizeRemaining = size_;
			while(sizeRemaining > 0)
			{
				i64 readSize = Core::Min(READ_CHUNK_SIZE, sizeRemaining);
				i64 bytesRead = file_->Read(dest, readSize);
				sizeRemaining -= readSize;
				dest += readSize;
				if(result_)
				{
					Core::AtomicAddRel(&result_->workRemaining_, -bytesRead);
				}

				// Check that we successfully read.
				if(bytesRead < readSize)
					break;
			}

			Result result = Result::FAILURE;
			if(sizeRemaining == 0)
				result = Result::SUCCESS;
			if(result_)
			{
				Core::AtomicExchg((volatile i32*)&result_->result_, (i32)result);
			}
			return result;
		}

		Result DoWrite()
		{
			DBG_ASSERT(file_);
			DBG_ASSERT(offset_ == 0);

			if(result_)
			{
				auto oldResult = (Result)Core::AtomicExchg((volatile i32*)&result_->result_, (i32)Result::RUNNING);
				DBG_ASSERT(oldResult == Result::PENDING);
			}

			// Write file in chunks.
			char* src = (char*)addr_;
			i64 sizeRemaining = size_;
			while(sizeRemaining > 0)
			{
				i64 writeSize = Core::Min(WRITE_CHUNK_SIZE, sizeRemaining);
				i64 bytesWrote = file_->Write(src, writeSize);
				sizeRemaining -= writeSize;
				src += writeSize;
				if(result_)
				{
					Core::AtomicAddRel(&result_->workRemaining_, -bytesWrote);
				}

				// Check that we successfully read.
				if(bytesWrote < writeSize)
					break;
			}

			Result result = Result::FAILURE;
			if(sizeRemaining == 0)
				result = Result::SUCCESS;
			if(result_)
			{
				Core::AtomicExchg((volatile i32*)&result_->result_, (i32)result);
			}
			return result;
		}
	};

	struct ManagerImpl
	{
		static const i32 MAX_READ_JOBS = 128;
		static const i32 MAX_WRITE_JOBS = 128;

		/// Plugins.
		Core::Vector<ConverterPlugin> converterPlugins_;

		/// Read job queue.
		Core::MPMCBoundedQueue<FileIOJob> readJobs_;
		/// Signalled when read job is waiting.
		Core::Event readJobEvent_;
		/// Thread to use for blocking reads.
		Core::Thread readThread_;

		/// Write job queue.
		Core::MPMCBoundedQueue<FileIOJob> writeJobs_;
		/// Signalled when write job is waiting.
		Core::Event writeJobEvent_;
		/// Thread to use for blocking reads.
		Core::Thread writeThread_;

		/// Resources.
		volatile i32 pendingResourceJobs_ = 0;
		ResourceList resourceList_;
		ResourceList releasedResourceList_;
		Core::Mutex resourceMutex_;

		void AcquireResourceEntry(ResourceEntry* entry)
		{ //
			Core::AtomicInc(&entry->refCount_);
		}

		bool ReleaseResourceEntry(ResourceEntry* entry)
		{
			Core::ScopedMutex lock(resourceMutex_);
			if(Core::AtomicDec(&entry->refCount_) == 0)
			{
				releasedResourceList_.push_back(entry);
				auto it = std::find_if(resourceList_.begin(), resourceList_.end(),
				    [entry](ResourceEntry* listEntry) { return entry == listEntry; });
				DBG_ASSERT(it != resourceList_.end());
				resourceList_.erase(it);
				return true;
			}
			return false;
		}

		ResourceEntry* AcquireResourceEntry(const Core::UUID& name, const Core::UUID& type)
		{
			Core::ScopedMutex lock(resourceMutex_);
			ResourceEntry* entry = nullptr;
			auto it =
			    std::find_if(resourceList_.begin(), resourceList_.end(), [&name, &type](ResourceEntry* listEntry) {
				    return listEntry->name_ == name && listEntry->type_ == type;
			    });
			if(it == resourceList_.end())
			{
				entry = new ResourceEntry();
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
			Core::ScopedMutex lock(resourceMutex_);
			auto it =
			    std::find_if(resourceList_.begin(), resourceList_.end(), [resource, &type](ResourceEntry* listEntry) {
				    return listEntry->resource_ == resource && listEntry->type_ == type;
			    });
			DBG_ASSERT(it != resourceList_.end());
			return ReleaseResourceEntry(*it);
		}

		/// @return if resource is ready.
		bool IsResourceReady(void* resource, const Core::UUID& type)
		{
			Core::ScopedMutex lock(resourceMutex_);
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
				Core::ScopedMutex lock(resourceMutex_);
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
		    , readJobEvent_(false, false, "Resource Manager Read Event")
		    , readThread_(ReadIOThread, this, 65536, "Resource Manager Read Thread")
		    , writeJobs_(MAX_WRITE_JOBS)
		    , writeJobEvent_(false, false, "Resource Manager Write Event")
		    , writeThread_(WriteIOThread, this, 65536, "Resource Manager Write Thread")
		{
			// Get converter plugins.
			i32 found = Plugin::Manager::GetPlugins<ConverterPlugin>(nullptr, 0);
			converterPlugins_.resize(found);
			Plugin::Manager::GetPlugins<ConverterPlugin>(converterPlugins_.data(), converterPlugins_.size());
		}

		~ManagerImpl()
		{
			// Wait for pending resource jobs to complete.
			while(pendingResourceJobs_ > 0)
			{
				Job::Manager::YieldCPU();
			}

			ProcessReleasedResources();

			// TODO: Mark jobs as cancelled.
			while(readJobs_.Enqueue(FileIOJob()) == false)
				Job::Manager::YieldCPU();
			readJobEvent_.Signal();
			readThread_.Join();

			while(writeJobs_.Enqueue(FileIOJob()) == false)
				Job::Manager::YieldCPU();
			writeJobEvent_.Signal();
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
				impl->readJobEvent_.Wait();
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
				impl->writeJobEvent_.Wait();
			}
		}
	};

	/// Resource load job.
	struct ResourceLoadJob
	{
		ResourceLoadJob(ManagerImpl* impl, IFactory* factory, ResourceEntry* entry, Core::UUID type, Core::File&& file)
		    : impl_(impl)
		    , factory_(factory)
		    , entry_(entry)
		    , type_(type)
		    , file_(std::move(file))
		{
		}

		void RunJob()
		{
			FactoryContext factoryContext;
			success_ = factory_->LoadResource(factoryContext, &entry_->resource_, type_, file_);
			if(success_)
				Core::AtomicInc(&entry_->loaded_);
			impl_->ReleaseResourceEntry(entry_);
			Core::AtomicDec(&impl_->pendingResourceJobs_);
		}

		ManagerImpl* impl_ = nullptr;
		IFactory* factory_ = nullptr;
		ResourceEntry* entry_ = nullptr;
		Core::UUID type_;
		Core::File file_;
		bool success_ = false;
	};

	ManagerImpl* impl_ = nullptr;

	/// Resource convert job.
	struct ResourceConvertJob
	{
		ResourceConvertJob(ResourceEntry* entry, Core::UUID type, const char* name, const char* convertedPath)
		    : entry_(entry)
		    , type_(type)
		{
			strcpy_s(name_.data(), name_.size(), name);
			strcpy_s(convertedPath_.data(), convertedPath_.size(), convertedPath);
		}

		void RunJob()
		{
			success_ = Manager::ConvertResource(name_.data(), convertedPath_.data(), type_);
			Core::AtomicDec(&impl_->pendingResourceJobs_);
		}

		ResourceEntry* entry_ = nullptr;
		Core::UUID type_;
		Core::Array<char, Core::MAX_PATH_LENGTH> name_;
		Core::Array<char, Core::MAX_PATH_LENGTH> convertedPath_;
		bool success_ = false;
	};


	void Manager::Initialize()
	{
		DBG_ASSERT(impl_ == nullptr);
		DBG_ASSERT(Job::Manager::IsInitialized());
		DBG_ASSERT(Plugin::Manager::IsInitialized());
		impl_ = new ManagerImpl();
	}

	void Manager::Finalize()
	{
		DBG_ASSERT(impl_);
		delete impl_;
		impl_ = nullptr;
	}

	bool Manager::IsInitialized() { return !!impl_; }

	bool Manager::RequestResource(void*& outResource, const char* name, const Core::UUID& type)
	{
		DBG_ASSERT(IsInitialized());
		DBG_ASSERT(outResource == nullptr);

		Core::Array<char, Core::MAX_PATH_LENGTH> path;
		Core::Array<char, Core::MAX_PATH_LENGTH> fileName;
		Core::Array<char, Core::MAX_PATH_LENGTH> ext;
		if(!Core::FileSplitPath(
		       name, path.data(), path.size(), fileName.data(), fileName.size(), ext.data(), ext.size()))
		{
			DBG_LOG("Unable to split file \"%s\"\n", name);
			return false;
		}

		// Build converted filename.
		Core::Array<char, Core::MAX_PATH_LENGTH> convertedFileName;
		Core::Array<char, Core::MAX_PATH_LENGTH> convertedPath;
		sprintf_s(convertedFileName.data(), convertedFileName.size(), "%s.%s.converted", fileName.data(), ext.data());
		sprintf_s(convertedPath.data(), convertedPath.size(), "converter_output");

		Core::FileCreateDir(convertedPath.data());

		Core::FileAppendPath(convertedPath.data(), convertedPath.size(), path.data());
		Core::FileAppendPath(convertedPath.data(), convertedPath.size(), convertedFileName.data());

		// Get factory for resource.
		if(auto factory = impl_->GetFactory(type))
		{
			// Acquire resource, create if required.
			ResourceEntry* entry = impl_->AcquireResourceEntry(name, type);
			if(entry->resource_ == nullptr)
			{
				FactoryContext factoryContext;

				// First create resource.
				if(!factory->CreateResource(factoryContext, &entry->resource_, type))
					return false;

				// Setup job to create.
				{
					// If converted file doesn't exist, convert now.
					// TODO: This should be done async.
					if(!Core::FileExists(convertedPath.data()))
					{
						auto* jobData = new ResourceConvertJob(entry, type, name, convertedPath.data());

						Job::JobDesc jobDesc;
						jobDesc.func_ = [](i32 inParam, void* inData) {
							auto* data = reinterpret_cast<ResourceConvertJob*>(inData);
							data->RunJob();
						};
						jobDesc.param_ = 0;
						jobDesc.data_ = jobData;
						jobDesc.name_ = "ResourceConvertJob";

						Core::AtomicInc(&impl_->pendingResourceJobs_);

						// TODO: Run async.
						Job::Counter* counter = nullptr;
						Job::Manager::RunJobs(&jobDesc, 1, &counter);
						Job::Manager::WaitForCounter(counter, 0);
						DBG_ASSERT(jobData->success_);
						delete jobData;
					}

					// Do load.
					{

						// Acquire entry for job.
						impl_->AcquireResourceEntry(entry);

						auto* jobData = new ResourceLoadJob(
						    impl_, factory, entry, type, Core::File(convertedPath.data(), Core::FileFlags::READ));
						Job::JobDesc jobDesc;
						jobDesc.func_ = [](i32 inParam, void* inData) {
							auto* data = reinterpret_cast<ResourceLoadJob*>(inData);
							data->RunJob();
							delete data;
						};
						jobDesc.param_ = 0;
						jobDesc.data_ = jobData;
						jobDesc.name_ = "ResourceLoadJob";

						Core::AtomicInc(&impl_->pendingResourceJobs_);
						Job::Manager::RunJobs(&jobDesc, 1, nullptr);
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
		for(auto converterPlugin : impl_->converterPlugins_)
		{
			auto* converter = converterPlugin.CreateConverter();
			if(converter->SupportsFileType(nullptr, type))
			{
				ConverterContext converterContext;
				retVal = converterContext.Convert(converter, name, convertedName);
			}
			converterPlugin.DestroyConverter(converter);
			if(retVal)
				break;
		}
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
			impl_->readJobEvent_.Signal();
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
			impl_->writeJobEvent_.Signal();
		}
		else
		{
			outResult = job.DoWrite();
		}
		return outResult;
	}


} // namespace Resource
