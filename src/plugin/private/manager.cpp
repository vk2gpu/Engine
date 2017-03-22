#pragma once

#include "plugin/manager.h"

#include "core/concurrency.h"
#include "core/file.h"
#include "core/library.h"
#include "core/uuid.h"
#include "core/vector.h"
#include "core/random.h"

#include <cstring>
#include <utility>

namespace Plugin
{
	struct PluginDesc
	{
		PluginDesc() = default;
		PluginDesc(const char* libName)
		{
			fileName_.resize((i32)strlen(libName) + 1, 0);
			strcat_s(fileName_.data(), fileName_.size(), libName);

			Reload();
		}

		bool Reload()
		{
			if(handle_)
			{
				Core::LibraryClose(handle_);
			}

			validPlugin_ = false;

			handle_ = Core::LibraryOpen(fileName_.data());
			if(handle_)
			{
				getPlugin_ = (GetPluginFn)Core::LibrarySymbol(handle_, "GetPlugin");
				if(getPlugin_)
				{
					if(getPlugin_(&plugin_, Plugin::GetUUID()))
					{
						if(plugin_.systemVersion_ == PLUGIN_SYSTEM_VERSION)
						{
							validPlugin_ = true;
						}
					}
				}
			}

			return validPlugin_;
		}

		~PluginDesc()
		{
			if(handle_)
			{
				Core::LibraryClose(handle_);
			}
		}

		PluginDesc(const PluginDesc&) = delete;

		PluginDesc(PluginDesc&& other) { swap(other); }

		void operator=(PluginDesc&& other) { swap(other); }

		void swap(PluginDesc& other)
		{
			using std::swap;
			swap(fileName_, other.fileName_);
			swap(handle_, other.handle_);
			swap(getPlugin_, other.getPlugin_);
			swap(plugin_, other.plugin_);
			swap(validPlugin_, other.validPlugin_);
		}

		operator bool() const { return validPlugin_; }

		Core::Vector<char> fileName_;
		Core::LibHandle handle_ = 0;
		GetPluginFn getPlugin_ = nullptr;
		Plugin plugin_;
		bool validPlugin_ = false;
	};

	struct ManagerImpl
	{
		Core::Vector<PluginDesc> pluginDesc_;
		Core::Mutex mutex_;
		Core::Random rng_;
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
		Core::ScopedMutex lock(impl_->mutex_);
#if PLATFORM_WINDOWS
		const char* libExt = "dll";
#elif PLATFORM_LINUX || PLATFORM_OSX
		const char* libExt = "so";
#endif
		impl_->pluginDesc_.clear();
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
					pluginDesc.plugin_.internalUuid_ = Core::UUID(impl_->rng_, 0);
					impl_->pluginDesc_.emplace_back(std::move(pluginDesc));
				}
			}
		}

		return impl_->pluginDesc_.size();
	}

	bool Manager::Reload(Plugin& inOutPlugin)
	{
		for(auto& pluginDesc : impl_->pluginDesc_)
		{
			if(pluginDesc.plugin_.internalUuid_ == inOutPlugin.internalUuid_)
			{
				bool retVal = pluginDesc.Reload();
				if(retVal)
				{
					Plugin* plugin = &inOutPlugin;
					i32 num = GetPlugins(plugin->uuid_, &plugin, 1);
					if(num == 0)
						retVal = false;
				}
				return retVal;
			}
		}
		return false;
	}

	i32 Manager::GetPlugins(Core::UUID uuid, Plugin** outPlugins, i32 maxPlugins)
	{
		Core::ScopedMutex lock(impl_->mutex_);

		i32 found = 0;

		if(outPlugins)
		{
			for(auto& pluginDesc : impl_->pluginDesc_)
			{
				if(found < maxPlugins)
				{
					if(pluginDesc.getPlugin_(outPlugins[found], uuid))
					{
						outPlugins[found]->internalUuid_ = pluginDesc.plugin_.internalUuid_;
						++found;
					}
				}
				else
					return found;
			}
		}
		else
		{
			for(auto& pluginDesc : impl_->pluginDesc_)
			{
				if(pluginDesc.getPlugin_(nullptr, uuid))
					++found;
			}
		}

		return found;
	}


} // namespace Plugin
