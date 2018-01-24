#include "core/array.h"
#include "core/debug.h"
#include "core/file.h"
#include "core/random.h"
#include "core/vector.h"

#include "catch.hpp"

namespace
{
	const char* fileName = "file_test_output";
	const char* folder1 = "file_test_folder";
	const char* folder2 = "file_test_folder/subfolder";

	const u8 testData[8] = {1, 2, 3, 4, 5, 6, 7, 8};

	void Cleanup()
	{
		if(Core::FileExists(fileName))
		{
			REQUIRE(Core::FileRemove(fileName));
		}

		if(Core::FileExists(folder2))
		{
			REQUIRE(Core::FileRemoveDir(folder2));
		}

		if(Core::FileExists(folder1))
		{
			REQUIRE(Core::FileRemoveDir(folder1));
		}
	}

	struct ScopedCleanup
	{
		ScopedCleanup() { Cleanup(); }

		~ScopedCleanup() { Cleanup(); }
	};

	void WriteTestData(Core::File& file, i64 offset, i64 bytes)
	{
		DBG_ASSERT(bytes <= 8);
		if(offset == 0)
		{
			REQUIRE(file.Write(&testData[0], bytes) == bytes);
		}
		else
		{
			for(i64 i = 0; i < bytes; ++i)
			{
				i64 j = (i + offset) % 8;
				REQUIRE(file.Write(&testData[j], 1) == 1);
			}
		}
	}

	void ReadTestData(Core::File& file, i64 offset, i64 bytes)
	{
		DBG_ASSERT(bytes <= 8);
		u8 readData[8] = {0};
		REQUIRE(file.Read(readData, bytes));
		for(i64 i = 0; i < bytes; ++i)
		{
			i64 j = (i + offset) % 8;
			CHECK(readData[i] == testData[j]);
		}
	}
}

TEST_CASE("file-tests-create")
{
	ScopedCleanup scopedCleanup;

	{
		Core::File file(fileName, Core::FileFlags::DEFAULT_WRITE);
	}

	REQUIRE(Core::FileExists(fileName));
}

TEST_CASE("file-tests-write")
{
	ScopedCleanup scopedCleanup;

	{
		Core::File file(fileName, Core::FileFlags::DEFAULT_WRITE);
		WriteTestData(file, 0, 8);
	}

	REQUIRE(Core::FileExists(fileName));
	{
		Core::File file(fileName, Core::FileFlags::READ);
		ReadTestData(file, 0, 8);
	}
}

TEST_CASE("file-tests-tell")
{
	ScopedCleanup scopedCleanup;

	{
		Core::File file(fileName, Core::FileFlags::DEFAULT_WRITE);
		WriteTestData(file, 0, 4);
		REQUIRE(file.Tell() == 4);
		WriteTestData(file, 4, 4);
		REQUIRE(file.Tell() == 8);
	}

	REQUIRE(Core::FileExists(fileName));
	{
		Core::File file(fileName, Core::FileFlags::READ);
		ReadTestData(file, 0, 4);
		REQUIRE(file.Tell() == 4);
		ReadTestData(file, 4, 4);
		REQUIRE(file.Tell() == 8);
	}
}

TEST_CASE("file-tests-seek")
{
	ScopedCleanup scopedCleanup;

	{
		Core::File file(fileName, Core::FileFlags::DEFAULT_WRITE);
		WriteTestData(file, 0, 8);
		REQUIRE(file.Seek(4));
		WriteTestData(file, 0, 4);
		WriteTestData(file, 0, 8);
		REQUIRE(file.Tell() == 16);
	}

	REQUIRE(Core::FileExists(fileName));
	{
		Core::File file(fileName, Core::FileFlags::READ);
		ReadTestData(file, 0, 4);
		ReadTestData(file, 0, 4);
		ReadTestData(file, 0, 8);
		REQUIRE(file.Seek(8));
		ReadTestData(file, 0, 8);
	}
}

TEST_CASE("file-tests-size")
{
	ScopedCleanup scopedCleanup;

	{
		Core::File file(fileName, Core::FileFlags::DEFAULT_WRITE);
		WriteTestData(file, 0, 8);
	}

	REQUIRE(Core::FileExists(fileName));

	{
		Core::File file(fileName, Core::FileFlags::READ);
		CHECK(file.Size() == 8);
	}

	{
		Core::File file(fileName, Core::FileFlags::DEFAULT_WRITE);
		WriteTestData(file, 0, 8);
		WriteTestData(file, 0, 8);
	}

	REQUIRE(Core::FileExists(fileName));

	{
		Core::File file(fileName, Core::FileFlags::READ);
		CHECK(file.Size() == 16);
	}
}

