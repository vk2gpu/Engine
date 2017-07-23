#pragma once

#include "serialization/dll.h"
#include "core/types.h"
#include "core/string.h"
#include "core/vector.h"

#include <type_traits>

namespace Core
{
	class File;
	class UUID;
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
		bool Serialize(const char* key, Core::UUID& value);
		bool Serialize(const char* key, Core::String& value);

		bool SerializeString(const char* key, char* str, i32 maxLength);
		bool SerializeBinary(const char* key, char* data, i32 size);

		/// Enum serialization.
		template<typename ENUM, typename = typename std::enable_if<std::is_enum<ENUM>::value>::type>
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

		/// Object serialization.	
		struct ScopedObject
		{
			ScopedObject(Serializer& serializer, const char* key)
			    : serializer_(serializer)
			    , key_(key)
			{
				valid_ = serializer_.BeginObject(key) == -1;
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

		template<typename TYPE, typename = typename std::enable_if<std::is_object<TYPE>::value>::type, typename std::enable_if<!std::is_enum<TYPE>::value>::type>
		bool Serialize(const char* key, TYPE& type)
		{
			if(auto object = Object(key))
				return type.Serialize(*this);
			return false;
		}

		/// Vector serialization.
		template<typename TYPE, typename ALLOCATOR>
		bool Serialize(const char* key, Core::Vector<TYPE, ALLOCATOR>& type)
		{
			if(IsReading())
			{
				i32 num = BeginObject(key, true);
				type.resize(num);
				for(i32 idx = 0; idx < num; ++idx)
				{
					Serialize(nullptr, type[idx]);
				}
				EndObject();
				return true;
			}
			else if(IsWriting())
			{
				if(-1 == BeginObject(key, true))
				{
					for(auto& val : type)
					{
						Serialize(nullptr, val);
					}
					EndObject();
				}
				return true;
			}
			return false;
		}

		bool IsReading() const;
		bool IsWriting() const;

	private:
		i32 BeginObject(const char* key, bool isArray = false);
		void EndObject();


		Serializer(const Serializer&) = delete;
		Serializer& operator=(const Serializer&) = delete;

		struct SerializerImpl* impl_ = nullptr;
	};
} // namespace Serialization
