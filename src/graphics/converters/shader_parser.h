#pragma once

#include "core/types.h"
#include "core/map.h"
#include "core/set.h"
#include "core/string.h"

#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4505)
#include <stb_c_lexer.h>
#pragma warning(pop)


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
		};

		struct Node;
		struct NodeShaderFile;
		struct NodeModifier;
		struct NodeType;
		struct NodeTypeIdent;
		struct NodeStruct;
		struct NodeDeclaration;

		struct Node
		{
			Node(Nodes nodeType)
			    : nodeType_(nodeType)
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

			Core::String code_;
			Core::Vector<NodeDeclaration*> parameters_;
			Core::Vector<NodeDeclaration*> functions_;
			Core::Vector<NodeStruct*> structs_;
		};

		struct NodeStorageClass : Node
		{
			NodeStorageClass()
			    : Node(Nodes::STORAGE_CLASS)
			{
			}
			virtual ~NodeStorageClass() = default;
		};

		struct NodeModifier : Node
		{
			NodeModifier()
			    : Node(Nodes::MODIFIER)
			{
			}
			virtual ~NodeModifier() = default;
		};

		struct NodeType : Node
		{
			NodeType()
			    : Node(Nodes::TYPE)
			{
			}
			virtual ~NodeType() = default;

			i32 size_ = 0;
			Core::Vector<NodeDeclaration*> members_;
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
			NodeStruct()
			    : Node(Nodes::STRUCT)
			{
			}
			virtual ~NodeStruct() = default;

			NodeType* type_ = nullptr;
		};

		struct NodeDeclaration : Node
		{
			NodeDeclaration()
			    : Node(Nodes::DECLARATION)
			{
			}
			virtual ~NodeDeclaration() = default;

			Core::Vector<NodeStorageClass*> storageClasses_;
			NodeTypeIdent* type_ = nullptr;
			Core::String value_;
			Core::String semantic_;
			bool isFunction_ = false;
			Core::Vector<NodeDeclaration*> parameters_;
		};
	}

	enum class ErrorType : i32
	{
		FIRST_ERROR = 100,

		PARSE_ERROR,
		UNEXPECTED_EOF,
		UNEXPECTED_TOKEN,
		TYPE_REDEFINITION,
		TYPE_MISSING,
		FUNCTION_REDEFINITION,
		UNMATCHED_PARENTHESIS,
		UNMATCHED_BRACKET,
		RESERVED_KEYWORD,
	};

	class IShaderParserCallbacks
	{
	public:
		virtual ~IShaderParserCallbacks() = default;

		/**
		 * Called on error.
		 */
		virtual void OnError(
		    ErrorType errorType, const char* fileName, i32 lineNumber, i32 lineOffset, const char* str) = 0;
	};

	class ShaderParser
	{
	public:
		ShaderParser();
		~ShaderParser();

		void Parse(const char* shaderFileName, const char* shaderCode, IShaderParserCallbacks* callbacks = nullptr);

	private:
		AST::NodeShaderFile* ParseShaderFile(stb_lexer& lexCtx);
		AST::NodeStorageClass* ParseStorageClass(stb_lexer& lexCtx);
		AST::NodeModifier* ParseModifier(stb_lexer& lexCtx);
		AST::NodeType* ParseType(stb_lexer& lexCtx);
		AST::NodeTypeIdent* ParseTypeIdent(stb_lexer& lexCtx);
		AST::NodeStruct* ParseStruct(stb_lexer& lexCtx);
		AST::NodeDeclaration* ParseDeclaration(stb_lexer& lexCtx);

		bool NextToken(stb_lexer& lexCtx);
		AST::Token GetToken() const;

		void Error(stb_lexer& lexCtx, AST::Node*& node, ErrorType errorType, Core::StringView errorStr);

		template<typename NODE_TYPE>
		void Error(stb_lexer& lexCtx, NODE_TYPE*& node, ErrorType errorType, Core::StringView errorStr)
		{
			Error(lexCtx, reinterpret_cast<AST::Node*&>(node), errorType, errorStr);
		}

	private:
		AST::NodeType* AddType(const char* name, i32 size);
		AST::NodeType* FindType(const char* name);

		AST::NodeModifier* AddModifier(const char* name);
		AST::NodeModifier* FindModifier(const char* name);

		AST::NodeStorageClass* AddStorageClass(const char* name);
		AST::NodeStorageClass* FindStorageClass(const char* name);

		const char* fileName_ = nullptr;
		IShaderParserCallbacks* callbacks_ = nullptr;
		AST::Token token_;

		Core::Map<Core::String, AST::NodeStruct*> structNodes_;
		Core::Map<Core::String, AST::NodeType*> typeNodes_;
		Core::Map<Core::String, AST::NodeModifier*> modifierNodes_;
		Core::Map<Core::String, AST::NodeStorageClass*> storageClassNodes_;

		Core::Set<Core::String> reserved_;
	};
} // namespace Graphics
