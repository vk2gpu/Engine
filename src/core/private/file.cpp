#include "core/file.h"
#include "core/file_impl.h"
#include "core/array.h"
#include "core/concurrency.h"
#include "core/misc.h"

#if !defined(_RELEASE)
#include "core/string.h"
#endif

#include <utility>

#if PLATFORM_LINUX || PLATFORM_OSX
#include <dirent.h>
#elif PLATFORM_WINDOWS
#include <direct.h>
#pragma warning(disable : 4996) // '_open': This function or variable may be unsafe...

#define WIN32_LEAN_AND_MEAN
#include "core/os.h"
#endif

#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <wchar.h>

#if PLATFORM_WINDOWS

#define lowLevelOpen ::_open
#define lowLevelClose ::_close
#define lowLevelReadFlags (_O_RDONLY | _O_BINARY)
#define lowLevelWriteFlags (_O_WRONLY | _O_BINARY)
#define lowLevelAppendFlags (_O_APPEND)
#define lowLevelCreateFlags (_O_CREAT)
#define lowLevelPermissionFlags (_S_IREAD | _S_IWRITE)

#elif PLATFORM_LINUX || PLATFORM_OSX
#include <unistd.h>

#define lowLevelOpen ::open
#define lowLevelClose ::close
#define lowLevelReadFlags (O_RDONLY) // Binary flag does not exist on posix
#define lowLevelWriteFlags (O_WRONLY)
#define lowLevelAppendFlags (O_APPEND)
#define lowLevelCreateFlags (O_CREAT)
#define lowLevelPermissionFlags (0666)

#endif

namespace Core
{
	namespace
	{
#if PLATFORM_WINDOWS
		FileTimestamp GetTimestamp(FILETIME fileTime)
		{
			FileTimestamp timestamp;
			SYSTEMTIME systemTime;
			::FileTimeToSystemTime(&fileTime, &systemTime);
			timestamp.year_ = systemTime.wYear - 1900;
			timestamp.month_ = systemTime.wMonth - 1;
			timestamp.day_ = systemTime.wDay;
			timestamp.hours_ = systemTime.wHour;
			timestamp.minutes_ = systemTime.wMinute;
			timestamp.seconds_ = systemTime.wSecond;
			timestamp.milliseconds_ = systemTime.wMilliseconds;
			return timestamp;
		}

		void LogWin32Error(const char* message, DWORD win32Error)
		{
			LPSTR messageBuffer = nullptr;
			FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			    NULL, win32Error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
			DBG_LOG("%s. Error: %s\n", message, messageBuffer);
		}
#endif
	}

	bool FileStats(const char* path, FileTimestamp* created, FileTimestamp* modified, i64* size)
	{
		DBG_ASSERT(path);
#if PLATFORM_LINUX || PLATFORM_OSX || PLATFORM_WINDOWS
		struct stat attrib;
		if(0 == stat(path, &attrib))
		{
			if(created)
			{
				struct tm* createdTime = nullptr;
				createdTime = gmtime(&(attrib.st_ctime));
				created->year_ = (i16)createdTime->tm_year;
				created->month_ = (i16)createdTime->tm_mon;
				created->day_ = (i16)createdTime->tm_mday;
				created->hours_ = (i16)createdTime->tm_hour;
				created->minutes_ = (i16)createdTime->tm_min;
				created->seconds_ = (i16)createdTime->tm_sec;
				created->milliseconds_ = 0;
			}

			if(modified)
			{
				struct tm* modifiedTime = nullptr;
				modifiedTime = gmtime(&(attrib.st_mtime));
				modified->year_ = (i16)modifiedTime->tm_year;
				modified->month_ = (i16)modifiedTime->tm_mon;
				modified->day_ = (i16)modifiedTime->tm_mday;
				modified->hours_ = (i16)modifiedTime->tm_hour;
				modified->minutes_ = (i16)modifiedTime->tm_min;
				modified->seconds_ = (i16)modifiedTime->tm_sec;
				modified->milliseconds_ = 0;
			}

			if(size)
			{
				*size = attrib.st_size;
			}
			return true;
		}

#else
#error "Unimplemented on this platform!"
#endif
		return false;
	}

