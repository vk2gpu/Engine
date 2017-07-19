#include "shader_parser.h"

#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4505)
#define STB_C_LEXER_IMPLEMENTATION
#include <stb_c_lexer.h>
#pragma warning(pop)

namespace
{
	using namespace Graphics::AST;
	i32 indent = 0;

	struct ScopedIndent
	{
		ScopedIndent() { ++indent; }
		~ScopedIndent() { --indent; }
	};

	void LogToken(Token token_)
	{
		Core::String outStr;
		const char* type = "INVALID:";
		switch(token_.type_)
		{
		case TokenType::IDENTIFIER:
			type = "IDENTIFIER";
			break;
		case TokenType::CHAR:
			type = "CHAR";
			break;
		case TokenType::INT:
			type = "INT";
			break;
		case TokenType::FLOAT:
			type = "FLOAT";
			break;
		case TokenType::STRING:
			type = "STRING";
			break;
		}

		for(i32 idx = 0; idx < indent; ++idx)
		{
			outStr.Appendf(" -");
		}
		outStr.Appendf(" \"%s\" (%s)", token_.value_.c_str(), type);
		Core::Log("%s\n", outStr.c_str());
	}
}

namespace Graphics
{
	namespace AST
	{
	} // namespace AST

	ShaderParser::ShaderParser()
	{
		AddType("void", 0);
		AddType("float", 4);
		AddType("float2", 8);
		AddType("float3", 12);
		AddType("float4", 16);
		AddType("float3x3", 36);
		AddType("float4x4", 64);
		AddType("int", 4);
		AddType("int2", 8);
		AddType("int3", 12);
		AddType("int4", 16);
		AddType("uint", 4);
		AddType("uint2", 8);
		AddType("uint3", 12);
		AddType("uint4", 16);

		const char* srvTypes[] = {
		    "Buffer", "ByteAddressBuffer", "StructuredBuffer", "Texture1D", "Texture1DArray", "Texture2D",
		    "Texture2DArray", "Texture3D", "Texture2DMS", "Texture2DMSArray", "TextureCube", "TextureCubeArray",
		};

		const char* uavTypes[] = {
		    "RWBuffer", "RWByteAddressBuffer", "RWStructuredBuffer", "RWTexture1D", "RWTexture1DArray", "RWTexture2D",
		    "RWTexture2DArray", "RWTexture3D",
		};

		for(auto type : srvTypes)
			AddType(type, -1);
		for(auto type : uavTypes)
			AddType(type, -1);

		AddStorageClass("extern");
		AddStorageClass("nointerpolation");
		AddStorageClass("precise");
		AddStorageClass("shared");
		AddStorageClass("groupshared");
		AddStorageClass("static");
		AddStorageClass("uniform");
		AddStorageClass("volatile");

		AddModifier("const");
		AddModifier("row_major");
		AddModifier("column_major");
	}

	ShaderParser::~ShaderParser() {}

#define CHECK_TOKEN(expectedType, expectedToken)                                                                       \
	if(*expectedToken != '\0' && token_.value_ != expectedToken)                                                       \
	{                                                                                                                  \
		Error(lexCtx, node, ErrorType::UNEXPECTED_TOKEN,                                                               \
		    Core::String().Printf(                                                                                     \
		        "\'%s\': Unexpected token. Did you mean \'%s\'?", token_.value_.c_str(), expectedToken));              \
		return node;                                                                                                   \
	}                                                                                                                  \
	if(token_.type_ != expectedType)                                                                                   \
	{                                                                                                                  \
		Error(lexCtx, node, ErrorType::UNEXPECTED_TOKEN,                                                               \
		    Core::String().Printf("\'%s\': Unexpected token.", token_.value_.c_str()));                                \
		return node;                                                                                                   \
	}                                                                                                                  \
	while(false)

#define PARSE_TOKEN()                                                                                                  \
	if(!NextToken(lexCtx))                                                                                             \
	{                                                                                                                  \
		Error(lexCtx, node, ErrorType::UNEXPECTED_EOF, Core::String().Printf("Unexpected EOF"));                       \
		return node;                                                                                                   \
	}

