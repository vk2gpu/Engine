#pragma once

#include "core/types.h"

/**
 * Timestamp.
 */
struct FileTimestamp
{
	u32 seconds_ = 0;
	u32 minutes_ = 0;
	u32 hours_ = 0;
	u32 monthDay_ = 0;
	u32 month_ = 0;
	u32 year_ = 0;
	u32 weekDay_ = 0;
	u32 yearDay_ = 0;
	u32 isDST_ = 0;
};

/**
 * Get file or directory stats.
 * @return Success.
 */
bool FileStats(const char* path, FileTimestamp* created, FileTimestamp* modified, i64* size);

/**
 * @return Does file exist?
 */
bool FileExists(const char* path);

/**
 * Remove file.
 * @return Success.
 */
bool FileRemove(const char* path);

/**
 * Remove directory.
 * @pre Directory must be empty.
 * @return Success.
 */
bool FileRemoveDir(const char* path);

/**
 * Rename file.
 * @return Success.
 */
bool FileRename(const char* srcPath, const char* destPath);

/**
 * Create directories.
 * Will recursively create whole path.
 * @return Success.
 */
bool FileCreateDir(const char* path);

/**
 * Change current directory.
 * @return Success.
 */
bool FileChangeDir(const char* path);

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
class File final
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
