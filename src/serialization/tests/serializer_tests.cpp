#include "catch.hpp"

#include "core/file.h"
#include "core/float.h"
#include "core/vector.h"

#include "serialization/serializer.h"

#include <cmath>

TEST_CASE("serializer-tests-basic-write-read")
{
	Core::Vector<u8*> buffer;
	buffer.resize(1024 * 1024);

	Core::File outFile(buffer.data(), buffer.size(), Core::FileFlags::WRITE);
	{
		char testText[16] = "test";
		bool testBool = true;
		i32 testInt = 1337;
		f32 testFloat = Core::F32_PI;
		char testBinary[256];
		for(i32 i = 0; i < 256; ++i)
			testBinary[i] = (char)i;

		Serialization::Serializer serializer(outFile, Serialization::Flags::TEXT);
		if(auto object = serializer.Object("root_object"))
		{
			REQUIRE(serializer.Serialize("bool", testBool));
			REQUIRE(serializer.Serialize("int", testInt));
			REQUIRE(serializer.Serialize("float", testFloat));
			REQUIRE(serializer.SerializeString("text", testText, sizeof(testText)));
			REQUIRE(serializer.SerializeBinary("binary", testBinary, sizeof(testBinary)));
		}
	}

	Core::File inFile(buffer.data(), outFile.Tell(), Core::FileFlags::READ);
	{
		char testText[16];
		bool testBool = false;
		i32 testInt = 0;
		f32 testFloat = 0.0f;
		char testBinary[256];
		memset(testBinary, 0, sizeof(testBinary));

		Serialization::Serializer serializer(inFile, Serialization::Flags::TEXT);
		if(auto object = serializer.Object("root_object"))
		{
			REQUIRE(serializer.Serialize("bool", testBool));
			REQUIRE(serializer.Serialize("int", testInt));
			REQUIRE(serializer.Serialize("float", testFloat));
			REQUIRE(serializer.SerializeString("text", testText, sizeof(testText)));
			REQUIRE(serializer.SerializeBinary("binary", testBinary, sizeof(testBinary)));
		}

		REQUIRE(strcmp(testText, "test") == 0);
		REQUIRE(testBool);
		REQUIRE(testInt == 1337);
		REQUIRE(abs(testFloat - Core::F32_PI) < Core::F32_EPSILON);
		for(i32 i = 0; i < 256; ++i)
			REQUIRE(testBinary[i] == (char)i);
	}
}

TEST_CASE("serializer-tests-vec-write-read")
{
	Core::Vector<u8*> buffer;
	buffer.resize(1024 * 1024);

	Core::File outFile(buffer.data(), buffer.size(), Core::FileFlags::WRITE);
	{
		Serialization::Serializer serializer(outFile, Serialization::Flags::TEXT);

		Core::Vector<i32> testVec;
		for(i32 idx = 0; idx < 32; ++idx)
			testVec.push_back(idx);

		if(auto object = serializer.Object("root_object"))
		{
			REQUIRE(serializer.Serialize("vec", testVec));
		}
	}

	Core::File inFile(buffer.data(), outFile.Tell(), Core::FileFlags::READ);
	{
		Core::Vector<i32> testVec;

		Serialization::Serializer serializer(inFile, Serialization::Flags::TEXT);
		if(auto object = serializer.Object("root_object"))
		{
			REQUIRE(serializer.Serialize("vec", testVec));
			REQUIRE(testVec.size() == 32);
			for(i32 idx = 0; idx < 32; ++idx)
				REQUIRE(testVec[idx] == idx);
		}
	}
}
