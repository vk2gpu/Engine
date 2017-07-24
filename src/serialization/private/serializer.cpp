#include "serialization/serializer.h"
#include "core/array.h"
#include "core/debug.h"
#include "core/file.h"
#include "core/misc.h"
#include "core/uuid.h"
#include "core/vector.h"

#include <json/json.h>

/*
 * libb64 (modified to support specifying output length)
 * Author: Chris Venter	chris.venter@gmail.com	http://rocketpod.blogspot.com
 * License:
   This work is released under into the Public Domain.
   It basically boils down to this: I put this work in the public domain, and you
   can take it and do whatever you want with it.

   An example of this "license" is the Creative Commons Public Domain License, a
   copy of which can be found in the LICENSE file, and also online at
   http://creativecommons.org/licenses/publicdomain/
 */
#pragma warning(push)
#pragma warning(disable : 4244)
namespace
{
	typedef enum { step_a, step_b, step_c, step_d } base64_decodestep;

	typedef struct
	{
		base64_decodestep step;
		char plainchar;
	} base64_decodestate;

	typedef enum { step_A, step_B, step_C } base64_encodestep;

	typedef struct
	{
		base64_encodestep step;
		char result;
		int stepcount;
	} base64_encodestate;

	int base64_decode_value(char value_in)
	{
		static const char decoding[] = {62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -2, -1,
		    -1, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1,
		    -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
		    48, 49, 50, 51};
		static const char decoding_size = sizeof(decoding);
		value_in -= 43;
		if(value_in < 0 || value_in > decoding_size)
			return -1;
		return decoding[(int)value_in];
	}

	void base64_init_decodestate(base64_decodestate* state_in)
	{
		state_in->step = step_a;
		state_in->plainchar = 0;
	}

	int base64_decode_block(const char* code_in, const int length_in, const int length_out, char* plaintext_out,
	    base64_decodestate* state_in)
	{
		const char* codechar = code_in;
		char* plainchar = plaintext_out;
		char* plainchar_end = plaintext_out + length_out;
		char fragment;

		*plainchar = state_in->plainchar;

		switch(state_in->step)
		{
			while(1)
			{
			case step_a:
				do
				{
					if(codechar == code_in + length_in)
					{
						state_in->step = step_a;
						state_in->plainchar = *plainchar;
						return plainchar - plaintext_out;
					}
					fragment = (char)base64_decode_value(*codechar++);
				} while(fragment < 0);
				*plainchar = (fragment & 0x03f) << 2;
			case step_b:
				do
				{
					if(codechar == code_in + length_in)
					{
						state_in->step = step_b;
						state_in->plainchar = *plainchar;
						return plainchar - plaintext_out;
					}
					fragment = (char)base64_decode_value(*codechar++);
				} while(fragment < 0);
				*plainchar++ |= (fragment & 0x030) >> 4;
				if(plainchar == plainchar_end)
					return plainchar - plaintext_out;
				*plainchar = (fragment & 0x00f) << 4;
			case step_c:
				do
				{
					if(codechar == code_in + length_in)
					{
						state_in->step = step_c;
						state_in->plainchar = *plainchar;
						return plainchar - plaintext_out;
					}
					fragment = (char)base64_decode_value(*codechar++);
				} while(fragment < 0);
				*plainchar++ |= (fragment & 0x03c) >> 2;
				if(plainchar == plainchar_end)
					return plainchar - plaintext_out;
				*plainchar = (fragment & 0x003) << 6;
			case step_d:
				do
				{
					if(codechar == code_in + length_in)
					{
						state_in->step = step_d;
						state_in->plainchar = *plainchar;
						return plainchar - plaintext_out;
					}
					fragment = (char)base64_decode_value(*codechar++);
				} while(fragment < 0);
				*plainchar++ |= (fragment & 0x03f);
				if(plainchar == plainchar_end)
					return plainchar - plaintext_out;
			}
		}
		/* control should not reach here */
		return plainchar - plaintext_out;
	}

