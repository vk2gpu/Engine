#include "shader_parser.h"
#include "gpu/enum.h"

#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4505)
#define STB_C_LEXER_IMPLEMENTATION
#include <stb_c_lexer.h>
#pragma warning(pop)

namespace
{
	using namespace Graphics::AST;

	// clang-format off
	NodeStorageClass STORAGE_CLASSES[] = 
	{
		"extern",
		"nointerpolation",
		"precise",
		"shared",
		"groupshared",
		"globallycoherent",
		"static",
		"uniform",
		"volatile",
		"in",
		"out",
		"inout",

		// Geometry shader only.
		"point",
		"line",
		"triangle",
		"lineadj",
		"triangleadj",
	};

	NodeModifier MODIFIERS[] =
	{
		"const",
		"row_major",
		"column_major",
	};

	NodeType BASE_TYPES[] =
	{
		{"void", 0},
		{"float", 4},
		{"float2", 8},
		{"float3", 12},
		{"float4", 16},
		{"float3x3", 36},
		{"float4x4", 64},
		{"int", 4},
		{"int2", 8},
		{"int3", 12},
		{"int4", 16},
		{"uint", 4},
		{"uint2", 8},
		{"uint3", 12},
		{"uint4", 16},
	};

	NodeType STREAM_TYPES[] =
	{
		{"PointStream", -1, "STREAM"}, 
		{"LineStream", -1, "STREAM"}, 
		{"TriangleStream", -1, "STREAM"}, 
	};

	NodeType SRV_TYPES[] =
	{
		{"Buffer", -1, "SRV"}, 
		{"ByteAddressBuffer", -1, "SRV"}, 
		{"StructuredBuffer", -1, "SRV"}, 
		{"Texture1D", -1, "SRV"}, 
		{"Texture1DArray", -1, "SRV"}, 
		{"Texture2D", -1, "SRV"}, 
		{"Texture2DArray", -1, "SRV"}, 
		{"Texture3D", -1, "SRV"}, 
		{"Texture2DMS", -1, "SRV"}, 
		{"Texture2DMSArray", -1, "SRV"}, 
		{"TextureCube", -1, "SRV"}, 
		{"TextureCubeArray", -1, "SRV"}, 
	};

	NodeType UAV_TYPES[] =
	{
		{"RWBuffer", -1, "UAV"}, 
		{"RWByteAddressBuffer", -1, "UAV"}, 
		{"RWStructuredBuffer", -1, "UAV"}, 
		{"RWTexture1D", -1, "UAV"}, 
		{"RWTexture1DArray", -1, "UAV"}, 
		{"RWTexture2D", -1, "UAV"}, 
		{"RWTexture2DArray", -1, "UAV"}, 
		{"RWTexture3D", -1, "UAV"}, 
	};

	NodeType ENUM_TYPES[] =
	{
		{"AddressingMode", GPU::AddressingMode::MAX},
		{"FilteringMode", GPU::FilteringMode::MAX},
		{"FillMode", GPU::FillMode::MAX},
		{"CullMode", GPU::CullMode::MAX},
		{"BlendType", GPU::BlendType::MAX},
		{"BlendFunc", GPU::BlendFunc::MAX},
		{"CompareMode", GPU::CompareMode::MAX},
		{"StencilFunc", GPU::StencilFunc::MAX},
	};

	const char* BASE_LIBRARY = R"(
		[internal("SamplerState")]
		struct SamplerState
		{
			AddressingMode AddressU;
			AddressingMode AddressV;
			AddressingMode AddressW;
			FilteringMode MinFilter;
			FilteringMode MagFilter;
			float MipLODBias;
			uint MaxAnisotropy;
			float BorderColor;//[4];
			float MinLOD;
			float MaxLOD;
		};

		[internal("BlendState")]
		struct BlendState
		{
			uint Enable;
			BlendType SrcBlend;
			BlendType DestBlend;
			BlendFunc BlendOp;
			BlendType SrcBlendAlpha;
			BlendType DestBlendAlpha;
			BlendFunc BlendOpAlpha;
			uint WriteMask;
		};

