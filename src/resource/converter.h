#pragma once

#include "resource/dll.h"
#include "resource/types.h"
#include "plugin/plugin.h"

namespace Resource
{
	/**
	 * Converter context interface.
	 * This is overriden engine side and passed to converters
	 * to provide conversion parameters, and receive back infomation
	 * from conversion, such as dependencies and outputs.
	 */
	class IConverterContext
	{
	public:
		virtual ~IConverterContext() {}

		/**
		 * Add dependency.
		 * Any files that the converter/input resource depends upon.
		 */
		virtual void AddDependency(const char* fileName) = 0;

		/**
		 * Add output.
		 * All files output by the converter.
		 */
		virtual void AddOutput(const char* fileName) = 0;

		/**
		 * Add error.
		 * During conversion, errors can occur. This will allow it to be tracked easily.
		 * @param errorFile File the error originates (i.e. a shader header file)
		 * @param errorLine Line on which error occurs, if appropriate.
		 * @param errorMsg Error message.
		 */
		virtual void AddError(const char* errorFile, int errorLine, const char* errorMsg) = 0;
	};


	/**
	 * Resource converter interface.
	 * This is responsible for converting individual resources into
	 * something usable by the engine.
	 */
	class IConverter
	{
	public:
		virtual ~IConverter() {}

		/**
		 * Does converter support file type?
		 * @param extension File extesnion (i.e. fbx, png, dds)
		 */
		virtual bool SupportsFileType(const char* extension) const = 0;

		/**
		 * Convert resource.
		 * @param context Converter context.
		 * @param sourceFile Source file to convert.
		 * @param destPath Destination path for resource.
		 * @return true if success.
		 */
		virtual bool Convert(IConverterContext& context, const char* sourceFile, const char* destPath) = 0;
	};

	/**
	 * Define converter plugin.
	 */
	struct ConverterPlugin : Plugin::Plugin
	{
		DECLARE_PLUGININFO("ConverterPlugin", 0);

		typedef IConverter* (*CreateConverterFn)();
		CreateConverterFn CreateConverter;

		typedef void (*DestroyConverterFn)(IConverter*);
		DestroyConverterFn DestroyConverter;
	};

} // namespace Resource