	const int CHARS_PER_LINE = 72;

	void base64_init_encodestate(base64_encodestate* state_in)
	{
		state_in->step = step_A;
		state_in->result = 0;
		state_in->stepcount = 0;
	}

	char base64_encode_value(char value_in)
	{
		static const char* encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		if(value_in > 63)
			return '=';
		return encoding[(int)value_in];
	}

	int base64_encode_block(
	    const char* plaintext_in, int length_in, char* code_out, int newLineEnabled, base64_encodestate* state_in)
	{
		const char* plainchar = plaintext_in;
		const char* const plaintextend = plaintext_in + length_in;
		char* codechar = code_out;
		char result;
		char fragment;

		result = state_in->result;

		switch(state_in->step)
		{
			while(1)
			{
			case step_A:
				if(plainchar == plaintextend)
				{
					state_in->result = result;
					state_in->step = step_A;
					return codechar - code_out;
				}
				fragment = *plainchar++;
				result = (fragment & 0x0fc) >> 2;
				*codechar++ = base64_encode_value(result);
				result = (fragment & 0x003) << 4;
			case step_B:
				if(plainchar == plaintextend)
				{
					state_in->result = result;
					state_in->step = step_B;
					return codechar - code_out;
				}
				fragment = *plainchar++;
				result |= (fragment & 0x0f0) >> 4;
				*codechar++ = base64_encode_value(result);
				result = (fragment & 0x00f) << 2;
			case step_C:
				if(plainchar == plaintextend)
				{
					state_in->result = result;
					state_in->step = step_C;
					return codechar - code_out;
				}
				fragment = *plainchar++;
				result |= (fragment & 0x0c0) >> 6;
				*codechar++ = base64_encode_value(result);
				result = (fragment & 0x03f) >> 0;
				*codechar++ = base64_encode_value(result);

				++(state_in->stepcount);

				if(newLineEnabled)
				{
					if(state_in->stepcount == CHARS_PER_LINE / 4)
					{
						*codechar++ = '\n';
						state_in->stepcount = 0;
					}
				}
			}
		}
		/* control should not reach here */
		return codechar - code_out;
	}

	int base64_encode_blockend(char* code_out, int newLineEnabled, base64_encodestate* state_in)
	{
		char* codechar = code_out;

		switch(state_in->step)
		{
		case step_B:
			*codechar++ = base64_encode_value(state_in->result);
			*codechar++ = '=';
			*codechar++ = '=';
			break;
		case step_C:
			*codechar++ = base64_encode_value(state_in->result);
			*codechar++ = '=';
			break;
		case step_A:
			break;
		}
		if(newLineEnabled)
		{
			*codechar++ = '\n';
		}

		return codechar - code_out;
	}
}
#pragma warning(pop)


namespace Serialization
{
	struct SerializerImpl
	{
		virtual ~SerializerImpl() {}
		virtual bool Serialize(const char* key, bool& value) = 0;
		virtual bool Serialize(const char* key, i32& value) = 0;
		virtual bool Serialize(const char* key, f32& value) = 0;
		virtual bool SerializeString(const char* key, char* str, i32 maxLength) = 0;
		virtual bool SerializeBinary(const char* key, char* data, i32 size) = 0;
		virtual i32 BeginObject(const char* key, bool isArray) = 0;
		virtual void EndObject() = 0;
		virtual bool IsReading() const = 0;
		virtual bool IsWriting() const = 0;
	};

	struct SerializerImplWriteJson : SerializerImpl
	{
		Core::File& outFile_;
		Json::Value rootValue_;
		Core::Vector<Json::Value*> objectStack_;

		SerializerImplWriteJson(Core::File& outFile)
		    : outFile_(outFile)
		{
			objectStack_.push_back(&rootValue_);
		}