TEST_CASE("file-tests-mem")
{
	Core::Vector<u8> fileData;
	fileData.resize(64 * 1024);
	fileData.fill(0xff);

	const i64 halfSize = fileData.size() / 2;

	{
		Core::Vector<u8> writeData;
		writeData.resize(fileData.size());
		writeData.fill(0x00);

		Core::File file(fileData.data(), halfSize, Core::FileFlags::WRITE);
		REQUIRE(file.Write(writeData.data(), writeData.size()) == file.Size());
		CHECK(fileData[0] == 0x00);
		CHECK(fileData[(i32)halfSize - 1] == 0x00);
		CHECK(fileData[(i32)halfSize] == 0xff);
	}

	{
		Core::Vector<u8> readData;
		readData.resize(fileData.size());
		readData.fill(0x00);

		fileData.fill(0xff);
		Core::File file(fileData.data(), halfSize, Core::FileFlags::READ);
		REQUIRE(file.Read(readData.data(), readData.size()) == file.Size());
		CHECK(readData[0] == 0xff);
		CHECK(readData[(i32)halfSize - 1] == 0xff);
		CHECK(readData[(i32)halfSize] == 0x00);
	}
}


TEST_CASE("file-tests-mmap")
{
	Core::Vector<u8> fileData;
	fileData.resize(256 * 1024);

	Core::Random rng;
	for(i32 i = 0; i < fileData.size(); ++i)
	{
		fileData[i] = (u8)(rng.Generate() & 0xff);
	}

	{
		Core::File file("temp_mmap.dat", Core::FileFlags::DEFAULT_WRITE);
		REQUIRE(!!file);
		REQUIRE(file.Write(fileData.data(), fileData.size()) == fileData.size());
	}

	{
		Core::File file("temp_mmap.dat", Core::FileFlags::MMAP | Core::FileFlags::READ);
		auto mapped = Core::MappedFile(file, 0, file.Size());
		REQUIRE(!!mapped);

		const u8* readData = reinterpret_cast<const u8*>(mapped.GetAddress());
		CHECK(memcmp(readData, fileData.data(), fileData.size()) == 0);
	}
}


TEST_CASE("file-tests-create-dir")
{
	ScopedCleanup scopedCleanup;

	REQUIRE(Core::FileCreateDir(folder1));
	REQUIRE(Core::FileExists(folder1));
	REQUIRE(Core::FileRemoveDir(folder1));

	REQUIRE(Core::FileCreateDir(folder2));
	REQUIRE(Core::FileExists(folder1));
	REQUIRE(Core::FileExists(folder2));
	REQUIRE(Core::FileRemoveDir(folder2));
	REQUIRE(!Core::FileExists(folder2));
	REQUIRE(Core::FileRemoveDir(folder1));
	REQUIRE(!Core::FileExists(folder1));
}

TEST_CASE("file-tests-find-files")
{
	i32 foundFiles = Core::FileFindInPath(".", nullptr, nullptr, 0);
	REQUIRE(foundFiles > 0);

	Core::Vector<Core::FileInfo> fileInfos;
	fileInfos.resize(foundFiles);

	foundFiles = Core::FileFindInPath(".", nullptr, fileInfos.data(), fileInfos.size());
}

TEST_CASE("file-tests-split-path")
{
	auto TestPath = [&](const char* input, const char* path, const char* file, const char* ext) {
		Core::Array<char, Core::MAX_PATH_LENGTH> outPath = {0};
		Core::Array<char, Core::MAX_PATH_LENGTH> outFile = {0};
		Core::Array<char, Core::MAX_PATH_LENGTH> outExt = {0};
		memset(outPath.data(), 0, outPath.size());
		memset(outFile.data(), 0, outFile.size());
		memset(outExt.data(), 0, outExt.size());
		REQUIRE(Core::FileSplitPath(input, path ? outPath.data() : nullptr, outPath.size(),
		    file ? outFile.data() : nullptr, outFile.size(), ext ? outExt.data() : nullptr, outExt.size()));
		if(path)
			REQUIRE(strcmp(outPath.data(), path) == 0);
		if(file)
			REQUIRE(strcmp(outFile.data(), file) == 0);
		if(ext)
			REQUIRE(strcmp(outExt.data(), ext) == 0);
	};

	TestPath("myfile.txt", "", "myfile", "txt");
	TestPath("path/to/myfile.txt", "path/to", "myfile", "txt");
	TestPath("path/to/myfile", "path/to", "myfile", "");

	TestPath("path.to/myfile.txt", "path.to", "myfile", "txt");
	TestPath("path.to/myfile/.txt", "path.to/myfile", "", "txt");

	TestPath("myfile.txt.exe", "", "myfile.txt", "exe");
	TestPath("path/to/myfile.txt.exe", "path/to", "myfile.txt", "exe");

	TestPath("C:\\myfile.txt", "C:", "myfile", "txt");
	TestPath("C:\\path\\to\\myfile.txt", "C:\\path\\to", "myfile", "txt");
	TestPath("C:\\path\\to\\myfile", "C:\\path\\to", "myfile", "");

	TestPath("C:\\path.to\\myfile.txt", "C:\\path.to", "myfile", "txt");
	TestPath("C:\\path.to\\myfile\\.txt", "C:\\path.to\\myfile", "", "txt");

	TestPath("C:\\myfile.txt.exe", "C:", "myfile.txt", "exe");
	TestPath("C:\\path\\to\\myfile.txt.exe", "C:\\path\\to", "myfile.txt", "exe");


	TestPath("path/to/myfile.txt", nullptr, "myfile", "txt");
	TestPath("path/to/myfile.txt", "path/to", nullptr, "txt");
	TestPath("path/to/myfile.txt", "path/to", "myfile", nullptr);
}
