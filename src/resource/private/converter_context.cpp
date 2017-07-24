#include "resource/private/converter_context.h"
#include "core/misc.h"

#include <algorithm>
#include <cstdarg>

namespace Resource
{
	ConverterContext::ConverterContext(Core::IFilePathResolver* pathResolver)
	    : pathResolver_(pathResolver)
	{
	}

	ConverterContext::~ConverterContext() {}

	void ConverterContext::AddDependency(const char* fileName)
	{
		if(std::find_if(dependencies_.begin(), dependencies_.end(),
		       [fileName](const Core::String& str) { return str == fileName; }) == dependencies_.end())
			dependencies_.push_back(fileName);
	}

	void ConverterContext::AddOutput(const char* fileName) { outputs_.push_back(fileName); }

	void ConverterContext::AddError(const char* errorFile, int errorLine, const char* errorMsg, ...)
	{
		if(errorFile)
		{
			Core::Log("%s(%d): ", errorFile, errorLine);
		}

		va_list args;
		va_start(args, errorMsg);
		Core::Log(errorMsg, args);
		va_end(args);

		Core::Log("\n");
	}

	Core::IFilePathResolver* ConverterContext::GetPathResolver() { return pathResolver_; }

	bool ConverterContext::Convert(IConverter* converter, const char* sourceFile, const char* destPath)
	{
		// Setup metadata path.
		if(GetPathResolver()->ResolvePath(sourceFile, metaDataFileName_, sizeof(metaDataFileName_)))
		{
			strcat_s(metaDataFileName_, sizeof(metaDataFileName_), ".metadata");
		}

		dependencies_.clear();
		outputs_.clear();

		// Do conversion.
		return converter->Convert(*this, sourceFile, destPath);
	}

	void ConverterContext::SetMetaData(MetaDataCb callback, void* metaData)
	{
		metaDataSer_ = Serialization::Serializer();
		metaDataFile_ = Core::File();

		if(Core::FileExists(metaDataFileName_))
		{
			auto removed = Core::FileRemove(metaDataFileName_);
			DBG_ASSERT(removed);
		}

		metaDataFile_ = Core::File(metaDataFileName_, Core::FileFlags::CREATE | Core::FileFlags::WRITE);
		metaDataSer_ = Serialization::Serializer(metaDataFile_, Serialization::Flags::TEXT);

		if(callback)
		{
			callback(metaDataSer_, metaData);
		}

		if(auto object = metaDataSer_.Object("internal"))
		{
			metaDataSer_.Serialize("dependencies", dependencies_);
			metaDataSer_.Serialize("outputs", outputs_);
		}

		metaDataSer_ = Serialization::Serializer();
		metaDataFile_ = Core::File();
	}

	void ConverterContext::GetMetaData(MetaDataCb callback, void* metaData)
	{
		// If we don't have a metadata file for red, attempt to open for read.
		if(!Core::ContainsAnyFlags(metaDataFile_.GetFlags(), Core::FileFlags::READ) &&
		    Core::FileExists(metaDataFileName_))
		{
			metaDataFile_ = Core::File(metaDataFileName_, Core::FileFlags::READ);
			metaDataSer_ = Serialization::Serializer(metaDataFile_, Serialization::Flags::TEXT);

			if(callback)
			{
				callback(metaDataSer_, metaData);
			}
		}
	}


} // namespace Resource