		~SerializerImplWriteJson()
		{
			DBG_ASSERT(objectStack_.size() == 1);
			Json::StyledWriter writer;
			auto outStr = writer.write(rootValue_);
			outFile_.Write(outStr.data(), outStr.size());
		}

		Json::Value& GetObject(i32 idx = -1)
		{
			if(idx < 0)
				return *objectStack_.back();
			else
				return objectStack_.back()[idx];
		}

		bool Serialize(const char* key, bool& value) override
		{
			auto& object = GetObject();
			if(key)
				object[key] = value;
			else
				object.append(value);
			return true;
		}

		bool Serialize(const char* key, i32& value) override
		{
			auto& object = GetObject();
			if(key)
				object[key] = value;
			else
				object.append(value);
			return true;
		}

		bool Serialize(const char* key, f32& value) override
		{
			auto& object = GetObject();
			if(key)
				object[key] = value;
			else
				object.append(value);
			return true;
		}

		bool SerializeString(const char* key, char* str, i32 maxLength) override
		{
			auto& object = GetObject();
			if(key)
				object[key] = str;
			else
				object.append(str);
			return true;
		}

		bool SerializeBinary(const char* key, char* data, i32 size) override
		{
			i32 bytesRequired = ((size * 4) / 3) + 4;
			DBG_ASSERT(bytesRequired >= 0);
			Core::Vector<char> outString;
			outString.resize(bytesRequired + 1, 0);
			base64_encodestate encodeState;
			base64_init_encodestate(&encodeState);
			auto outBytes = base64_encode_block(data, size, outString.data(), 0, &encodeState);
			base64_encode_blockend(&outString[outBytes], 0, &encodeState);

			auto& object = GetObject();
			if(key)
				object[key] = outString.data();
			else
				object.append(outString.data());
			return true;
		}

		i32 BeginObject(const char* key, bool isArray) override
		{
			auto& object = GetObject();
			if(isArray)
			{
				auto& newObject = object[key] = Json::Value(Json::arrayValue);
				objectStack_.push_back(&newObject);
			}
			else
			{
				auto& newObject = object[key] = Json::Value(Json::objectValue);
				objectStack_.push_back(&newObject);
			}
			return -1;
		}

		void EndObject() override { objectStack_.pop_back(); }

		bool IsReading() const override { return false; }
		bool IsWriting() const override { return true; }
	};

	struct SerializerImplReadJson : SerializerImpl
	{
		Core::File& inFile_;
		Json::Value rootValue_;
		Core::Vector<Json::Value*> objectStack_;
		Core::Vector<i32> vectorStack_;

		SerializerImplReadJson(Core::File& inFile)
		    : inFile_(inFile)
		{
			Json::Reader reader;
			Core::Vector<char> inBuffer;
			inBuffer.resize((i32)inFile.Size());
			inFile.Read(inBuffer.data(), inBuffer.size());
			reader.parse(inBuffer.data(), inBuffer.data() + inBuffer.size(), rootValue_, false);

			objectStack_.push_back(&rootValue_);
		}

		~SerializerImplReadJson() { DBG_ASSERT(objectStack_.size() == 1); }

		Json::Value& GetObject() { return *objectStack_.back(); }

		Json::Value& GetObjectWithKey(const char* key)
		{
			if(key)
				return GetObject()[key];
			else
			{
				i32 idx = vectorStack_.back()++;
				return GetObject()[idx];
			}
		}

		bool Serialize(const char* key, bool& value) override
		{
			auto& object = GetObjectWithKey(key);
			if(object.isBool())
			{
				value = object.asBool();
				return true;
			}
			return false;
		}

		bool Serialize(const char* key, i32& value) override
		{
			auto& object = GetObjectWithKey(key);
			if(object.isInt())
			{
				value = object.asInt();
				return true;
			}
			return false;
		}

		bool Serialize(const char* key, f32& value) override
		{
			auto& object = GetObjectWithKey(key);
			if(object.isDouble())
			{
				value = object.asFloat();
				return true;
			}
			return false;
		}

