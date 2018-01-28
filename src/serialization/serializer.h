#pragma once

#include "serialization/dll.h"
#include "core/types.h"
#include "core/map.h"
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
	 * Helper macros.
	 */
#define SERIALIZE_MEMBER(_name)                                                                                        \
	if(!serializer.Serialize(#_name, _name))                                                                           \
		return false;
#define SERIALIZE_STRING_MEMBER(_name)                                                                                 \
	if(!serializer.SerializeString(#_name, _name, sizeof(_name)))                                                      \
		return false;
#define SERIALIZE_BINARY_MEMBER(_name)                                                                                 \
	if(!serializer.SerializeBinary(#_name, (char*)&_name, sizeof(_name)))                                              \
		return false;

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
		bool Serialize(const char* key, i16& value);
		bool Serialize(const char* key, u16& value);
		bool Serialize(const char* key, i32& value);
		bool Serialize(const char* key, u32& value);
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
			ScopedObject(Serializer& serializer, const char* key, bool isArray)
			    : serializer_(serializer)
			    , key_(key)
			{
				valid_ = serializer_.BeginObject(key, isArray) != -1;
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

		ScopedObject Object(const char* key, bool isArray = false);

		template<typename TYPE, typename = typename std::enable_if<std::is_object<TYPE>::value>::type,
		    typename = typename std::enable_if<!std::is_enum<TYPE>::value>::type>
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
				if(num >= 0)
				{
					type.resize(num);
					for(i32 idx = 0; idx < num; ++idx)
					{
						Serialize(nullptr, type[idx]);
					}
					EndObject();
				}
				return true;
			}
			else if(IsWriting())
			{
				if(0 == BeginObject(key, true))
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

		/// Map serialization.
		template<typename TYPE, typename ALLOCATOR>
		bool Serialize(const char* key, Core::Map<Core::String, TYPE, ALLOCATOR>& type)
		{
			if(IsReading())
			{
				i32 num = BeginObject(key, false);
				if(num >= 0)
				{
					for(i32 idx = 0; idx < num; ++idx)
					{
						Core::String objKey = GetObjectKey(idx);
						TYPE value;
						Serialize(objKey.c_str(), value);
						type.insert(objKey, std::move(value));
					}
					EndObject();
				}
				return true;
			}
			else if(IsWriting())
			{
				if(0 == BeginObject(key, false))
				{
					for(auto& val : type)
					{
						Serialize(val.first.c_str(), val.second);
					}
					EndObject();
				}
				return true;
			}
			return false;
		}

		bool IsReading() const;
		bool IsWriting() const;

		explicit operator bool() const { return impl_ != nullptr; }

	private:
		i32 BeginObject(const char* key, bool isArray = false);
		void EndObject();
		Core::String GetObjectKey(i32 idx);

		Serializer(const Serializer&) = delete;
		Serializer& operator=(const Serializer&) = delete;

		struct SerializerImpl* impl_ = nullptr;
	};
} // namespace Serialization
