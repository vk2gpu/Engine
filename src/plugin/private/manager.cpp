#pragma once

#include "plugin/manager.h"

#include "core/concurrency.h"
#include "core/file.h"
#include "core/library.h"
#include "core/uuid.h"
#include "core/vector.h"

#include <algorithm>
#include <utility>

namespace Plugin
{
	struct PluginDesc
	{
		PluginDesc() = default;
		PluginDesc(const char* libName)
		{
			handle_ = Core::LibraryOpen(libName);
			if(handle_)
			{
				getPluginName_ = (GetPluginNameFn)Core::LibrarySymbol(handle_, "GetPluginName");
				getPluginDesc_ = (GetPluginDescFn)Core::LibrarySymbol(handle_, "GetPluginDesc");
				getPluginVersion_ = (GetPluginVersionFn)Core::LibrarySymbol(handle_, "GetPluginVersion");
				createPlugin_ = (CreatePluginFn)Core::LibrarySymbol(handle_, "CreatePlugin");
				destroyPlugin_ = (DestroyPluginFn)Core::LibrarySymbol(handle_, "DestroyPlugin");

				if(getPluginName_ && getPluginDesc_ && getPluginVersion_ && createPlugin_ && destroyPlugin_)
				{
					version_ = getPluginVersion_();
					if(version_ == PLUGIN_VERSION)
					{
						name_ = getPluginName_();
						desc_ = getPluginDesc_();
					}
				}

				// Reset if version doesn't match.
				if(version_ != PLUGIN_VERSION)
				{
					getPluginName_ = nullptr;
					getPluginDesc_ = nullptr;
					getPluginVersion_ = nullptr;
					createPlugin_ = nullptr;
					destroyPlugin_ = nullptr;
					Core::LibraryClose(handle_);
					handle_ = 0;
				}
				else
				{
					plugin_ = createPlugin_();
				}
			}
		}

		~PluginDesc()
		{
			if(handle_)
			{
				if(plugin_)
				{
					destroyPlugin_(plugin_);
					plugin_ = nullptr;
				}
				Core::LibraryClose(handle_);
			}
		}

		PluginDesc(const PluginDesc&) = delete;

		PluginDesc(PluginDesc&& other) { swap(other); }

		void operator=(PluginDesc&& other) { swap(other); }

		void swap(PluginDesc& other)
		{
			using std::swap;
			swap(handle_, other.handle_);
			swap(getPluginName_, other.getPluginName_);
			swap(getPluginDesc_, other.getPluginDesc_);
			swap(getPluginVersion_, other.getPluginVersion_);
			swap(createPlugin_, other.createPlugin_);
			swap(destroyPlugin_, other.destroyPlugin_);
			swap(version_, other.version_);
			swap(name_, other.name_);
			swap(desc_, other.desc_);
			swap(plugin_, other.plugin_);
		}

		operator bool() const { return !!handle_ && version_ == PLUGIN_VERSION && name_ && desc_; }

		Core::LibHandle handle_ = 0;
		GetPluginNameFn getPluginName_ = nullptr;
		GetPluginDescFn getPluginDesc_ = nullptr;
		GetPluginVersionFn getPluginVersion_ = nullptr;
		CreatePluginFn createPlugin_ = nullptr;
		DestroyPluginFn destroyPlugin_ = nullptr;

		i32 version_ = 0;
		const char* name_ = nullptr;
		const char* desc_ = nullptr;

		IPlugin* plugin_ = nullptr;

		Core::Mutex mutex_;
	};

	struct ManagerImpl
	{
		Core::Vector<PluginDesc> plugins_;
	};

	Manager::Manager()
	{ //
		impl_ = new ManagerImpl(); 
	}

	Manager::~Manager()
	{ //		
		delete impl_; 
	}

	i32 Manager::Scan(const char* path)
	{
#if PLATFORM_WINDOWS
		const char* libExt = "dll";
#elif PLATFORM_LINUX || PLATFORM_OSX
		const char* libExt = "so";
#endif
		impl_->plugins_.clear();
		i32 foundLibs = Core::FileFindInPath(path, libExt, nullptr, 0);
		if(foundLibs)
		{
			Core::Vector<Core::FileInfo> fileInfos;
			fileInfos.resize(foundLibs);
			foundLibs = Core::FileFindInPath(path, libExt, fileInfos.data(), fileInfos.size());
			for(i32 i = 0; i < foundLibs; ++i)
			{
				const Core::FileInfo& fileInfo = fileInfos[i];
				PluginDesc pluginDesc(fileInfo.fileName_);
				if(pluginDesc)
				{
					impl_->plugins_.emplace_back(std::move(pluginDesc));
				}
			}
		}

		return impl_->plugins_.size();
	}

	IPlugin* GetPlugin(const char* name)
	{

		return nullptr;
	}


} // namespace Plugin