	void ShaderParser::Parse(const char* shaderFileName, const char* shaderCode, IShaderParserCallbacks* callbacks)
	{
		callbacks_ = callbacks;
		Core::Vector<char> stringStore;
		stringStore.resize(1024 * 1024);

		stb_lexer lexCtx;
		stb_c_lexer_init(&lexCtx, shaderCode, shaderCode + strlen(shaderCode), stringStore.data(), stringStore.size());

		fileName_ = shaderFileName;
		AST::NodeShaderFile* shaderFile = ParseShaderFile(lexCtx);

		++shaderFile;
	}

	AST::NodeShaderFile* ShaderParser::ParseShaderFile(stb_lexer& lexCtx)
	{
		ScopedIndent scopedIndent;
		AST::NodeShaderFile* node = new AST::NodeShaderFile();
		node->name_ = "<root>";

		while(NextToken(lexCtx))
		{
			switch(token_.type_)
			{
			case AST::TokenType::IDENTIFIER:
			{
				if(token_.value_ == "struct")
				{
					auto* structNode = ParseStruct(lexCtx);
					if(structNode)
					{
						structNodes_[structNode->name_] = structNode;
						node->structs_.push_back(structNode);
					}
				}
				else
				{
					auto* declNode = ParseDeclaration(lexCtx);
					if(declNode)
					{
						if(declNode->isFunction_)
						{
							node->functions_.push_back(declNode);
						}
						else
						{
							// Trailing ;
							CHECK_TOKEN(AST::TokenType::CHAR, ";");

							node->parameters_.push_back(declNode);
						}
					}
				}
			}
			break;
			}
		}

		return node;
	}

	AST::NodeStorageClass* ShaderParser::ParseStorageClass(stb_lexer& lexCtx)
	{
		ScopedIndent scopedIndent;
		AST::NodeStorageClass* node = nullptr;

		node = FindStorageClass(token_.value_.c_str());
		if(node != nullptr)
		{
			PARSE_TOKEN();
		}
		return node;
	}

	AST::NodeModifier* ShaderParser::ParseModifier(stb_lexer& lexCtx)
	{
		ScopedIndent scopedIndent;
		AST::NodeModifier* node = nullptr;

		node = FindModifier(token_.value_.c_str());
		if(node != nullptr)
		{
			PARSE_TOKEN();
		}
		return node;
	}

	AST::NodeType* ShaderParser::ParseType(stb_lexer& lexCtx)
	{
		ScopedIndent scopedIndent;
		AST::NodeType* node = nullptr;

		CHECK_TOKEN(AST::TokenType::IDENTIFIER, "");

		// Check it's not a reserved keyword.
		if(reserved_.find(token_.value_) != reserved_.end())
		{
			Error(lexCtx, node, ErrorType::RESERVED_KEYWORD,
			    Core::String().Printf("\'%s\': is a reserved keyword. Type expected.", token_.value_.c_str()));
			return node;
		}

		// Find type.
		node = FindType(token_.value_.c_str());
		if(node == nullptr)
		{
			Error(lexCtx, node, ErrorType::TYPE_MISSING,
			    Core::String().Printf("\'%s\': type missing", token_.value_.c_str()));
			return node;
		}

		return node;
	}

