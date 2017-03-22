#pragma once

#include "plugin/manager.h"

#include "core/concurrency.h"
#include "core/file.h"
#include "core/library.h"
#include "core/map.h"
#include "core/uuid.h"

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
		Core::Map<Core::UUID, PluginDesc*> pluginDesc_;
		Core::Mutex mutex_;
	};

	Manager::Manager()
	{ //
		impl_ = new ManagerImpl(); 
	}

	Manager::~Manager()
	{ //		
		for(auto pluginDescIt : impl_->pluginDesc_)
		{
			delete pluginDescIt.second;
		}
		impl_->pluginDesc_.clear();

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
		i32 foundLibs = Core::FileFindInPath(path, libExt, nullptr, 0);
		if(foundLibs)
		{
			Core::Vector<Core::FileInfo> fileInfos;
			fileInfos.resize(foundLibs);
			foundLibs = Core::FileFindInPath(path, libExt, fileInfos.data(), fileInfos.size());
			for(i32 i = 0; i < foundLibs; ++i)
			{
				const Core::FileInfo& fileInfo = fileInfos[i];
				PluginDesc* pluginDesc = new PluginDesc(fileInfo.fileName_);
				if(*pluginDesc)
				{
					pluginDesc->plugin_.fileName_ = pluginDesc->fileName_.data();
					pluginDesc->plugin_.fileUuid_ = Core::UUID(pluginDesc->plugin_.fileName_);

					if(impl_->pluginDesc_.find(pluginDesc->plugin_.fileUuid_) == impl_->pluginDesc_.end())
					{
						impl_->pluginDesc_.insert(pluginDesc->plugin_.fileUuid_, pluginDesc);
					}
				}
				else
				{
					delete pluginDesc;
				}
			}
		}

		return impl_->pluginDesc_.size();
	}

	bool Manager::Reload(Plugin& inOutPlugin)
	{
		auto it = impl_->pluginDesc_.find(inOutPlugin.fileUuid_);
		if(it != impl_->pluginDesc_.end())
		{
			bool retVal = it->second->Reload();
			if(retVal)
			{
				Plugin* plugin = &inOutPlugin;
				i32 num = GetPlugins(plugin->uuid_, &plugin, 1);
				if(num == 0)
					retVal = false;
			}
			else
			{
				// Erase plugin that failed to reload?
				delete it->second;
				impl_->pluginDesc_.erase(it);
			}
			return retVal;
		}
		return false;
	}

	i32 Manager::GetPlugins(Core::UUID uuid, Plugin** outPlugins, i32 maxPlugins)
	{
		Core::ScopedMutex lock(impl_->mutex_);

		i32 found = 0;

		if(outPlugins)
		{
			for(auto& it : impl_->pluginDesc_)
			{
				if(found < maxPlugins)
				{
					if(it.second->getPlugin_(outPlugins[found], uuid))
					{
						outPlugins[found]->fileUuid_ = it.second->plugin_.fileUuid_;
						outPlugins[found]->fileName_ = it.second->fileName_.data();
						++found;
					}
				}
				else
					return found;
			}
		}
		else
		{
			for(auto& it : impl_->pluginDesc_)
			{
				if(it.second->getPlugin_(nullptr, uuid))
					++found;
			}
		}

		return found;
	}


} // namespace Plugin