	bool FileExists(const char* path)
	{
		DBG_ASSERT(path);
#if PLATFORM_LINUX || PLATFORM_OSX || PLATFORM_WINDOWS
		struct stat attrib;
		bool retVal = false;
		if(0 == stat(path, &attrib))
		{
			retVal = true;
		}
		return retVal;
#else
#error "Unimplemented on this platform!"
		return false;
#endif
	}

	bool FileRemove(const char* path)
	{
		DBG_ASSERT(path);
#if PLATFORM_LINUX || PLATFORM_OSX || PLATFORM_WINDOWS
		return 0 == remove(path);
#else
#error "Unimplemented on this platform!";
		return false;
#endif
	}

	bool FileRemoveDir(const char* path)
	{
		DBG_ASSERT(path);
#if PLATFORM_LINUX || PLATFORM_OSX
		return rmdir(path) == 0;
#elif PLATFORM_WINDOWS
		return _rmdir(path) == 0;
#else
#error "Unimplemented on this platform!";
		return false;
#endif
	}

	bool FileRename(const char* srcPath, const char* destPath)
	{
		DBG_ASSERT(srcPath && destPath);
#if PLATFORM_LINUX || PLATFORM_OSX || PLATFORM_WINDOWS
		return rename(srcPath, destPath) == 0;
#else
#error "Unimplemented on this platform!";
		return false;
#endif
	}

	bool FileCopy(const char* srcPath, const char* destPath)
	{
		DBG_ASSERT(srcPath && destPath);
#if PLATFORM_WINDOWS
		BOOL retVal = ::CopyFile(srcPath, destPath, FALSE);
		if(retVal == FALSE)
		{
			DWORD error = ::GetLastError();
			LogWin32Error("FileCopy failed", error);
		}
		return !!retVal;
#else
#error "Unimplemented on this platform!";
		return false;
#endif
	}

	bool FileCreateDir(const char* path)
	{
		DBG_ASSERT(path);
#if PLATFORM_LINUX || PLATFORM_OSX || PLATFORM_WINDOWS
		Array<char, 256> tempPath;

		// Copy into temp path.
		strcpy_s(tempPath.data(), tempPath.size(), path);
		char* foundSeparator = nullptr;
		char* nextSearchPosition = tempPath.data();
		do
		{
			// Find separator.
			foundSeparator = strstr(nextSearchPosition, "/");
			if(foundSeparator == nullptr)
			{
				foundSeparator = strstr(nextSearchPosition, "\\");
			}

			// If found, null terminate.
			char separatorChar = 0;
			if(foundSeparator != nullptr)
			{
				separatorChar = *foundSeparator;
				*foundSeparator = '\0';
			}

			// Attempt to create the path if it doesn't exist.
			if(FileExists(tempPath.data()) == false)
			{
#if PLATFORM_WINDOWS
				int retVal = _mkdir(tempPath.data());
#else
				int retVal = mkdir(tempPath.data(), 0755);
#endif
				if(retVal)
				{
					return false;
				}
			}

			// Put separator back.
			if(separatorChar)
			{
				*foundSeparator = separatorChar;
				nextSearchPosition = foundSeparator + 1;
			}
		} while(foundSeparator != nullptr);

		return true;
#else
#error "Unimplemented on this platform!"
		return false;
#endif
	}

	bool FileChangeDir(const char* path)
	{
#if PLATFORM_LINUX || PLATFORM_OSX || PLATFORM_WINDOWS
		return chdir(path) == 0;
#else
#error "Unimplemented on this platform!";
		return false;
#endif
	}