	AST::NodeTypeIdent* ShaderParser::ParseTypeIdent(stb_lexer& lexCtx)
	{
		ScopedIndent scopedIndent;
		AST::NodeTypeIdent* node = new AST::NodeTypeIdent();

		// Parse base modifiers.
		while(auto* modifierNode = ParseModifier(lexCtx))
		{
			node->baseModifiers_.push_back(modifierNode);
		}

		// Parse type.
		node->baseType_ = ParseType(lexCtx);

		// check for template.
		PARSE_TOKEN();
		if(token_.value_ == "<")
		{
			PARSE_TOKEN();

			while(auto* modifierNode = ParseModifier(lexCtx))
			{
				node->templateModifiers_.push_back(modifierNode);
			}

			node->templateType_ = ParseType(lexCtx);

			PARSE_TOKEN();
			CHECK_TOKEN(TokenType::CHAR, ">");
			PARSE_TOKEN();
		}

		return node;
	}

	AST::NodeStruct* ShaderParser::ParseStruct(stb_lexer& lexCtx)
	{
		ScopedIndent scopedIndent;
		AST::NodeStruct* node = new AST::NodeStruct();

		// Check struct.
		CHECK_TOKEN(AST::TokenType::IDENTIFIER, "struct");

		// Parse name.
		PARSE_TOKEN();
		CHECK_TOKEN(AST::TokenType::IDENTIFIER, "");
		node->name_ = token_.value_;

		auto* nodeType = FindType(token_.value_.c_str());
		if(nodeType != nullptr)
		{
			Error(lexCtx, node, ErrorType::TYPE_REDEFINITION,
			    Core::String().Printf("\'%s\': 'struct' type redefinition.", token_.value_.c_str()));
			return node;
		}

		// Check if token is a reserved keyword.
		if(reserved_.find(token_.value_) != reserved_.end())
		{
			Error(lexCtx, node, ErrorType::RESERVED_KEYWORD,
			    Core::String().Printf("\'%s\': is a reserved keyword. Type expected.", token_.value_.c_str()));
			return node;
		}

		node->type_ = AddType(token_.value_.c_str(), -1);

		// Parse {
		PARSE_TOKEN();
		if(token_.value_ == "{")
		{
			PARSE_TOKEN();
			while(token_.value_ != "}")
			{
				CHECK_TOKEN(AST::TokenType::IDENTIFIER, "");
				auto* memberNode = ParseDeclaration(lexCtx);
				if(memberNode)
				{
					// Trailing ;
					CHECK_TOKEN(AST::TokenType::CHAR, ";");

					node->type_->members_.push_back(memberNode);
				}
				PARSE_TOKEN();
			}
			PARSE_TOKEN();
		}
		else if(token_.value_ != ";")
		{
			CHECK_TOKEN(AST::TokenType::CHAR, "{");
		}

		// Check ;
		CHECK_TOKEN(AST::TokenType::CHAR, ";");

		return node;
	}


