#include "core/file.h"
#include "core/array.h"
#include "core/misc.h"

#include <utility>

#if PLATFORM_LINUX || PLATFORM_OSX
#include <dirent.h>
#elif PLATFORM_WINDOWS
#include <direct.h>
#pragma warning(disable : 4996) // '_open': This function or variable may be unsafe...
#endif

#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

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
			created->seconds_ = createdTime->tm_sec;
			created->minutes_ = createdTime->tm_min;
			created->hours_ = createdTime->tm_hour;
			created->monthDay_ = createdTime->tm_mday;
			created->month_ = createdTime->tm_mon;
			created->year_ = createdTime->tm_year;
			created->weekDay_ = createdTime->tm_wday;
			created->yearDay_ = createdTime->tm_yday;
			created->isDST_ = createdTime->tm_isdst;
		}

		if(modified)
		{
			struct tm* modifiedTime = nullptr;
			modifiedTime = gmtime(&(attrib.st_mtime));
			modified->seconds_ = modifiedTime->tm_sec;
			modified->minutes_ = modifiedTime->tm_min;
			modified->hours_ = modifiedTime->tm_hour;
			modified->monthDay_ = modifiedTime->tm_mday;
			modified->month_ = modifiedTime->tm_mon;
			modified->year_ = modifiedTime->tm_year;
			modified->weekDay_ = modifiedTime->tm_wday;
			modified->yearDay_ = modifiedTime->tm_yday;
			modified->isDST_ = modifiedTime->tm_isdst;
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

struct FileImpl
{
	FILE* fileHandle_ = nullptr;
	int fileDescriptor_ = -1;
	FileFlags flags_ = FileFlags::NONE;
};

File::File(const char* path, FileFlags flags)
{
	impl_ = new FileImpl();

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
	if(ContainsAllFlags(flags, FileFlags::APPEND))
	{
		lowLevelFlags |= lowLevelAppendFlags;
		openString[openStringIdx++] = 'a';
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
		delete impl_;
		impl_ = nullptr;
		return;
	}
	impl_->fileDescriptor_ = desc;

	// Load as file handle.
	impl_->fileHandle_ = ::fdopen(desc, openString);
	if(nullptr == impl_->fileHandle_)
	{
		lowLevelClose(impl_->fileDescriptor_);
		delete impl_;
		impl_ = nullptr;
	}

	impl_->flags_ = flags;
}

File::~File()
{
	if(impl_)
	{
		::fclose(impl_->fileHandle_);
		delete impl_;
	}
}

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
	i64 bytesRead = 0;
	if(ContainsAllFlags(GetFlags(), FileFlags::READ))
	{
		DBG_ASSERT(bytes <= SIZE_MAX);
		bytesRead = fread(buffer, 1, bytes, impl_->fileHandle_);
	}
	return bytesRead;
}

i64 File::Write(const void* buffer, i64 bytes)
{
	DBG_ASSERT(ContainsAllFlags(GetFlags(), FileFlags::WRITE));
	DBG_ASSERT(bytes > 0);
	i64 bytesWritten = 0;
	if(ContainsAllFlags(GetFlags(), FileFlags::WRITE))
	{
		DBG_ASSERT(bytes <= SIZE_MAX);
		bytesWritten = fwrite(buffer, 1, bytes, impl_->fileHandle_);
	}
	return bytesWritten;
}

bool File::Seek(i64 offset)
{
	DBG_ASSERT(ContainsAnyFlags(GetFlags(), FileFlags::READ | FileFlags::WRITE));
	DBG_ASSERT(offset >= 0);
	DBG_ASSERT(offset <= SIZE_MAX);
	return 0 == fseek(impl_->fileHandle_, (long)offset, SEEK_SET);
}

i64 File::Tell() const
{
	DBG_ASSERT(ContainsAnyFlags(GetFlags(), FileFlags::READ | FileFlags::WRITE));
	return ftell(impl_->fileHandle_);
}

i64 File::Size() const
{
	DBG_ASSERT(impl_);
	i64 size = 0;
	struct stat attrib;
	if(0 == fstat(impl_->fileDescriptor_, &attrib))
	{
		size = attrib.st_size;
	}
	return size;
}

FileFlags File::GetFlags() const
{
	return impl_ ? impl_->flags_ : FileFlags::NONE;
}
