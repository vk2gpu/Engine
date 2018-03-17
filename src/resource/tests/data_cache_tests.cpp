#include "catch.hpp"

#include "core/allocator.h"
#include "core/debug.h"
#include "core/file.h"
#include "core/misc.h"
#include "core/uuid.h"
#include "core/vector.h"
#include "job/manager.h"
#include "resource/data_cache.h"

#include <Windows.h>
#include <WinInet.h>
#pragma comment(lib, "wininet.lib")

#include "curl/curl.h"

#include <cstdarg>

TEST_CASE("data-cache-tests-basic")
{
	Resource::DataCache dataCache;

	float testSrcData[] = { 0.0f, 1.0f, 2.0f, 3.0f };
	u8 testDataOut[] = { 0, 1, 2, 3 };

	Resource::DataHash hash = Core::HashSHA1(testSrcData, sizeof(testSrcData));

	dataCache.Write(hash, testDataOut, sizeof(testDataOut));
	
	u8 testDataIn[] = { 0xff, 0xff, 0xff, 0xff };
	dataCache.Read(hash, testDataIn, sizeof(testDataIn));

	bool readSuccessful = memcmp(testDataIn, testDataOut, sizeof(testDataOut)) == 0;
	REQUIRE(readSuccessful);
}


TEST_CASE("data-cache-tests-http")
{
	const char* url = "https://downloads.sourceforge.net/project/ispcmirror/v1.9.1/ispc-v1.9.1-windows-vs2015.zip";
	HINTERNET hInternet = ::InternetOpenA("httpRequest", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);

	if(::InternetGoOnlineA(url, nullptr, 0))
	{
		HINTERNET hFileHandle = ::InternetOpenUrlA(hInternet, url, nullptr, 0, 0, 0);

		DWORD requestStatusSize = sizeof(requestStatusSize);
		DWORD requestStatus = 0;
		::HttpQueryInfoA(hFileHandle, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &requestStatus, &requestStatusSize, nullptr);

		if(requestStatus == HTTP_STATUS_OK)
		{
			DWORD requestLengthSize = sizeof(requestStatusSize);
			DWORD requestLength = 0;
			::HttpQueryInfoA(hFileHandle, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &requestLength, &requestLengthSize, nullptr);
		
			DWORD chunkSize = 32 * 1024;
			Core::Vector<u8> internetData(requestLength);
			DWORD totalRead = 0;
			DWORD thisRead = 0;
			while(::InternetReadFile(hFileHandle, &internetData[totalRead], chunkSize, &thisRead) && thisRead > 0 && totalRead < requestLength)
			{
				totalRead += thisRead;
				if((i32)totalRead > (internetData.size() / 2))
				{
					internetData.resize(internetData.size() * 2);
				}
				Core::Log("Total downloaded... %ukb (%.1f%%)\n", (totalRead / 1024), ((f32)totalRead / (f32)requestLength) * 100.0f);
			}
		}

		::InternetCloseHandle(hFileHandle);
	}
}


TEST_CASE("data-cache-tests-http-curl")
{
	CURL *curl;
 
	curl_global_init(CURL_GLOBAL_DEFAULT);
 
	curl = curl_easy_init();
	if(curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, "https://downloads.sourceforge.net/project/ispcmirror/v1.9.1/ispc-v1.9.1-windows-vs2015.zip");
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);

		struct Context
		{
			bool headerBegan = true;
			CURL* curl = nullptr;
			u8* data = nullptr;
			size_t maxSize = 0;
			size_t chunkSize = 32 * 1024;
			size_t totalRead = 0;
			size_t thisRead = 0;

			~Context()
			{
				if(data)
					Core::VirtualAllocator().Deallocate(data);
			}
		};

		Context ctx;
		ctx.curl = curl;

		using writeFuncSig = size_t(char *ptr, size_t size, size_t nmemb, void *userdata);

		writeFuncSig* headerFunc = [](char* buffer, size_t size, size_t count, void* inCtx) -> size_t
		{
			Context& ctx = *(Context*)inCtx;
			const size_t writeSize = size * count;

			Core::Log("%s", buffer);

			// End of header?
			if(buffer[0] == '\r' || buffer[0] == '\n')
			{
				if(!ctx.data)
				{
					size_t responseCode = 0;
					curl_easy_getinfo(ctx.curl, CURLINFO_RESPONSE_CODE, &responseCode);
					if(responseCode == 200)
					{
						curl_off_t contentLength = 0;
						if(CURLE_OK == curl_easy_getinfo(ctx.curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &contentLength) && contentLength > 0)
						{
							ctx.maxSize = contentLength;
							ctx.data = (u8*)Core::VirtualAllocator().Allocate(contentLength, 4096);
						}
					}
				}
			}
			return writeSize;
		};
		
		writeFuncSig* writeFunc = [](char* buffer, size_t size, size_t count, void* inCtx) -> size_t
		{
			Context& ctx = *(Context*)inCtx;

			DBG_ASSERT(ctx.data);
	
			double speed = 0.0;
			curl_easy_getinfo(ctx.curl, CURLINFO_SPEED_DOWNLOAD, &speed);

			const size_t writeSize = size * count;
			if((ctx.totalRead + writeSize) <= ctx.maxSize)
			{
				memcpy(&ctx.data[ctx.totalRead], buffer, writeSize);
			}

			ctx.totalRead += writeSize;
			Core::Log("Total downloaded... %ukb (%.1f%%, %.2f kb/s)\n", (ctx.totalRead / 1024), ((f32)ctx.totalRead / (f32)ctx.maxSize) * 100.0f, speed / 1024.0f);

			return writeSize;
		};

		struct curl_slist *chunk = NULL;
		chunk = curl_slist_append(chunk, "If-None-Match: \"577ff2d4-1a24e9f\"");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

		//curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerFunc);
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, &ctx);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunc);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);

		CURLcode res = curl_easy_perform(curl);

		/* Check for errors */ 
		if(res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));

		/* always cleanup */ 
		curl_easy_cleanup(curl);
	}
 
	curl_global_cleanup();
 
}
