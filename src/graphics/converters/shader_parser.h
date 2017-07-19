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

			Core::String code_;
			Core::Vector<NodeDeclaration*> parameters_;
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
			NodeType(const char* name, i32 size = -1)
			    : Node(Nodes::TYPE, name)
			    , size_(size)
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
			NodeStruct(const char* name)
			    : Node(Nodes::STRUCT, name)
			{
			}
			virtual ~NodeStruct() = default;

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

			Core::Vector<AST::NodeAttribute*> attributes_;
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
		virtual void OnError(ErrorType errorType, const char* fileName, i32 lineNumber, i32 lineOffset,
		    const char* line, const char* str) = 0;
	};

	class ShaderParser
	{
	public:
		ShaderParser();
		~ShaderParser();

		void Parse(const char* shaderFileName, const char* shaderCode, IShaderParserCallbacks* callbacks = nullptr);

	private:
		AST::NodeShaderFile* ParseShaderFile(stb_lexer& lexCtx);
		AST::NodeAttribute* ParseAttribute(stb_lexer& lexCtx);
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
		template<typename TYPE>
		void AddReserved(TYPE& nodes)
		{
			for(auto& node : nodes)
				reserved_.insert(node.name_);
		}

		template<typename TYPE>
		TYPE* AddToMap(TYPE* node)
		{
			return node;
		}
		AST::NodeStorageClass* AddToMap(AST::NodeStorageClass* node) { return storageClassNodes_.Add(node); }
		AST::NodeModifier* AddToMap(AST::NodeModifier* node) { return modifierNodes_.Add(node); }
		AST::NodeType* AddToMap(AST::NodeType* node) { return typeNodes_.Add(node); }
		AST::NodeStruct* AddToStorageMap(AST::NodeStruct* node) { return structNodes_.Add(node); }

		template<typename TYPE>
		void AddNodes(TYPE& nodeArray)
		{
			for(auto& node : nodeArray)
				AddToMap(&node);
		}

		template<typename TYPE, typename... ARGS>
		TYPE* AddNode(ARGS&&... args)
		{
			TYPE* node = new TYPE(std::forward<ARGS>(args)...);
			allocatedNodes_.push_back(node);
			return AddToMap(node);
		}

		const char* fileName_ = nullptr;
		IShaderParserCallbacks* callbacks_ = nullptr;
		AST::Token token_;

		template<typename TYPE>
		struct NodeMap
		{
			TYPE* Find(const char* name)
			{
				auto it = storage_.find(name);
				if(it != storage_.end())
					return it->second;
				return nullptr;
			}

			TYPE* Add(TYPE* node)
			{
				storage_[node->name_] = node;
				return node;
			}

			Core::Map<Core::String, TYPE*> storage_;
		};

		NodeMap<AST::NodeStorageClass> storageClassNodes_;
		NodeMap<AST::NodeModifier> modifierNodes_;
		NodeMap<AST::NodeType> typeNodes_;
		NodeMap<AST::NodeStruct> structNodes_;

		Core::Vector<AST::NodeAttribute*> attributeNodes_;

		Core::Set<Core::String> reserved_;

		Core::Vector<AST::Node*> allocatedNodes_;
	};
} // namespace Graphics