	void FileNormalizePath(char* inoutPath, i32 maxLength, bool stripTrailing)
	{
		char platformSlash = FilePathSeparator();

		i32 counter = 0;
		while(auto& pathChar = *inoutPath++)
		{
			if(maxLength > 0)
			{
				if(counter >= maxLength)
				{
					break;
				}
			}
			if(pathChar == '\\' || pathChar == '/')
			{
				pathChar = platformSlash;

				if(stripTrailing)
				{
					if(*inoutPath == '\0')
					{
						pathChar = '\0';
						break;
					}
				}
			}
			++counter;
		}
	}
	i32 FileFindInPath(const char* path, const char* extension, FileInfo* outInfos, i32 maxInfos)
	{
#if PLATFORM_WINDOWS
		char newPath[MAX_PATH_LENGTH] = {0};
		strcpy_s(newPath, MAX_PATH_LENGTH, path);
		FileNormalizePath(newPath, MAX_PATH_LENGTH, true);

		if(extension)
		{
			strcat_s(newPath, "/*.");
			strcat_s(newPath, extension);
		}
		else
		{
			strcat_s(newPath, "/*");
		}

		i32 numFound = 0;

		WIN32_FIND_DATA findData;
		memset(&findData, 0, sizeof(findData));
		auto handle = ::FindFirstFileA(newPath, &findData);
		BOOL foundFile = TRUE;
		while(handle != INVALID_HANDLE_VALUE && foundFile)
		{
			if(outInfos && numFound < maxInfos)
			{
				FileInfo& outInfo = outInfos[numFound];
				outInfo.created_ = GetTimestamp(findData.ftCreationTime);
				outInfo.modified_ = GetTimestamp(findData.ftLastWriteTime);

				outInfo.attribs_ = FileAttribs::NONE;
				if(ContainsAllFlags(findData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
					outInfo.attribs_ |= FileAttribs::DIRECTORY;
				if(ContainsAllFlags(findData.dwFileAttributes, FILE_ATTRIBUTE_READONLY))
					outInfo.attribs_ |= FileAttribs::READ_ONLY;
				if(ContainsAllFlags(findData.dwFileAttributes, FILE_ATTRIBUTE_HIDDEN))
					outInfo.attribs_ |= FileAttribs::HIDDEN;

				outInfo.fileSize_ = ((i64)findData.nFileSizeHigh << 32LL) | (i64)findData.nFileSizeLow;
				strcpy_s(outInfo.fileName_, sizeof(outInfo.fileName_), findData.cFileName);
			}

			++numFound;

			foundFile = ::FindNextFileA(handle, &findData);
		}

		if(handle != INVALID_HANDLE_VALUE)
		{
			::FindClose(handle);
		}
		return numFound;
#else
#error "Unimplemented on this platform!";
		return 0;
#endif
	}

	char FilePathSeparator() { return '/'; }

	void FileGetCurrDir(char* buffer, i32 bufferLength)
	{
		i32 length = ::GetCurrentDirectoryA(bufferLength, buffer);
		FileNormalizePath(buffer, length, true);
	}

	bool FileSplitPath(
	    const char* inPath, char* outPath, i32 pathLen, char* outFile, i32 fileLen, char* outExt, i32 extLen)
	{
		i32 inPathLen = (i32)strlen(inPath);
		if(inPathLen >= MAX_PATH_LENGTH)
		{
			return false;
		}
		Core::Array<char, MAX_PATH_LENGTH> workBuffer;
		strcpy_s(workBuffer.data(), workBuffer.size(), inPath);

		i32 searchStart = inPathLen;

		if(searchStart > 0)
		{
			for(i32 i = searchStart - 1; i >= 0; --i)
			{
				char currChar = workBuffer[i];
				// Terminate on path separator, no extension.
				if(currChar == '\\' || currChar == '/')
				{
					outExt[0] = '\0';
					searchStart = inPathLen;
					break;
				}
				// Found extension.
				else if(currChar == '.')
				{
					if(outExt && extLen > 0)
						strcpy_s(outExt, extLen, &workBuffer[i + 1]);
					workBuffer[i] = '\0';
					searchStart = i;
					break;
				}
			}
		}

		if(searchStart > 0)
		{
			for(i32 i = searchStart - 1; i >= 0; --i)
			{
				char currChar = workBuffer[i];
				// Hit end.
				if(i == 0)
				{
					if(outFile && fileLen > 0)
						strcpy_s(outFile, fileLen, &workBuffer[i]);
					searchStart = i;
					break;
				}
				// Found path separator.
				else if(currChar == '\\' || currChar == '/')
				{
					if(outFile && fileLen > 0)
						strcpy_s(outFile, fileLen, &workBuffer[i + 1]);
					workBuffer[i] = '\0';
					searchStart = i;
					break;
				}
			}
		}

		if(outPath && pathLen > 0 && searchStart > 0)
		{
			strcpy_s(outPath, pathLen, workBuffer.data());
		}

		return true;
	}

	bool FileAppendPath(char* inOutPath, i32 maxPathLen, const char* appendPath)
	{
		const char separator[2] = {FilePathSeparator(), '\0'};

		FileNormalizePath(inOutPath, maxPathLen, true);
		if(strlen(inOutPath) > 0)
			strcat_s(inOutPath, maxPathLen, separator);
		strcat_s(inOutPath, maxPathLen, appendPath);
		return true;
	}

	/// Native file implementation.
	class FileImplGeneric : public FileImpl
	{
	public:
		FileImplGeneric(const char* path, FileFlags flags, IFilePathResolver* resolver)
		{
			char resolvedPath[MAX_PATH_LENGTH];

			// Resolve path if required.
			if(resolver)
			{
				if(resolver->ResolvePath(path, resolvedPath, sizeof(resolvedPath)))
					path = &resolvedPath[0];
			}

			// Setup flags.
			char openString[8] = {0};
			int openStringIdx = 0;
			int openPermissions = 0;
			int lowLevelFlags = 0;
			if(ContainsAllFlags(flags, FileFlags::READ))
			{
				lowLevelFlags |= lowLevelReadFlags;
				openString[openStringIdx++] = 'r';
				openString[openStringIdx++] = 'b';
			}
			if(ContainsAllFlags(flags, FileFlags::WRITE))
			{
				lowLevelFlags |= lowLevelWriteFlags;
				openString[openStringIdx++] = 'w';
				openString[openStringIdx++] = 'b';
			}
			if(ContainsAllFlags(flags, FileFlags::CREATE))
			{
				lowLevelFlags |= lowLevelCreateFlags;
				openString[openStringIdx++] = '+';
				openPermissions = lowLevelPermissionFlags;
			}

			// Load as descriptor.
			int desc = lowLevelOpen(path, lowLevelFlags, openPermissions);
			if(-1 == desc)
			{
				return;
			}
			fileDescriptor_ = desc;

			// Load as file handle.
			fileHandle_ = ::fdopen(desc, openString);
			if(nullptr == fileHandle_)
			{
				lowLevelClose(fileDescriptor_);
				fileDescriptor_ = -1;
			}

			flags_ = flags;
#if !defined(_RELEASE)
			path_ = path;
#endif
		}

		virtual ~FileImplGeneric()
		{
			if(fileHandle_)
				::fclose(fileHandle_);
		}

		i64 Read(void* buffer, i64 bytes) override
		{
			i64 bytesRead = 0;
			if(ContainsAllFlags(GetFlags(), FileFlags::READ))
			{
				DBG_ASSERT(bytes <= SIZE_MAX);
				bytesRead = ::fread(buffer, 1, bytes, fileHandle_);
			}
			return bytesRead;
		}

		i64 Write(const void* buffer, i64 bytes) override
		{
			i64 bytesWritten = 0;
			if(ContainsAllFlags(GetFlags(), FileFlags::WRITE))
			{
				DBG_ASSERT(bytes <= SIZE_MAX);
				bytesWritten = ::fwrite(buffer, 1, bytes, fileHandle_);
			}
			return bytesWritten;
		}

		bool Seek(i64 offset) override
		{
			DBG_ASSERT(offset <= SIZE_MAX);
			return 0 == ::fseek(fileHandle_, (long)offset, SEEK_SET);
		}

		i64 Tell() const override { return ::ftell(fileHandle_); }

		i64 Size() const override
		{
			i64 size = 0;
			struct stat attrib;
			if(0 == ::fstat(fileDescriptor_, &attrib))
			{
				size = attrib.st_size;
			}
			return size;
		}

		FileFlags GetFlags() const override { return flags_; }

		bool IsValid() const override { return fileDescriptor_ != -1; }

		const char* GetPath() const override
		{
#if !defined(_RELEASE)
			return path_.c_str();
#else
			return "";
#endif
		}

	private:
		FILE* fileHandle_ = nullptr;
		int fileDescriptor_ = -1;
		FileFlags flags_ = FileFlags::NONE;

#if !defined(_RELEASE)
		Core::String path_;
#endif
	};

	class FileImplMem : public FileImpl
	{
	public:
		FileImplMem(void* data, i64 size, FileFlags flags)
		    : data_(data)
		    , size_(size)
		    , flags_(flags)
		{
#if !defined(_RELEASE)
			path_.Printf("memory://%p:%lld", data, size);
#endif
		}

		FileImplMem(const void* data, i64 size)
		    : constData_(data)
		    , size_(size)
		    , flags_(FileFlags::READ)
		{
#if !defined(_RELEASE)
			path_.Printf("memory://%p:%lld", data, size);
#endif
		}

		~FileImplMem()
		{ //
		}

		i64 Read(void* buffer, i64 bytes) override
		{
			if(ContainsAnyFlags(flags_, FileFlags::READ))
			{
				const i64 remaining = size_ - offset_;
				const i64 copyBytes = Min(remaining, bytes);
				memcpy(buffer, (const u8*)constData_ + offset_, copyBytes);
				offset_ += copyBytes;
				return copyBytes;
			}
			return 0;
		}

		i64 Write(const void* buffer, i64 bytes) override
		{
			if(ContainsAnyFlags(flags_, FileFlags::WRITE))
			{
				const i64 remaining = size_ - offset_;
				const i64 copyBytes = Min(remaining, bytes);
				memcpy((u8*)data_ + offset_, buffer, copyBytes);
				offset_ += copyBytes;
				return copyBytes;
			}
			return 0;
		}

		bool Seek(i64 offset) override
		{
			if(offset < size_)
			{
				offset_ = offset;
				return true;
			}
			return false;
		}

		i64 Tell() const override { return offset_; }

		i64 Size() const override { return size_; }

		FileFlags GetFlags() const override { return flags_; }

		bool IsValid() const override { return !!constData_; }

		const char* GetPath() const override
		{
#if !defined(_RELEASE)
			return path_.c_str();
#else
			return "";
#endif
		}

	private:
		/// Choosing this over const_cast.
		union
		{
			void* data_ = nullptr;
			const void* constData_;
		};
		i64 size_ = 0;
		FileFlags flags_ = FileFlags::NONE;
		i64 offset_ = 0;

#if !defined(_RELEASE)
		Core::String path_;
#endif
	};

#if defined(PLATFORM_WINDOWS)
	class FileImplWin32 : public FileImpl
	{
	public:
		FileImplWin32(const char* path, FileFlags flags, IFilePathResolver* resolver)
		    : flags_(flags)
		{
			char resolvedPath[MAX_PATH_LENGTH] = {};

			// Resolve path if required.
			if(resolver)
			{
				if(resolver->ResolvePath(path, resolvedPath, sizeof(resolvedPath)))
					path = &resolvedPath[0];
			}

			DWORD desiredAccess = 0;
			DWORD shareMode = 0;
			DWORD createFlags = 0;
			DWORD fileFlags = 0;

			if(ContainsAnyFlags(flags, FileFlags::CREATE))
			{
				if(Core::FileExists(path))
					createFlags = TRUNCATE_EXISTING;
				else
					createFlags = CREATE_ALWAYS;
			}
			else
			{
				createFlags = OPEN_EXISTING;
			}

			if(ContainsAnyFlags(flags, FileFlags::READ))
			{
				desiredAccess |= GENERIC_READ;
				shareMode = FILE_SHARE_READ;
			}
			if(ContainsAnyFlags(flags, FileFlags::WRITE))
			{
				desiredAccess |= GENERIC_WRITE;
			}

			if(ContainsAnyFlags(flags, FileFlags::CACHE_WRITE_THROUGH))
				fileFlags |= FILE_FLAG_WRITE_THROUGH;
			if(ContainsAnyFlags(flags, FileFlags::CACHE_SEQUENTIAL))
				fileFlags |= FILE_FLAG_SEQUENTIAL_SCAN;
			if(ContainsAnyFlags(flags, FileFlags::CACHE_RANDOM_ACCESS))
				fileFlags |= FILE_FLAG_RANDOM_ACCESS;

			fileHandle_ = ::CreateFile(path, desiredAccess, shareMode, nullptr, createFlags, fileFlags, 0);
			if(fileHandle_ != INVALID_HANDLE_VALUE)
			{
				DWORD sizeHi = 0;
				DWORD sizeLo = ::GetFileSize(fileHandle_, &sizeHi);
				size_ = (i64)sizeHi << 32ull | sizeLo;
			}
			else
			{
				auto error = ::GetLastError();
				DBG_LOG("Error creating file \"%s\", GetLastError() = %x\n", path, error);
			}

#if !defined(_RELEASE)
			path_ = path;
#endif
		}

		~FileImplWin32()
		{
			if(fileHandle_ != INVALID_HANDLE_VALUE)
			{
				::FlushFileBuffers(fileHandle_);
				::CloseHandle(fileHandle_);
			}
		}

		i64 Read(void* buffer, i64 bytes) override
		{
			const i64 maxReadSize = 0x000000007fffffffull;
			i64 totalRead = 0;
			u8* readBuffer = static_cast<u8*>(buffer);
			while(bytes > 0)
			{
				DWORD bytesToRead = (DWORD)(Core::Min(maxReadSize, bytes));
				DWORD bytesRead = 0;
				if(FALSE == ::ReadFile(fileHandle_, readBuffer, bytesToRead, &bytesRead, nullptr))
				{
					return bytesRead;
				}
				totalRead += bytesRead;
				readBuffer += bytesRead;
				bytes -= bytesRead;
			}
			return totalRead;
		}

		i64 Write(const void* buffer, i64 bytes) override
		{
			const i64 maxWriteSize = 0x00000000ffffffffull;
			i64 totalWritten = 0;
			const u8* writeBuffer = static_cast<const u8*>(buffer);
			while(bytes > 0)
			{
				DWORD bytesToWrite = (DWORD)(Core::Min(maxWriteSize, bytes));
				DWORD bytesWritten = 0;
				if(FALSE == ::WriteFile(fileHandle_, writeBuffer, bytesToWrite, &bytesWritten, nullptr))
				{
					return bytesWritten;
				}
				totalWritten += bytesWritten;
				writeBuffer += bytesWritten;
				bytes -= bytesWritten;
			}
			return totalWritten;
		}

		bool Seek(i64 offset) override
		{
			const LONG offsetHi = offset >> 32u;
			const LONG offsetLo = offset & 0xffffffffu;
			LONG offsetHiOut = offsetHi;
			return ::SetFilePointer(fileHandle_, offsetLo, &offsetHiOut, FILE_BEGIN) == (DWORD)offsetLo &&
			       offsetHiOut == offsetHi;
		}

		i64 Tell() const override
		{
			LONG offsetHi = 0;
			DWORD offsetLo = ::SetFilePointer(fileHandle_, 0, &offsetHi, FILE_CURRENT);
			return (i64)offsetHi << 32ull | offsetLo;
		}

		i64 Size() const override { return size_; }

		FileFlags GetFlags() const override { return flags_; }

		bool IsValid() const override { return fileHandle_ != INVALID_HANDLE_VALUE; }

		const char* GetPath() const override
		{
#if !defined(_RELEASE)
			return path_.c_str();
#else
			return "";
#endif
		}

	private:
		friend class MappedFileImpl;

		i64 size_ = 0;
		FileFlags flags_ = FileFlags::NONE;
		HANDLE fileHandle_ = INVALID_HANDLE_VALUE;
		volatile i32 mappedCount_ = 0;

#if !defined(_RELEASE)
		Core::String path_;
#endif
	};

	class MappedFileImpl
	{
	public:
		MappedFileImpl(FileImpl* fileImpl, i64 offset, i64 size)
		{
			const FileFlags flags = fileImpl->GetFlags();

			// Have to explicitly declare MMAP to be memory mappable.
			if(!ContainsAllFlags(flags, FileFlags::MMAP))
			{
				return;
			}

			fileImpl_ = static_cast<FileImplWin32*>(fileImpl);

			DWORD protect = 0;
			DWORD desiredAccess = 0;
			if(ContainsAnyFlags(flags, FileFlags::WRITE) && ContainsAnyFlags(flags, FileFlags::READ))
			{
				protect |= PAGE_READWRITE;
				desiredAccess |= FILE_MAP_WRITE | FILE_MAP_READ;
			}
			else if(ContainsAnyFlags(flags, FileFlags::WRITE))
			{
				protect |= PAGE_READWRITE | PAGE_WRITECOMBINE;
				desiredAccess |= FILE_MAP_WRITE;
			}
			else if(ContainsAnyFlags(flags, FileFlags::READ))
			{
				protect |= PAGE_READONLY;
				desiredAccess |= FILE_MAP_READ;
			}

			size_ = size;

			// Round offset down to nearest page, and round up the size appropriately.
			SYSTEM_INFO sysInfo = {};
			::GetSystemInfo(&sysInfo);
			const i64 newOffset = Core::PotRoundDown(offset, sysInfo.dwAllocationGranularity);
			const i64 newSize = size_ + (offset - newOffset);

			const DWORD sizeHi = newSize >> 32ull;
			const DWORD sizeLo = newSize & 0xffffffffull;
			mappingHandle_ =
			    ::CreateFileMapping(fileImpl_->fileHandle_, nullptr, protect, sizeHi, sizeLo, fileImpl_->GetPath());
			if(mappingHandle_ == INVALID_HANDLE_VALUE)
				return;

			Core::AtomicInc(&fileImpl_->mappedCount_);

			const DWORD offsetHi = newOffset >> 32u;
			const DWORD offsetLo = newOffset & 0xffffffffu;
			baseAddress_ = ::MapViewOfFile(mappingHandle_, desiredAccess, offsetHi, offsetLo, newSize);
			if(baseAddress_)
			{
				mappedAddress_ = (u8*)baseAddress_ + offset;
			}
			else
			{
				auto error = ::GetLastError();
				DBG_LOG("Error mapping file \"%s\", GetLastError() = %x\n", fileImpl_->GetPath(), error);
				return;
			}
		}

		~MappedFileImpl()
		{
			if(mappedAddress_)
			{
				::UnmapViewOfFile(baseAddress_);
				mappedAddress_ = nullptr;
			}

			if(mappingHandle_ != INVALID_HANDLE_VALUE)
			{
				::CloseHandle(mappingHandle_);
				mappingHandle_ = INVALID_HANDLE_VALUE;

				Core::AtomicDec(&fileImpl_->mappedCount_);
			}
		}

		bool IsValid() const { return !!mappedAddress_; }

		FileImplWin32* fileImpl_ = nullptr;

		HANDLE mappingHandle_ = INVALID_HANDLE_VALUE;
		i64 size_ = 0;
		void* baseAddress_ = nullptr;
		void* mappedAddress_ = nullptr;
	};
#endif // defined(PLATFORM_WINDOWS)

	File::File(const char* path, FileFlags flags, IFilePathResolver* resolver)
	{
		DBG_ASSERT((ContainsAnyFlags(flags, FileFlags::READ) ^ ContainsAnyFlags(flags, FileFlags::WRITE)) ||
		           ContainsAnyFlags(flags, FileFlags::MMAP));
		DBG_ASSERT(ContainsAnyFlags(flags, FileFlags::WRITE) ||
		           (ContainsAnyFlags(flags, FileFlags::READ) && !ContainsAnyFlags(flags, FileFlags::CREATE)));

#if defined(PLATFORM_WINDOWS)
		impl_ = new FileImplWin32(path, flags, resolver);
#else
		impl_ = new FileImplGeneric(path, flags, resolver);
#endif

		if(!impl_->IsValid())
		{
			delete impl_;
			impl_ = nullptr;
		}
	}

	File::File(void* data, i64 size, FileFlags flags)
	{
		DBG_ASSERT(data);
		DBG_ASSERT(size > 0);
		DBG_ASSERT(ContainsAnyFlags(flags, FileFlags::READ) ^ ContainsAnyFlags(flags, FileFlags::WRITE));
		DBG_ASSERT(!ContainsAnyFlags(flags, FileFlags::CREATE));
		impl_ = new FileImplMem(data, size, flags);
	}

	File::File(const void* data, i64 size)
	{
		DBG_ASSERT(data);
		DBG_ASSERT(size > 0);
		impl_ = new FileImplMem(data, size);
	}


	File::~File() { delete impl_; }

	File::File(File&& other)
	{
		using std::swap;
		swap(impl_, other.impl_);
	}

	File& File::operator=(File&& other)
	{
		using std::swap;
		swap(impl_, other.impl_);
		return *this;
	}


	i64 File::Read(void* buffer, i64 bytes)
	{
		DBG_ASSERT(ContainsAllFlags(GetFlags(), FileFlags::READ));
		return impl_->Read(buffer, bytes);
	}

	i64 File::Write(const void* buffer, i64 bytes)
	{
		DBG_ASSERT(ContainsAllFlags(GetFlags(), FileFlags::WRITE));
		DBG_ASSERT(bytes > 0);
		return impl_->Write(buffer, bytes);
	}

	bool File::Seek(i64 offset)
	{
		DBG_ASSERT(ContainsAnyFlags(GetFlags(), FileFlags::READ | FileFlags::WRITE));
		DBG_ASSERT(offset >= 0);
		return impl_->Seek(offset);
	}

	i64 File::Tell() const
	{
		DBG_ASSERT(ContainsAnyFlags(GetFlags(), FileFlags::READ | FileFlags::WRITE));
		return impl_->Tell();
	}

	i64 File::Size() const
	{
		DBG_ASSERT(impl_);
		return impl_->Size();
	}

	FileFlags File::GetFlags() const
	{ //
		return impl_ ? impl_->GetFlags() : FileFlags::NONE;
	}

	const char* File::GetPath() const
	{ //
		return impl_ ? impl_->GetPath() : "<NULL>";
	}

	MappedFile::MappedFile(File& file, i64 offset, i64 size)
	{
		if(file.impl_)
		{
			impl_ = new MappedFileImpl(file.impl_, offset, size);
			if(!impl_->IsValid())
			{
				delete impl_;
				impl_ = nullptr;
			}
		}
	}

	MappedFile::~MappedFile() { delete impl_; }

	MappedFile::MappedFile(MappedFile&& other)
	{
		using std::swap;
		swap(impl_, other.impl_);
	}

	MappedFile& MappedFile::operator=(MappedFile&& other)
	{
		using std::swap;
		swap(impl_, other.impl_);
		return *this;
	}

	void* MappedFile::GetAddress() const { return impl_->mappedAddress_; }

	i64 MappedFile::GetSize() const { return impl_->size_; }


} // namespace Core
