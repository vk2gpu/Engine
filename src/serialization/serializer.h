#pragma once

#include "serialization/dll.h"
#include "core/types.h"

namespace Core
{
	class File;
} // namespace Core

namespace Serialization
{
	enum class Flags
	{
		/// Set when text output is desired.
		TEXT = 0x1,
		/// Set when binary output is desired.
		BINARY = 0x2
	};

	/**
	 * General purpose serializer.
	 * Handles both reading and writing via the same interface.
	 */
	class SERIALIZATION_DLL Serializer
	{
	public:
		Serializer() = default;
		Serializer(Core::File& file, Flags flags);
		~Serializer();

		Serializer(Serializer&&);
		Serializer& operator=(Serializer&&);

		bool Serialize(const char* key, bool& value);
		bool Serialize(const char* key, i32& value);
		bool Serialize(const char* key, f32& value);

		bool SerializeString(const char* key, char* str, i32 maxLength);
		bool SerializeBinary(const char* key, char* data, i32 size);

		template<typename ENUM>
		bool Serialize(const char* key, ENUM& value)
		{
			if(IsReading())
			{
				char valueStr[32] = {0};
				SerializeString(key, &valueStr[0], sizeof(valueStr));
				if(valueStr != nullptr)
				{
					return Core::EnumFromString(value, valueStr);
				}
			}
			else if(IsWriting())
			{
				const char* valueStr = Core::EnumToString(value);
				SerializeString(key, (char*)valueStr, 0);
			}
			return true;
		}

		struct ScopedObject
		{
			ScopedObject(Serializer& serializer, const char* key)
			    : serializer_(serializer)
			    , key_(key)
			{
				valid_ = serializer_.BeginObject(key);
			}

			~ScopedObject()
			{
				if(valid_)
					serializer_.EndObject();
			}

			operator bool() const { return valid_; }

			Serializer& serializer_;
			const char* key_ = nullptr;
			bool valid_ = false;
		};

		ScopedObject Object(const char* key);

		template<typename TYPE>
		bool SerializeObject(const char* key, TYPE& type)
		{
			if(auto object = Object(key))
				return type.Serialize(*this);
			return false;
		}

		bool IsReading() const;
		bool IsWriting() const;

	private:
		bool BeginObject(const char* key);
		void EndObject();

		Serializer(const Serializer&) = delete;
		Serializer& operator=(const Serializer&) = delete;

		struct SerializerImpl* impl_ = nullptr;
	};
} // namespace Serialization
