#pragma once

#include "core/types.h"
#include "core/dll.h"

namespace Core
{
	/// Maximum path length supported. Platform may vary.
	static const i32 MAX_PATH_LENGTH = 512;

	/**
	 * Timestamp.
	 */
	struct FileTimestamp
	{
		/// years since 1900
		i16 year_ = -1;
		/// months since January - [0, 11]
		i16 month_ = -1;
		/// day of the month - [1, 31]
		i16 day_ = -1;
		/// hours since midnight - [0, 23]
		i16 hours_ = -1;
		/// minutes after the hour - [0, 59]
		i16 minutes_ = -1;
		/// seconds after the minute - [0, 60]
		i16 seconds_ = -1;
		/// milliseconds after the second - [0, 999]
		i16 milliseconds_ = -1;

		bool operator==(const FileTimestamp& other) const
		{
			return year_ == other.year_ && month_ == other.month_ && day_ == other.day_ && hours_ == other.hours_ &&
			       minutes_ == other.minutes_ && seconds_ == other.seconds_ && milliseconds_ == other.milliseconds_;
		}

		bool operator!=(const FileTimestamp& other) const
		{
			return year_ != other.year_ || month_ != other.month_ || day_ != other.day_ || hours_ != other.hours_ ||
			       minutes_ != other.minutes_ || seconds_ != other.seconds_ || milliseconds_ != other.milliseconds_;
		}

		bool operator<(const FileTimestamp& other) const
		{
			if(year_ < other.year_)
				return true;
			if(year_ > other.year_)
				return false;
			if(month_ < other.month_)
				return true;
			if(month_ > other.month_)
				return false;
			if(day_ < other.day_)
				return true;
			if(day_ > other.day_)
				return false;
			if(hours_ < other.hours_)
				return true;
			if(hours_ > other.hours_)
				return false;
			if(minutes_ < other.minutes_)
				return true;
			if(minutes_ > other.minutes_)
				return false;
			if(seconds_ < other.seconds_)
				return true;
			if(seconds_ > other.seconds_)
				return false;
			if(milliseconds_ < other.milliseconds_)
				return true;
			if(milliseconds_ > other.milliseconds_)
				return false;
			return false;
		}
	};

	/**
	 * File attributes.
	 */
	enum class FileAttribs : u32
	{
		NONE = 0x0,
		DIRECTORY = 0x1,
		READ_ONLY = 0x2,
		HIDDEN = 0x4,
	};

	DEFINE_ENUM_CLASS_FLAG_OPERATOR(FileAttribs, |);
	DEFINE_ENUM_CLASS_FLAG_OPERATOR(FileAttribs, &);

	/**
	 * File info structure.
	 */
	struct FileInfo
	{
		/// Created time.
		FileTimestamp created_;
		/// Modified time.
		FileTimestamp modified_;
		/// File size.
		i64 fileSize_;
		/// Attributes.
		FileAttribs attribs_;
		/// Filename in UTF-8.
		char fileName_[MAX_PATH_LENGTH];
	};

	/**
	 * Get file or directory stats.
	 * @return Success.
	 */
	CORE_DLL bool FileStats(const char* path, FileTimestamp* created, FileTimestamp* modified, i64* size);

	/**
	 * @return Does file exist?
	 */
	CORE_DLL bool FileExists(const char* path);

	/**
	 * Remove file.
	 * @return Success.
	 */
	CORE_DLL bool FileRemove(const char* path);

	/**
	 * Remove directory.
	 * @pre Directory must be empty.
	 * @return Success.
	 */
	CORE_DLL bool FileRemoveDir(const char* path);

	/**
	 * Rename file.
	 * @return Success.
	 */
	CORE_DLL bool FileRename(const char* srcPath, const char* destPath);

	/**
	 * Copy file. Will overwrite existing file.
	 * @return Success.
	 */
	CORE_DLL bool FileCopy(const char* srcPath, const char* destPath);

	/**
	 * Create directories.
	 * Will recursively create whole path.
	 * @return Success.
	 */
	CORE_DLL bool FileCreateDir(const char* path);

	/**
	 * Change current directory.
	 * @return Success.
	 */
	CORE_DLL bool FileChangeDir(const char* path);

	/**
	 * Normalize path slashes for current platform.
	 * @param inoutPath Path to normalize.
	 * @param maxLength Maximum length of path (can be 0 to avoid safety check).
	 * @param stripTrailing Strip trailing slash.
	 */
	CORE_DLL void FileNormalizePath(char* inoutPath, i32 maxLength, bool stripTrailing);

	/**
	 * Find files in path.
	 * @param path Path to search
	 * @param extension Extensions to include. nullptr to ignore.
	 * @param outInfos Output file infos. nullptr is valid.
	 * @param maxInfo Maximum number of infos to fill in.
	 * @param Number of files found.
	 */
	CORE_DLL i32 FileFindInPath(const char* path, const char* extension, FileInfo* outInfos, i32 maxInfos);

	/**
	 * Get path separator.
	 */
	CORE_DLL char FilePathSeparator();

