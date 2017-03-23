#include "core/file.h"
#include "core/array.h"
#include "core/misc.h"

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
			if(outInfos)
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

	char FilePathSeparator()
	{
#if PLATFORM_WINDOWS
		return '\\';
#else
		return '/';
#endif
	}

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

		if(outExt && extLen > 0 && searchStart > 0)
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
					strcpy_s(outExt, extLen, &workBuffer[i + 1]);
					workBuffer[i] = '\0';
					searchStart = i;
					break;
				}
			}
		}

		if(outFile && fileLen > 0 && searchStart > 0)
		{
			for(i32 i = searchStart - 1; i >= 0; --i)
			{
				char currChar = workBuffer[i];
				// Hit end.
				if(i == 0)
				{
					strcpy_s(outFile, fileLen, &workBuffer[i]);
					searchStart = i;
					break;
				}
				// Found path separator.
				else if(currChar == '\\' || currChar == '/')
				{
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
	{ //
		return impl_ ? impl_->flags_ : FileFlags::NONE;
	}
} // namespace Core