	AST::NodeDeclaration* ShaderParser::ParseDeclaration(stb_lexer& lexCtx)
	{
		ScopedIndent scopedIndent;
		AST::NodeDeclaration* node = new AST::NodeDeclaration();

		// Parse storage class.
		while(auto* storageClassNode = ParseStorageClass(lexCtx))
		{
			node->storageClasses_.push_back(storageClassNode);
		}

		// Parse type identifier.
		node->type_ = ParseTypeIdent(lexCtx);

		// Check name.
		CHECK_TOKEN(AST::TokenType::IDENTIFIER, "");

		// Check if identifier is a reserved keyword.
		if(reserved_.find(token_.value_) != reserved_.end())
		{
			Error(lexCtx, node, ErrorType::RESERVED_KEYWORD,
			    Core::String().Printf("\'%s\': is a reserved keyword.", token_.value_.c_str()));
			return node;
		}

		node->name_ = token_.value_;

		// Check for params/semantic/assignment/end.
		PARSE_TOKEN();
		if(token_.value_ == "(")
		{
			node->isFunction_ = true;

			PARSE_TOKEN();
			while(token_.value_ != ")")
			{
				CHECK_TOKEN(AST::TokenType::IDENTIFIER, "");
				auto* parameterNode = ParseDeclaration(lexCtx);
				if(parameterNode)
				{
					// Trailing ,
					if(token_.value_ == "," || token_.value_ == ")")
					{
						node->parameters_.push_back(parameterNode);
					}

					if(token_.value_ == ",")
						PARSE_TOKEN();
				}
			}

			PARSE_TOKEN();
		}

		if(token_.value_ == ":")
		{
			// Parse semantic.
			PARSE_TOKEN();
			CHECK_TOKEN(AST::TokenType::IDENTIFIER, "");

			// Check if identifier is a reserved keyword.
			if(reserved_.find(token_.value_) != reserved_.end())
			{
				Error(lexCtx, node, ErrorType::RESERVED_KEYWORD,
				    Core::String().Printf("\'%s\': is a reserved keyword. Semantic expected.", token_.value_.c_str()));
				return node;
			}

			node->semantic_ = token_.value_;

			PARSE_TOKEN();
		}

		if(token_.value_ == "=")
		{
			// Parse expression until ;
			const char* beginCode = lexCtx.parse_point;
			const char* endCode = nullptr;
			i32 parenLevel = 0;
			i32 bracketLevel = 0;
			while(token_.value_ != ";")
			{
				endCode = lexCtx.parse_point;
				PARSE_TOKEN();
				if(token_.value_ == "(")
					++parenLevel;
				if(token_.value_ == ")")
					--parenLevel;
				if(token_.value_ == "[")
					++bracketLevel;
				if(token_.value_ == "]")
					--bracketLevel;

				if(parenLevel < 0)
				{
					Error(lexCtx, node, ErrorType::UNMATCHED_PARENTHESIS,
					    Core::String().Printf("\'%s\': Unmatched parenthesis.", token_.value_.c_str()));
					return node;
				}
				if(bracketLevel < 0)
				{
					Error(lexCtx, node, ErrorType::UNMATCHED_BRACKET,
					    Core::String().Printf("\'%s\': Unmatched parenthesis.", token_.value_.c_str()));
					return node;
				}
			}

			if(parenLevel > 0)
			{
				Error(lexCtx, node, ErrorType::UNMATCHED_PARENTHESIS,
				    Core::String().Printf("\'%s\': Missing ')'", token_.value_.c_str()));
				return node;
			}
			if(bracketLevel > 0)
			{
				Error(lexCtx, node, ErrorType::UNMATCHED_BRACKET,
				    Core::String().Printf("\'%s\': Missing ']'.", token_.value_.c_str()));
				return node;
			}

			node->value_ = Core::String(beginCode, endCode);
		}

		if(node->isFunction_)
		{
			// Capture function body.
			if(token_.value_ == "{")
			{
				const char* beginCode = lexCtx.parse_point;
				const char* endCode = nullptr;
				i32 scopeLevel = 1;
				i32 parenLevel = 0;
				i32 bracketLevel = 0;
				while(scopeLevel > 0)
				{
					endCode = lexCtx.parse_point;
					PARSE_TOKEN();
					if(token_.value_ == "{")
						++scopeLevel;
					if(token_.value_ == "}")
						--scopeLevel;
					if(token_.value_ == "(")
						++parenLevel;
					if(token_.value_ == ")")
						--parenLevel;
					if(token_.value_ == "[")
						++bracketLevel;
					if(token_.value_ == "]")
						--bracketLevel;

					if(parenLevel < 0)
					{
						Error(lexCtx, node, ErrorType::UNMATCHED_PARENTHESIS,
						    Core::String().Printf("\'%s\': Unmatched parenthesis.", token_.value_.c_str()));
						return node;
					}
					if(bracketLevel < 0)
					{
						Error(lexCtx, node, ErrorType::UNMATCHED_BRACKET,
						    Core::String().Printf("\'%s\': Unmatched parenthesis.", token_.value_.c_str()));
						return node;
					}
				}

				if(parenLevel > 0)
				{
					Error(lexCtx, node, ErrorType::UNMATCHED_PARENTHESIS,
					    Core::String().Printf("\'%s\': Missing ')'", token_.value_.c_str()));
					return node;
				}
				if(bracketLevel > 0)
				{
					Error(lexCtx, node, ErrorType::UNMATCHED_BRACKET,
					    Core::String().Printf("\'%s\': Missing ']'.", token_.value_.c_str()));
					return node;
				}

				node->value_ = Core::String(beginCode, endCode);
			}
		}

		return node;
	}