	/**
	 * Get current directory.
	 * Will return with trailing slash stripped and paths normalized for platform.
	 */
	CORE_DLL void FileGetCurrDir(char* buffer, i32 bufferLength);

	/**
	 * Split path into elements.
	 * @param inPath Input path to split. ("C:\Path\To\File.txt" or "/path/to/file.txt")
	 * @param outPath Output path (optional). ("C:\Path\To" or "/path/to")
	 * @param pathLen Maximum length of output path buffer.
	 * @param outPath Output file (optional). ("File" or "file")
	 * @param pathLen Maximum length of output file buffer.
	 * @param outPath Output extension (optional). ("txt" or"txt")
	 * @param pathLen Maximum length of output ext buffer.
	 * @pre strlen(inPath) < MAX_PATH_LENGTH.
	 * @return true is successful. false if failure.
	 */
	CORE_DLL bool FileSplitPath(
	    const char* inPath, char* outPath, i32 pathLen, char* outFile, i32 fileLen, char* outExt, i32 extLen);

	/**
	 * Append path.
	 * @parma inOutPath Path to append to.
	 * @param maxPathLen Maximum length of path.
	 * @param appendPath Path to append.
	 */
	CORE_DLL bool FileAppendPath(char* inOutPath, i32 maxPathLen, const char* appendPath);

	/**
	 * Flags which define behaviour or file.
	 */
	enum class FileFlags : u32
	{
		NONE = 0x0,
		READ = 0x1,
		WRITE = 0x2,
		APPEND = 0x4,
		CREATE = 0x8,
	};

	DEFINE_ENUM_CLASS_FLAG_OPERATOR(FileFlags, |);
	DEFINE_ENUM_CLASS_FLAG_OPERATOR(FileFlags, &);

	/**
	 * Path resolver.
	 */
	class IFilePathResolver
	{
	public:
		IFilePathResolver() {}
		virtual ~IFilePathResolver() {}

		/**
		 * Resolve path.
		 * @param inPath Input path. (i.e. my_file.txt)
		 * @param outPath Output path. (i.e. res/my_file.txt)
		 * @param maxOutPath Maximum output path length.
		 * @return true if successfully resolved.
		 */
		virtual bool ResolvePath(const char* inPath, char* outPath, i32 maxOutPath) = 0;

		/**
		 * Get original path from already resolved path.
		 */
		virtual bool OriginalPath(const char* inPath, char* outPath, i32 maxOutPath) = 0;
	};

	/**
	 * File class.
	 */
	class CORE_DLL File final
	{
	public:
		File() = default;

		/**
		 * Open file.
		 * @param path Path to open.
		 * @param flags Flags to control file behavior.
		 * @param resolver Path resolver to use. Can be nullptr.
		 * @pre @a flags only contains READ or WRITE, but not both.
		 * @pre If @a flags contains READ, it doesn't contain APPEND or CREATE.
		 */
		File(const char* path, FileFlags flags, IFilePathResolver* resolver = nullptr);

		/**
		 * Create file from memory.
		 * @param data Pointer to data.
		 * @param size Size available for use.
		 * @param Flags to control file behavior.
		 * @pre size > 0.
		 * @pre flags only contains READ or WRITE, but not both.
		 */
		File(void* data, i64 size, FileFlags flags);

		/**
		 * Create file from memory (read-only).
		 * @param data Pointer to data.
		 * @param size Size available for use.
		 * @pre size > 0.
		 */
		File(const void* data, i64 size);

		~File();

		/// Move operators.
		File(File&&);
		File& operator=(File&&);

		/**
		 * Read bytes from file.
		 * @param buffer Buffer to read into.
		 * @param bytes Bytes to read.
		 * @return Bytes read.
		 * @pre GetFlags contains FileFlags::READ.
		 * @pre buffer != nullptr.
		 * @pre bytes > 0.
		 */
		i64 Read(void* buffer, i64 bytes);

		/**
		 * Write bytes to end of file.
		 * @param buffer Buffer to write.
		 * @param bytes Bytes to write.
		 * @return Bytes written.
		 * @pre GetFlags contains FileFlags::WRITE.
		 * @pre buffer != nullptr.
		 * @pre bytes > 0.
		 */
		i64 Write(const void* buffer, i64 bytes);

		/**
		 * Seek to location within file.
		 * @param offset Offset in bytes.
		 * @pre GetFlags contains FileFlags::READ or FileFlags::WRITE.
		 * @pre offset >= 0.
		 * @return Moved location successfully.
		 */
		bool Seek(i64 offset);

		/**
		 * Get current read/write position in file..
		 * @param offset Offset in bytes.
		 * @pre GetFlags contains FileFlags::READ or FileFlags::WRITE.
		 */
		i64 Tell() const;

		/**
		 * @return File size.
		 */
		i64 Size() const;

		/**
		 * @return flags.
		 */
		FileFlags GetFlags() const;

		/**
		 * @return Is file valid?
		 */
		operator bool() const { return impl_ != nullptr; }

	private:
		File(const File&) = delete;

		class FileImpl* impl_ = nullptr;
	};
} // namespace Core