		[internal("StencilFaceState")]
		struct StencilFaceState
		{
			StencilFunc Fail;
			StencilFunc DepthFail;
			StencilFunc Pass;
			CompareMode Func;
		};

		[internal("RenderState")]
		struct RenderState
		{
			BlendState BlendStates;//[8];

			StencilFaceState StencilFront;
			StencilFaceState StencilBack;
			uint DepthEnable;
			uint DepthWriteMask;
			CompareMode DepthFunc;
			uint StencilEnable;
			uint StencilRef;
			uint StencilRead;
			uint StencilWrite;

			FillMode FillMode;
			CullMode CullMode;
			float DepthBias;
			float SlopeScaledDepthBias;
			uint AntialiasedLineEnable;
		};

		[internal("Technique")]
		struct Technique
		{
			[fn("VertexShader")]
			void VertexShader;

			[fn("GeometryShader")]
			void GeometryShader;

			[fn("HullShader")]
			void HullShader;

			[fn("DomainShader")]
			void DomainShader;

			[fn("PixelShader")]
			void PixelShader;

			[fn("ComputeShader")]
			void ComputeShader;

			RenderState RenderState;
		};
	)";

	// clang-format on
}

namespace Graphics
{
	ShaderParser::ShaderParser()
	{
		AddNodes(STORAGE_CLASSES);
		AddReserved(STORAGE_CLASSES);

		AddNodes(MODIFIERS);
		AddReserved(MODIFIERS);
		AddNodes(BASE_TYPES);
		AddNodes(STREAM_TYPES);
		AddNodes(SRV_TYPES);
		AddNodes(UAV_TYPES);
		AddNodes(ENUM_TYPES);
	}

	ShaderParser::~ShaderParser()
	{
		// Free all allocated nodes.
		for(auto* node : allocatedNodes_)
		{
			delete node;
		}
	}

#define CHECK_TOKEN(expectedType, expectedToken)                                                                       \
	if(*expectedToken != '\0' && token_.value_ != expectedToken)                                                       \
	{                                                                                                                  \
		Error(node, ErrorType::UNEXPECTED_TOKEN,                                                                       \
		    Core::String().Printf(                                                                                     \
		        "\'%s\': Unexpected token. Did you mean \'%s\'?", token_.value_.c_str(), expectedToken));              \
		return node;                                                                                                   \
	}                                                                                                                  \
	if(token_.type_ != expectedType)                                                                                   \
	{                                                                                                                  \
		Error(node, ErrorType::UNEXPECTED_TOKEN,                                                                       \
		    Core::String().Printf("\'%s\': Unexpected token.", token_.value_.c_str()));                                \
		return node;                                                                                                   \
	}                                                                                                                  \
	while(false)

#define PARSE_TOKEN()                                                                                                  \
	if(!NextToken())                                                                                                   \
	{                                                                                                                  \
		Error(node, ErrorType::UNEXPECTED_EOF, Core::String().Printf("Unexpected EOF"));                               \
		return node;                                                                                                   \
	}

	AST::NodeShaderFile* ShaderParser::Parse(
	    const char* shaderFileName, const char* shaderCode, IShaderParserCallbacks* callbacks)
	{
		callbacks_ = callbacks;
		Core::Vector<char> stringStore;
		stringStore.resize(1024 * 1024);

		Core::String patchedShaderCode = BASE_LIBRARY;
		patchedShaderCode.Append("\n");
		patchedShaderCode.Append(shaderCode);

		stb_c_lexer_init(
		    &lexCtx_, patchedShaderCode.begin(), patchedShaderCode.end(), stringStore.data(), stringStore.size());

		fileName_ = shaderFileName;
		AST::NodeShaderFile* shaderFile = ParseShaderFile();
		if(shaderFile)
			shaderFile->name_ = shaderFileName;

		// If there are any errors, don't return the shader file.
		if(numErrors_ > 0)
		{
			shaderFile = nullptr;
		}

		return shaderFile;
	}