	bool ShaderParser::NextToken(stb_lexer& lexCtx)
	{
		while(stb_c_lexer_get_token(&lexCtx) > 0)
		{
			if(lexCtx.token < CLEX_eof)
			{
				char tokenStr[] = {(char)lexCtx.token, '\0'};
				token_.type_ = AST::TokenType::CHAR;
				token_.value_ = tokenStr;
			}
			else if(lexCtx.token == CLEX_id)
			{
				token_.type_ = AST::TokenType::IDENTIFIER;
				token_.value_ = lexCtx.string;
			}
			else if(lexCtx.token == CLEX_floatlit)
			{
				token_.type_ = AST::TokenType::FLOAT;
				token_.value_ = Core::String(lexCtx.where_firstchar, lexCtx.where_lastchar + 1);
			}
			else if(lexCtx.token == CLEX_intlit)
			{
				token_.type_ = AST::TokenType::INT;
				token_.value_ = Core::String(lexCtx.where_firstchar, lexCtx.where_lastchar + 1);
			}
			else if(lexCtx.token == CLEX_dqstring)
			{
				token_.type_ = AST::TokenType::STRING;
				token_.value_ = lexCtx.string;
			}
			token_ = token_;
			return true;
		}
		token_ = AST::Token();
		return false;
	}

	AST::Token ShaderParser::GetToken() const { return token_; }

	void ShaderParser::Error(stb_lexer& lexCtx, AST::Node*& node, ErrorType errorType, Core::StringView errorStr)
	{
		stb_lex_location loc;
		stb_c_lexer_get_location(&lexCtx, lexCtx.parse_point, &loc);

		if(callbacks_)
		{
			callbacks_->OnError(errorType, fileName_, loc.line_number, loc.line_offset, errorStr.ToString().c_str());
		}
		else
		{
			Core::Log(
			    "%s(%i): error: %u: %s\n", fileName_, loc.line_number, (u32)errorType, errorStr.ToString().c_str());
		}

		delete node;
		node = nullptr;
	}

	AST::NodeType* ShaderParser::AddType(const char* name, i32 size)
	{
		AST::NodeType* node = new AST::NodeType();
		node->name_ = name;
		node->size_ = size;

		typeNodes_[name] = node;
		return node;
	}

	AST::NodeType* ShaderParser::FindType(const char* name)
	{
		auto it = typeNodes_.find(name);
		if(it != typeNodes_.end())
			return it->second;
		return nullptr;
	}

	AST::NodeModifier* ShaderParser::AddModifier(const char* name)
	{
		AST::NodeModifier* node = new AST::NodeModifier();
		node->name_ = name;

		modifierNodes_[name] = node;
		reserved_.insert(name);
		return node;
	}

	AST::NodeModifier* ShaderParser::FindModifier(const char* name)
	{
		auto it = modifierNodes_.find(name);
		if(it != modifierNodes_.end())
			return it->second;
		return nullptr;
	}

	AST::NodeStorageClass* ShaderParser::AddStorageClass(const char* name)
	{
		AST::NodeStorageClass* node = new AST::NodeStorageClass();
		node->name_ = name;

		storageClassNodes_[name] = node;
		reserved_.insert(name);
		return node;
	}

	AST::NodeStorageClass* ShaderParser::FindStorageClass(const char* name)
	{
		auto it = storageClassNodes_.find(name);
		if(it != storageClassNodes_.end())
			return it->second;
		return nullptr;
	}

} // namespace Graphics
