#include "resource/data_cache.h"
#include "core/file.h"
#include "core/string.h"
#include "core/vector.h"


namespace Resource
{
	struct DataCacheImpl
	{
		/// Local path to cache in.
		Core::String localPath_;
		/// Remote path to pull from/push to.
		Core::String remotePath_;
	};

	namespace
	{
		void AppendHashPath(char* outPath, i32 outPathLen, const DataHash& hash)
		{
			const auto writeChar = [](char*& outStr, char* end, char byte)
			{
				if((end - outStr) > 1)
					*outStr++ = byte;
			};

			const auto writeByte = [&](char*& outStr, char* end, u8 byte) {
				char base0 = '0';
				char base10 = 'a' - 10;
				u8 b0 = (byte >> 4) & 0x0f;
				u8 b1 = byte & 0x0f;

				if(b0 < 10)
					writeChar(outStr, end, base0 + b0);
				else
					writeChar(outStr, end, base10 + b0);
				if(b1 < 10)
					writeChar(outStr, end, base0 + b1);
				else
					writeChar(outStr, end, base10 + b1);
			};

			char* outChar = outPath;
			char* end = outPath + outPathLen;
			
			writeByte(outChar, end, hash.data8_[18]);
			writeChar(outChar, end, '/');
			writeByte(outChar, end, hash.data8_[19]);
			writeChar(outChar, end, '/');
			for(i32 i = 0; i < 20; ++i)
				writeByte(outChar, end, hash.data8_[i]);
			
			*outChar = '\0';
		}
	}


	DataCache::DataCache()
	{
		impl_ = new DataCacheImpl();

		impl_->localPath_ = R"(D:/engine_cache/local)";
		impl_->remotePath_ = R"(\\neilo-desktop\D\engine_cache\remote)";
	}

	DataCache::~DataCache()
	{
		delete impl_;		
	}

	bool DataCache::Exists(const DataHash& hash)
	{
		char outFilename[1024] = {};
		AppendHashPath(outFilename, sizeof(outFilename), hash);

		char outLocalPath[1024] = {};
		char outRemotePath[1024] = {};
		sprintf_s(outLocalPath, sizeof(outLocalPath), "%s/%s", impl_->localPath_.c_str(), outFilename);
		sprintf_s(outRemotePath, sizeof(outRemotePath), "%s/%s", impl_->remotePath_.c_str(), outFilename);

		return Core::FileExists(outLocalPath) || Core::FileExists(outRemotePath);
	}

	i64 DataCache::GetSize(const DataHash& hash)
	{
		char outFilename[1024] = {};
		AppendHashPath(outFilename, sizeof(outFilename), hash);

		char outLocalPath[1024] = {};
		char outRemotePath[1024] = {};
		sprintf_s(outLocalPath, sizeof(outLocalPath), "%s/%s", impl_->localPath_.c_str(), outFilename);
		sprintf_s(outRemotePath, sizeof(outRemotePath), "%s/%s", impl_->remotePath_.c_str(), outFilename);

		// Check local first.
		if(Core::FileExists(outLocalPath))
		{
			i64 size = -1;
			Core::FileStats(outLocalPath, nullptr, nullptr, &size);
			return size;
		}

		// Now check remote.
		if(Core::FileExists(outRemotePath))
		{
			i64 size = -1;
			Core::FileStats(outRemotePath, nullptr, nullptr, &size);
			return size;
		}

		return -1;
	}
	
	bool DataCache::Write(const DataHash& hash, const void* data, i64 size)
	{
		char outFilename[1024] = {};
		AppendHashPath(outFilename, sizeof(outFilename), hash);

		char outLocalPath[1024] = {};
		char outRemotePath[1024] = {};
		sprintf_s(outLocalPath, sizeof(outLocalPath), "%s/%s", impl_->localPath_.c_str(), outFilename);
		sprintf_s(outRemotePath, sizeof(outRemotePath), "%s/%s", impl_->remotePath_.c_str(), outFilename);

		if(!Core::FileExists(outLocalPath))
		{
			DBG_VERIFY(Core::FileCreateDir(outLocalPath));
		}

		char outLocalDataPath[1024] = {};
		char outRemoteDataPath[1024] = {};
		sprintf_s(outLocalDataPath, sizeof(outLocalDataPath), "%s/data", outLocalPath);
		sprintf_s(outRemoteDataPath, sizeof(outRemoteDataPath), "%s/data", outRemotePath);

		bool doCopy = false;
		if(auto file = Core::File(outLocalDataPath, Core::FileFlags::DEFAULT_WRITE))
		{
			file.Write(data, size);

			doCopy = true;
		}

		// TODO: Move to background thread?
		if(doCopy)
		{
			if(!Core::FileExists(outRemotePath))
			{
				DBG_VERIFY(Core::FileCreateDir(outRemotePath));
			}

			Core::FileCopy(outLocalDataPath, outRemoteDataPath);

			return true;
		}

		return false;
	}

	bool DataCache::Read(const DataHash& hash, void* data, i64 size)
	{
		char outFilename[1024] = {};
		AppendHashPath(outFilename, sizeof(outFilename), hash);

		char outLocalPath[1024] = {};
		char outRemotePath[1024] = {};
		sprintf_s(outLocalPath, sizeof(outLocalPath), "%s/%s", impl_->localPath_.c_str(), outFilename);
		sprintf_s(outRemotePath, sizeof(outRemotePath), "%s/%s", impl_->remotePath_.c_str(), outFilename);

		if(!Core::FileExists(outLocalPath))
		{
			DBG_VERIFY(Core::FileCreateDir(outLocalPath));
		}

		char outLocalDataPath[1024] = {};
		char outRemoteDataPath[1024] = {};
		sprintf_s(outLocalDataPath, sizeof(outLocalDataPath), "%s/data", outLocalPath);
		sprintf_s(outRemoteDataPath, sizeof(outRemoteDataPath), "%s/data", outRemotePath);
		
		if(!Core::FileExists(outLocalDataPath))
		{
			if(!Core::FileCopy(outRemoteDataPath, outLocalDataPath))
			{
				return false;
			}
		}

		if(auto file = Core::File(outLocalDataPath, Core::FileFlags::DEFAULT_READ))
		{
			file.Read(data, size);
			return true;
		}
		return false;
	}

} // namespace Resource
