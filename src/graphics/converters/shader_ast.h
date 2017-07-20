#pragma once

#include "core/types.h"
#include "core/enum.h"
#include "core/map.h"
#include "core/set.h"
#include "core/string.h"

namespace Graphics
{
	namespace AST
	{
		enum class TokenType : i32
		{
			INVALID = -1,

			IDENTIFIER,
			CHAR,
			INT,
			FLOAT,
			STRING,
		};

		enum class ValueType : i32
		{
			INVALID = -1,

			INT,
			FLOAT,
			STRING,
			ENUM,
			ARRAY,
			STRUCT,
			RAW_CODE
		};

		struct Token
		{
			TokenType type_ = TokenType::INVALID;
			Core::String value_;

			operator bool() const { return type_ != TokenType::INVALID; }
		};

		enum class Nodes : i32
		{
			INVALID = -1,

			// Whole shader file.
			SHADER_FILE = 0,
			// attribute: i.e. '[unroll]' '[maxiterations(8)]' '[numthreads(1,2,4)]'
			ATTRIBUTE,
			// storage class: i.e. 'static', 'groupshared'
			STORAGE_CLASS,
			// modifier: i.e. 'const', 'unorm'
			MODIFIER,
			// type: i.e. 'float4', 'RWTexture2D'
			TYPE,
			// type identifier: i.e. 'float4', 'RWTexture2D<float4>'.
			TYPE_IDENT,
			// struct declaration: i.e. "struct MyStruct { ... };"
			STRUCT,
			// parameter/function declaration: i.e. "float4 myThing : SEMANTIC = 0;" or "float4 func(int param) : SEMANTIC"
			DECLARATION,
			// base value: i.e. "1", "2"
			VALUE,
			// values: i.e. "{ <value>, <value>, <value> }"
			VALUES,
			// member value: i.e. "<member> = <value>"
			MEMBER_VALUE,
		};

		struct Node;
		struct NodeShaderFile;
		struct NodeAttribute;
		struct NodeStorageClass;
		struct NodeModifier;
		struct NodeType;
		struct NodeTypeIdent;
		struct NodeStruct;
		struct NodeDeclaration;
		struct NodeValue;
		struct NodeValues;
		struct NodeMemberValue;

		struct Node
		{
			Node(Nodes nodeType, const char* name = "")
			    : nodeType_(nodeType)
			    , name_(name)
			{
			}
			virtual ~Node() = default;

			Nodes nodeType_;
			Core::String name_;
		};

		struct NodeShaderFile : Node
		{
			NodeShaderFile()
			    : Node(Nodes::SHADER_FILE)
			{
			}
			virtual ~NodeShaderFile() = default;

			NodeDeclaration* FindVariable(const char* name);
			NodeDeclaration* FindFunction(const char* name);

			Core::String code_;
			Core::Vector<NodeDeclaration*> variables_;
			Core::Vector<NodeDeclaration*> functions_;
			Core::Vector<NodeStruct*> structs_;
		};

		struct NodeAttribute : Node
		{
			NodeAttribute(const char* name)
			    : Node(Nodes::ATTRIBUTE, name)
			{
			}
			virtual ~NodeAttribute() = default;

			Core::Vector<Core::String> parameters_;
		};

		struct NodeStorageClass : Node
		{
			NodeStorageClass(const char* name)
			    : Node(Nodes::STORAGE_CLASS, name)
			{
			}
			virtual ~NodeStorageClass() = default;
		};

		struct NodeModifier : Node
		{
			NodeModifier(const char* name)
			    : Node(Nodes::MODIFIER, name)
			{
			}
			virtual ~NodeModifier() = default;
		};

		struct NodeType : Node
		{
			using EnumValueFn = bool (*)(i32&, const char*);
			using EnumNameFn = const char* (*)(i32);

			NodeType(const char* name, i32 size = -1)
			    : Node(Nodes::TYPE, name)
			    , size_(size)
			{
			}

			template<typename ENUM_TYPE>
			NodeType(const char* name, ENUM_TYPE maxEnumValue)
			    : Node(Nodes::TYPE, name)
			    , size_(sizeof(i32))
			{
				enumValueFn_ = [](i32& val, const char* str) -> bool {
					ENUM_TYPE enumVal;
					if(Core::EnumFromString(enumVal, str))
					{
						val = (i32)enumVal;
						return true;
					}
					return false;
				};

				enumNameFn_ = [](i32 val) -> const char* { return Core::EnumToString((ENUM_TYPE)val); };
				maxEnumValue_ = (i32)maxEnumValue;
			}
			virtual ~NodeType() = default;

			bool IsEnum() { return !!enumValueFn_; }
			bool IsPOD() { return size_ >= 0; }

			NodeDeclaration* FindMember(const char* name) const;
			const char* FindEnumName(i32 val) const;
			bool HasEnumValue(const char* name) const;

			i32 size_ = 0;
			EnumValueFn enumValueFn_ = nullptr;
			EnumNameFn enumNameFn_ = nullptr;
			i32 maxEnumValue_ = 0;
			Core::Vector<NodeDeclaration*> members_;
			NodeStruct* struct_ = nullptr;
		};

		struct NodeTypeIdent : Node
		{
			NodeTypeIdent()
			    : Node(Nodes::TYPE_IDENT)
			{
			}
			virtual ~NodeTypeIdent() = default;

			NodeType* baseType_ = nullptr;
			NodeType* templateType_ = nullptr;

			Core::Vector<NodeModifier*> baseModifiers_;
			Core::Vector<NodeModifier*> templateModifiers_;
		};

		struct NodeStruct : Node
		{
			NodeStruct(const char* name)
			    : Node(Nodes::STRUCT, name)
			{
			}
			virtual ~NodeStruct() = default;

			NodeAttribute* FindAttribute(const char* name) const;

			Core::Vector<AST::NodeAttribute*> attributes_;
			NodeType* type_ = nullptr;
		};

		struct NodeDeclaration : Node
		{
			NodeDeclaration(const char* name)
			    : Node(Nodes::DECLARATION, name)
			{
			}
			virtual ~NodeDeclaration() = default;

			NodeAttribute* FindAttribute(const char* name) const;

			Core::Vector<AST::NodeAttribute*> attributes_;
			Core::Vector<NodeStorageClass*> storageClasses_;
			NodeTypeIdent* type_ = nullptr;
			NodeValue* value_ = nullptr;
			Core::String semantic_;
			bool isFunction_ = false;
			Core::Vector<NodeDeclaration*> parameters_;
		};

		struct NodeValue : Node
		{
			NodeValue(Nodes nodeType = Nodes::VALUE)
			    : Node(nodeType)
			{
			}
			virtual ~NodeValue() = default;

			ValueType type_ = ValueType::INVALID;
			Core::String data_;
		};

		struct NodeValues : NodeValue
		{
			NodeValues()
			    : NodeValue(Nodes::VALUES)
			{
				type_ = ValueType::ARRAY;
			}
			virtual ~NodeValues() = default;

			Core::Vector<NodeValue*> values_;
		};

		struct NodeMemberValue : NodeValue
		{
			NodeMemberValue()
			    : NodeValue(Nodes::VALUES)
			{
				type_ = ValueType::STRUCT;
			}
			virtual ~NodeMemberValue() = default;

			Core::String member_;
			NodeValue* value_;
		};
	} // namespace AST
} // namespace Graphics