	AST::NodeShaderFile* ShaderParser::ParseShaderFile()
	{
		AST::NodeShaderFile* node = AddNode<AST::NodeShaderFile>();
		shaderFileNode_ = node;

		while(NextToken())
		{
			switch(token_.type_)
			{
			case AST::TokenType::CHAR:
			{
				auto* attributeNode = ParseAttribute();
				if(attributeNode)
					attributeNodes_.push_back(attributeNode);
			}
			break;
			case AST::TokenType::IDENTIFIER:
			{
				if(token_.value_ == "struct" || token_.value_ == "cbuffer")
				{
					auto* structNode = ParseStruct();
					if(structNode)
					{
						structNodes_.Add(structNode);
						if(structNode->isCBuffer_)
						{
							node->cbuffers_.push_back(structNode);
						}
						else
						{
							node->structs_.push_back(structNode);
						}
					}
				}
				else
				{
					auto* declNode = ParseDeclaration();
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

							node->variables_.push_back(declNode);
						}
					}
				}
			}
			break;
			}
		}

		return node;
	}

	AST::NodeAttribute* ShaderParser::ParseAttribute()
	{
		AST::NodeAttribute* node = nullptr;

		if(token_.value_ == "[")
		{
			PARSE_TOKEN();
			CHECK_TOKEN(AST::TokenType::IDENTIFIER, "");
			node = AddNode<AST::NodeAttribute>(token_.value_.c_str());

			PARSE_TOKEN();
			if(token_.value_ == "(")
			{
				PARSE_TOKEN();
				while(token_.value_ != ")")
				{
					if(token_.type_ == AST::TokenType::INT || token_.type_ == AST::TokenType::FLOAT ||
					    token_.type_ == AST::TokenType::STRING)
					{
						node->parameters_.emplace_back(token_.value_);
						PARSE_TOKEN();
					}
					else
					{
						Error(node, ErrorType::UNEXPECTED_TOKEN,
						    Core::String().Printf(
						        "\'%s\': Unexpected token. Should be uint, int, float or string value.",
						        token_.value_.c_str()));
						return node;
					}

					if(token_.type_ == AST::TokenType::CHAR && token_.value_ == ",")
					{
						PARSE_TOKEN();
					}
				}
				PARSE_TOKEN();
			}
			CHECK_TOKEN(AST::TokenType::CHAR, "]");
		}
		return node;
	}

	AST::NodeStorageClass* ShaderParser::ParseStorageClass()
	{
		AST::NodeStorageClass* node = nullptr;
		if(Find(node, token_.value_))
		{
			PARSE_TOKEN();
		}
		return node;
	}

	AST::NodeModifier* ShaderParser::ParseModifier()
	{
		AST::NodeModifier* node = nullptr;
		if(Find(node, token_.value_))
		{
			PARSE_TOKEN();
		}
		return node;
	}

	AST::NodeType* ShaderParser::ParseType()
	{
		AST::NodeType* node = nullptr;

		CHECK_TOKEN(AST::TokenType::IDENTIFIER, "");

		// Check it's not a reserved keyword.
		if(reserved_.find(token_.value_) != reserved_.end())
		{
			Error(node, ErrorType::RESERVED_KEYWORD,
			    Core::String().Printf("\'%s\': is a reserved keyword. Type expected.", token_.value_.c_str()));
			return node;
		}

		// Find type.
		if(!Find(node, token_.value_))
		{
			Error(node, ErrorType::TYPE_MISSING, Core::String().Printf("\'%s\': type missing", token_.value_.c_str()));
			return node;
		}

		return node;
	}

	AST::NodeTypeIdent* ShaderParser::ParseTypeIdent()
	{
		AST::NodeTypeIdent* node = AddNode<AST::NodeTypeIdent>();

		// Parse base modifiers.
		while(auto* modifierNode = ParseModifier())
		{
			node->baseModifiers_.push_back(modifierNode);
		}

		// Parse type.
		node->baseType_ = ParseType();

		// check for template.
		PARSE_TOKEN();
		if(token_.value_ == "<")
		{
			PARSE_TOKEN();

			while(auto* modifierNode = ParseModifier())
			{
				node->templateModifiers_.push_back(modifierNode);
			}

			node->templateType_ = ParseType();

			PARSE_TOKEN();
			CHECK_TOKEN(TokenType::CHAR, ">");
			PARSE_TOKEN();
		}

		return node;
	}

	AST::NodeStruct* ShaderParser::ParseStruct()
	{
		AST::NodeStruct* node = nullptr;

		// Check if cbuffer or struct.
		bool isCBuffer = false;
		if(token_.value_ == "cbuffer")
		{
			isCBuffer = true;
		}
		else
		{
			CHECK_TOKEN(AST::TokenType::IDENTIFIER, "struct");
		}

		// Parse name.
		PARSE_TOKEN();
		CHECK_TOKEN(AST::TokenType::IDENTIFIER, "");
		node = AddNode<AST::NodeStruct>(token_.value_.c_str());
		node->isCBuffer_ = isCBuffer;

		NodeType* nodeType = nullptr;
		if(Find(nodeType, token_.value_))
		{
			Error(node, ErrorType::TYPE_REDEFINITION,
			    Core::String().Printf(
			        "\'%s\': '%s' type redefinition.", isCBuffer ? "cbuffer" : "struct", token_.value_.c_str()));
			return node;
		}

		// Check if token is a reserved keyword.
		if(reserved_.find(token_.value_) != reserved_.end())
		{
			Error(node, ErrorType::RESERVED_KEYWORD,
			    Core::String().Printf("\'%s\': is a reserved keyword. Type expected.", token_.value_.c_str()));
			return node;
		}

		//
		node->type_ = AddNode<AST::NodeType>(token_.value_.c_str(), -1);
		node->type_->struct_ = node;

		// consume attributes.
		node->attributes_ = std::move(attributeNodes_);

		// Parse {
		PARSE_TOKEN();
		if(token_.value_ == "{")
		{
			PARSE_TOKEN();
			while(token_.value_ != "}")
			{
				while(auto* attributeNode = ParseAttribute())
				{
					PARSE_TOKEN();
					attributeNodes_.push_back(attributeNode);
				}

				CHECK_TOKEN(AST::TokenType::IDENTIFIER, "");
				auto* memberNode = ParseDeclaration();
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


	AST::NodeDeclaration* ShaderParser::ParseDeclaration()
	{
		AST::NodeDeclaration* node = nullptr;

		Core::Vector<NodeStorageClass*> storageClasses;

		// Parse storage class.
		while(auto* storageClassNode = ParseStorageClass())
		{
			storageClasses.push_back(storageClassNode);
		}

		// Parse type identifier.
		auto* nodeTypeIdent = ParseTypeIdent();

		// Check name.
		CHECK_TOKEN(AST::TokenType::IDENTIFIER, "");

		// Check if identifier is a reserved keyword.
		if(reserved_.find(token_.value_) != reserved_.end())
		{
			Error(node, ErrorType::RESERVED_KEYWORD,
			    Core::String().Printf("\'%s\': is a reserved keyword.", token_.value_.c_str()));
			return node;
		}
		node = AddNode<AST::NodeDeclaration>(token_.value_.c_str());
		node->storageClasses_ = std::move(storageClasses);
		node->type_ = nodeTypeIdent;

		// consume attributes.
		node->attributes_ = std::move(attributeNodes_);

		// Check for params/semantic/assignment/end.
		PARSE_TOKEN();

		// Arrays.
		node->arrayDims_.fill(0);
		if(token_.value_ == "[")
		{
			i32 arrayDim = 0;
			{
				PARSE_TOKEN();
				CHECK_TOKEN(AST::TokenType::INT, "");

				node->arrayDims_[arrayDim++] = token_.valueInt_;

				PARSE_TOKEN();
				CHECK_TOKEN(AST::TokenType::CHAR, "]");

				PARSE_TOKEN();
			}
			while(token_.value_ == "[" && arrayDim < 3)
				;
		}

		else if(token_.value_ == "(")
		{
			node->isFunction_ = true;

			PARSE_TOKEN();
			while(token_.value_ != ")")
			{
				CHECK_TOKEN(AST::TokenType::IDENTIFIER, "");
				auto* parameterNode = ParseDeclaration();
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
				Error(node, ErrorType::RESERVED_KEYWORD,
				    Core::String().Printf("\'%s\': is a reserved keyword. Semantic expected.", token_.value_.c_str()));
				return node;
			}

			node->semantic_ = token_.value_;

			PARSE_TOKEN();
		}

		if(node->isFunction_)
		{
			// Capture function body.
			if(token_.value_ == "{")
			{
				const char* beginCode = lexCtx_.parse_point;
				const char* endCode = nullptr;
				i32 scopeLevel = 1;
				i32 parenLevel = 0;
				i32 bracketLevel = 0;
				while(scopeLevel > 0)
				{
					endCode = lexCtx_.parse_point;
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
						Error(node, ErrorType::UNMATCHED_PARENTHESIS,
						    Core::String().Printf("\'%s\': Unmatched parenthesis.", token_.value_.c_str()));
						return node;
					}
					if(bracketLevel < 0)
					{
						Error(node, ErrorType::UNMATCHED_BRACKET,
						    Core::String().Printf("\'%s\': Unmatched parenthesis.", token_.value_.c_str()));
						return node;
					}
				}

				if(parenLevel > 0)
				{
					Error(node, ErrorType::UNMATCHED_PARENTHESIS,
					    Core::String().Printf("\'%s\': Missing ')'", token_.value_.c_str()));
					return node;
				}
				if(bracketLevel > 0)
				{
					Error(node, ErrorType::UNMATCHED_BRACKET,
					    Core::String().Printf("\'%s\': Missing ']'.", token_.value_.c_str()));
					return node;
				}

				node->value_ = AddNode<AST::NodeValue>();
				node->value_->type_ = AST::ValueType::RAW_CODE;
				node->value_->data_ = Core::String(beginCode, endCode);
			}
		}
		else if(token_.value_ == "=")
		{
			PARSE_TOKEN();
			node->value_ = ParseValue(node->type_->baseType_, nullptr);
		}


		return node;
	}

	AST::NodeValue* ShaderParser::ParseValue(AST::NodeType* nodeType, AST::NodeDeclaration* nodeDeclaration)
	{
		AST::NodeValue* node = nullptr;

		// Attempt to parse values.
		if(nodeType->members_.size() > 0)
		{
			node = ParseValues(nodeType);
			if(node != nullptr)
			{
				return node;
			}

			// Attempt to parse member value.
			node = ParseMemberValue(nodeType);
			if(node != nullptr)
			{
				return node;
			}
		}

		if(token_.type_ == AST::TokenType::FLOAT || token_.type_ == AST::TokenType::INT ||
		    token_.type_ == AST::TokenType::STRING || token_.type_ == AST::TokenType::IDENTIFIER)
		{
			auto* nodeValue = AddNode<AST::NodeValue>();
			switch(token_.type_)
			{
			case AST::TokenType::FLOAT:
				nodeValue->type_ = AST::ValueType::FLOAT;
				nodeValue->dataFloat_ = token_.valueFloat_;
				break;
			case AST::TokenType::INT:
				nodeValue->type_ = AST::ValueType::INT;
				nodeValue->dataInt_ = token_.valueInt_;
				break;
			case AST::TokenType::STRING:
				nodeValue->type_ = AST::ValueType::STRING;
				break;
			case AST::TokenType::IDENTIFIER:
			{
				if(nodeType->IsEnum())
				{
					nodeValue->type_ = AST::ValueType::ENUM;
					if(nodeType->HasEnumValue(token_.value_.c_str()))
					{
						nodeValue->data_ = token_.value_;
					}
					else
					{
						Core::String errorStr;
						errorStr.Printf("\'%s\': Invalid value. Expecting enum value for \'%s\'. Valid values are:\n",
						    token_.value_.c_str(), nodeType->name_.c_str());
						for(i32 idx = 0; idx < nodeType->maxEnumValue_; ++idx)
						{
							errorStr.Appendf(" - %s\n", nodeType->FindEnumName(idx));
						}

						Error(node, ErrorType::INVALID_VALUE, errorStr);
						return node;
					}
				}
				else
				{
					if(nodeDeclaration != nullptr && nodeDeclaration->FindAttribute("fn"))
					{
						// Check if the type is matched by a function in the shader file.
						if(auto nodeVariable = shaderFileNode_->FindFunction(token_.value_.c_str()))
						{
							if(!nodeVariable->isFunction_)
							{
								Core::String errorStr;
								errorStr.Printf(
								    "\'%s\': has invalid type. Expecting type function.", token_.value_.c_str());
								Error(node, ErrorType::INVALID_TYPE, errorStr);
								return node;
							}

							nodeValue->type_ = AST::ValueType::IDENTIFIER;
						}
						else
						{
							Core::String errorStr;
							errorStr.Printf("\'%s\': Identifier missing.", token_.value_.c_str());
							Error(node, ErrorType::IDENTIFIER_MISSING, errorStr);
							return node;
						}
					}
					else
					{
						// Check if the type is matched by a variable in the shader file.
						if(auto nodeVariable = shaderFileNode_->FindVariable(token_.value_.c_str()))
						{
							if(nodeVariable->type_->baseType_ != nodeType)
							{
								Core::String errorStr;
								errorStr.Printf("\'%s\': has invalid type. Expecting type \'%s\'",
								    token_.value_.c_str(), nodeType->name_.c_str());
								Error(node, ErrorType::INVALID_TYPE, errorStr);
								return node;
							}

							nodeValue->type_ = AST::ValueType::IDENTIFIER;
						}
						else
						{
							Core::String errorStr;
							errorStr.Printf("\'%s\': Identifier missing.", token_.value_.c_str());
							Error(node, ErrorType::IDENTIFIER_MISSING, errorStr);
							return node;
						}
					}
				}
			}
			break;
			default:
				break;
			}
			nodeValue->data_ = token_.value_;
			node = nodeValue;
			PARSE_TOKEN();

			return node;
		}
		else
		{
			Error(node, ErrorType::UNEXPECTED_TOKEN,
			    Core::String().Printf("\'%s\': Unexpected token.", token_.value_.c_str()));
		}
		return node;
	}

	AST::NodeValues* ShaderParser::ParseValues(AST::NodeType* nodeType)
	{
		AST::NodeValues* node = nullptr;

		if(token_.type_ == AST::TokenType::CHAR && token_.value_ == "{")
		{
			node = AddNode<AST::NodeValues>();

			PARSE_TOKEN();
			const char* lastParsePoint = lexCtx_.parse_point;
			while(token_.value_ != "}")
			{
				auto nodeValue = ParseMemberValue(nodeType);
				if(nodeValue)
					node->values_.push_back(nodeValue);

				if(token_.value_ == ";")
					return node;

				if(token_.type_ == AST::TokenType::CHAR && token_.value_ == ",")
				{
					PARSE_TOKEN();
				}

				if(lastParsePoint == lexCtx_.parse_point)
				{
					Error(node, ErrorType::UNEXPECTED_TOKEN,
					    Core::String().Printf("\'%s\': Unexpected token.", token_.value_.c_str()));
					return node;
				}
				lastParsePoint = lexCtx_.parse_point;
			}
			PARSE_TOKEN();
		}

		return node;
	}

	AST::NodeMemberValue* ShaderParser::ParseMemberValue(AST::NodeType* nodeType)
	{
		AST::NodeMemberValue* node = nullptr;

		if(token_.type_ == AST::TokenType::CHAR && token_.value_ == ".")
		{
			PARSE_TOKEN();
			node = AddNode<AST::NodeMemberValue>();

			node->member_ = token_.value_;

			auto* memberType = nodeType->FindMember(node->member_.c_str());
			if(memberType == nullptr)
			{
				Core::String errorStr;
				errorStr.Printf("\'%s\': Invalid member. Valid values are:\n", token_.value_.c_str());
				for(auto* member : nodeType->members_)
				{
					errorStr.Appendf(" - %s\n", member->name_.c_str());
				}
				Error(node, ErrorType::INVALID_MEMBER, errorStr);
			}

			PARSE_TOKEN();
			CHECK_TOKEN(AST::TokenType::CHAR, "=");

			PARSE_TOKEN();
			node->value_ = ParseValue(memberType->type_->baseType_, memberType);
		}

		return node;
	}

	bool ShaderParser::NextToken()
	{
		while(stb_c_lexer_get_token(&lexCtx_) > 0)
		{
			if(lexCtx_.token < CLEX_eof)
			{
				char tokenStr[] = {(char)lexCtx_.token, '\0'};
				token_.type_ = AST::TokenType::CHAR;
				token_.value_ = tokenStr;
			}
			else if(lexCtx_.token == CLEX_id)
			{
				token_.type_ = AST::TokenType::IDENTIFIER;
				token_.value_ = lexCtx_.string;
			}
			else if(lexCtx_.token == CLEX_floatlit)
			{
				token_.type_ = AST::TokenType::FLOAT;
				token_.value_ = Core::String(lexCtx_.where_firstchar, lexCtx_.where_lastchar + 1);
				token_.valueFloat_ = (f32)lexCtx_.real_number;
			}
			else if(lexCtx_.token == CLEX_intlit)
			{
				token_.type_ = AST::TokenType::INT;
				token_.value_ = Core::String(lexCtx_.where_firstchar, lexCtx_.where_lastchar + 1);
				token_.valueInt_ = lexCtx_.int_number;
			}
			else if(lexCtx_.token == CLEX_dqstring)
			{
				token_.type_ = AST::TokenType::STRING;
				token_.value_ = lexCtx_.string;
			}
			else
			{
				token_.type_ = AST::TokenType::CHAR;
				token_.value_ = lexCtx_.string;
			}
			token_ = token_;
			return true;
		}
		token_ = AST::Token();
		return false;
	}

	AST::Token ShaderParser::GetToken() const { return token_; }

	void ShaderParser::Error(AST::Node*& node, ErrorType errorType, Core::StringView errorStr)
	{
		stb_lex_location loc;
		stb_c_lexer_get_location(&lexCtx_, lexCtx_.parse_point, &loc);

		const char* line_start = lexCtx_.parse_point - loc.line_offset;
		const char* line_end = strstr(line_start, "\n");
		Core::String line(line_start, line_end);
		if(callbacks_)
		{
			callbacks_->OnError(
			    errorType, fileName_, loc.line_number, loc.line_offset, line.c_str(), errorStr.ToString().c_str());
		}
		else
		{
			Core::Log("%s(%i-%i): error: %u: %s\n", fileName_, loc.line_number, loc.line_offset, (u32)errorType,
			    errorStr.ToString().c_str());
			Core::Log("%s\n", line.c_str());
			Core::String indent("> ");
			indent.reserve(loc.line_offset + 1);
			for(i32 idx = 1; idx < loc.line_offset; ++idx)
				indent += " ";
			indent += "^";
			Core::Log("> %s\n", indent.c_str());
		}

		numErrors_++;
		node = nullptr;

		// Force fast fail.
		NextToken();
	}
} // namespace Graphics
