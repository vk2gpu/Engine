#include "resource/converter.h"
#include "core/debug.h"
#include "core/file.h"
#include "serialization/serializer.h"

namespace Resource
{
	/// Converter context to use during resource conversion.
	class ConverterContext : public Resource::IConverterContext
	{
	public:
		ConverterContext(Core::IFilePathResolver* pathResolver);
		virtual ~ConverterContext();

		void AddDependency(const char* fileName) override;
		void AddOutput(const char* fileName) override;
		void AddError(const char* errorFile, int errorLine, const char* errorMsg, ...) override;
		Core::IFilePathResolver* GetPathResolver() override;
		bool Convert(IConverter* converter, const char* sourceFile, const char* destPath);
		void SetMetaData(MetaDataCb callback, void* metaData) override;
		void GetMetaData(MetaDataCb callback, void* metaData) override;

	private:
		Core::IFilePathResolver* pathResolver_ = nullptr;
		char metaDataFileName_[Core::MAX_PATH_LENGTH] = {0};
		Core::File metaDataFile_;
		Serialization::Serializer metaDataSer_;
		Core::Vector<Core::String> dependencies_;
		Core::Vector<Core::String> outputs_;
	};


} // namespace Resource