		bool SerializeString(const char* key, char* str, i32 maxLength) override
		{
			auto& object = GetObjectWithKey(key);
			if(object.isString())
			{
				strcpy_s(str, maxLength, object.asCString());
				return true;
			}
			return false;
		}

		bool SerializeBinary(const char* key, char* data, i32 size) override
		{
			auto& object = GetObjectWithKey(key);
			if(object.isString())
			{
				i32 strLen = (i32)strlen(object.asCString());
				memset(data, 0, size);
				base64_decodestate decodeState;
				base64_init_decodestate(&decodeState);
				base64_decode_block(object.asCString(), strLen, size, data, &decodeState);
				return true;
			}
			return false;
		}

		i32 BeginObject(const char* key, bool isArray) override
		{
			auto& object = GetObjectWithKey(key);
			if(object.isObject())
			{
				objectStack_.push_back(&object);
				return -1;
			}
			else if(object.isArray())
			{
				objectStack_.push_back(&object);
				vectorStack_.push_back(0);
				return object.size();
			}
			return 0;
		}

		void EndObject() override
		{
			if(objectStack_.back()->isArray())
				vectorStack_.pop_back();
			objectStack_.pop_back();
		}

		bool IsReading() const override { return true; }
		bool IsWriting() const override { return false; }
	};

	Serializer::Serializer(Core::File& file, Flags flags)
	    : impl_()
	{
		if(Core::ContainsAllFlags(flags, Flags::TEXT))
		{
			if(Core::ContainsAnyFlags(file.GetFlags(), Core::FileFlags::WRITE))
				impl_ = new SerializerImplWriteJson(file);
			if(Core::ContainsAnyFlags(file.GetFlags(), Core::FileFlags::READ))
				impl_ = new SerializerImplReadJson(file);
		}
	}

	Serializer::~Serializer() { delete impl_; }

	Serializer::Serializer(Serializer&& other)
	{
		using std::swap;
		swap(impl_, other.impl_);
	}

	Serializer& Serializer::operator=(Serializer&& other)
	{
		using std::swap;
		swap(impl_, other.impl_);
		return *this;
	}


	bool Serializer::Serialize(const char* key, bool& value) { return impl_->Serialize(key, value); }

	bool Serializer::Serialize(const char* key, i32& value) { return impl_->Serialize(key, value); }

	bool Serializer::Serialize(const char* key, f32& value) { return impl_->Serialize(key, value); }

	bool Serializer::Serialize(const char* key, Core::UUID& value)
	{
		if(IsReading())
		{
			Core::Array<char, 37> str;
			if(SerializeString(key, str.data(), str.size()))
			{
				return value.FromString(str.data());
			}
			return false;
		}
		else if(IsWriting())
		{
			Core::Array<char, 37> str;
			value.AsString(str.data());
			return SerializeString(key, str.data(), str.size());
		}
		return false;
	}

	bool Serializer::Serialize(const char* key, Core::String& value)
	{
		if(IsReading())
		{
			value.resize(4096);
		}
		auto retVal = impl_->SerializeString(key, value.data(), value.size());
		value.shrink_to_fit();
		return retVal;
	}

	bool Serializer::SerializeString(const char* key, char* str, i32 maxLength)
	{
		return impl_->SerializeString(key, str, maxLength);
	}

	bool Serializer::SerializeBinary(const char* key, char* data, i32 size)
	{
		return impl_->SerializeBinary(key, data, size);
	}

	Serializer::ScopedObject Serializer::Object(const char* key) { return Serializer::ScopedObject(*this, key); }

	i32 Serializer::BeginObject(const char* key, bool isArray) { return impl_->BeginObject(key, isArray); }

	void Serializer::EndObject() { impl_->EndObject(); }

	bool Serializer::IsReading() const { return impl_->IsReading(); }

	bool Serializer::IsWriting() const { return impl_->IsWriting(); }


} // namespace Serialization
