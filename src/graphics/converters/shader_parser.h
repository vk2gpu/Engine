#pragma once

#include "core/types.h"
#include "core/enum.h"
#include "core/map.h"
#include "core/set.h"
#include "core/string.h"
#include "graphics/converters/shader_ast.h"

#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4505)
#include <stb_c_lexer.h>
#pragma warning(pop)

namespace Graphics
{
	enum class ErrorType : i32
	{
		FIRST_ERROR = 100,

		PARSE_ERROR,
		UNEXPECTED_EOF,
		UNEXPECTED_TOKEN,
		TYPE_REDEFINITION,
		TYPE_MISSING,
		IDENTIFIER_MISSING,
		FUNCTION_REDEFINITION,
		UNMATCHED_PARENTHESIS,
		UNMATCHED_BRACKET,
		RESERVED_KEYWORD,
		INVALID_MEMBER,
		INVALID_TYPE,
		INVALID_VALUE,
		INTERNAL_ERROR,
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

		AST::NodeShaderFile* Parse(
		    const char* shaderFileName, const char* shaderCode, IShaderParserCallbacks* callbacks = nullptr);

	private:
		AST::NodeShaderFile* ParseShaderFile();
		AST::NodeAttribute* ParseAttribute();
		AST::NodeStorageClass* ParseStorageClass();
		AST::NodeModifier* ParseModifier();
		AST::NodeType* ParseType();
		AST::NodeTypeIdent* ParseTypeIdent();
		AST::NodeStruct* ParseStruct();
		AST::NodeDeclaration* ParseDeclaration();
		AST::NodeValue* ParseValue(AST::NodeType* nodeType, AST::NodeDeclaration* nodeDeclaration);
		AST::NodeValues* ParseValues(AST::NodeType* nodeType);
		AST::NodeMemberValue* ParseMemberValue(AST::NodeType* nodeType);

		bool NextToken();
		AST::Token GetToken() const;

		bool GetCurrentLine(i32& outLineNum, i32& outLineOff, const char*& outFile) const;

		void Error(AST::Node*& node, ErrorType errorType, Core::StringView errorStr);

		template<typename NODE_TYPE>
		void Error(NODE_TYPE*& node, ErrorType errorType, Core::StringView errorStr)
		{
			Error(reinterpret_cast<AST::Node*&>(node), errorType, errorStr);
		}

		// Find node by name.
		template<typename TYPE>
		bool Find(TYPE*& node, const char* name)
		{
			return false;
		}
		bool Find(AST::NodeStorageClass*& node, const char* name)
		{
			return (node = storageClassNodes_.Find(name)) != nullptr;
		}
		bool Find(AST::NodeModifier*& node, const char* name) { return (node = modifierNodes_.Find(name)) != nullptr; }
		bool Find(AST::NodeType*& node, const char* name) { return (node = typeNodes_.Find(name)) != nullptr; }
		bool Find(AST::NodeStruct*& node, const char* name) { return (node = structNodes_.Find(name)) != nullptr; }

		template<typename TYPE>
		bool Find(TYPE*& node, const Core::String& name)
		{
			return Find(node, name.c_str());
		}

	public:
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
		AST::NodeStruct* AddToMap(AST::NodeStruct* node) { return structNodes_.Add(node); }

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

		Core::Set<Core::String> structTypes_;

		struct LineDirective
		{
			/// Line in the parsed file.
			i32 sourceLine_ = 1;

			/// Line in @a file_
			i32 line_ = 1;
			Core::String file_;
		};

		Core::Vector<LineDirective> lineDirectives_;

		AST::NodeShaderFile* shaderFileNode_ = nullptr;

		Core::Vector<AST::NodeAttribute*> attributeNodes_;

		Core::Set<Core::String> reserved_;

		Core::Vector<AST::Node*> allocatedNodes_;
		i32 numErrors_ = 0;
		stb_lexer lexCtx_;
	};
} // namespace Graphics
