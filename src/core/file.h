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
	 * File class.
	 */
	class CORE_DLL File final
	{
	public:
		File() = default;

		/**
		 * Open file.
		 * @param path Path to open.
		 * @param
		 */
		File(const char* path, FileFlags flags);
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

		struct FileImpl* impl_ = nullptr;
	};
} // namespace Core