#include "graphics/converters/shader_backend_hlsl.h"

#include <cstdarg>

namespace Graphics
{
	ShaderBackendHLSL::ShaderBackendHLSL() {}

	ShaderBackendHLSL::~ShaderBackendHLSL() {}

	bool ShaderBackendHLSL::VisitEnter(AST::NodeShaderFile* node)
	{
		Write("////////////////////////////////////////////////////////////////////////////////////////////////////");
		NextLine();
		Write("// generated shader for %s", node->name_.c_str());
		NextLine();
		Write("////////////////////////////////////////////////////////////////////////////////////////////////////");
		NextLine();
		NextLine();

		Write("////////////////////////////////////////////////////////////////////////////////////////////////////");
		NextLine();
		Write("// structs");
		NextLine();

		for(auto* intNode : node->structs_)
		{
			if(!IsInternal(intNode))
			{
				intNode->Visit(this);
				NextLine();
			}
		}


		Write("////////////////////////////////////////////////////////////////////////////////////////////////////");
		NextLine();
		Write("// sampler states");
		NextLine();

		writeInternalDeclaration_ = "SamplerState";
		for(auto* intNode : node->variables_)
			intNode->Visit(this);
		writeInternalDeclaration_ = nullptr;
		NextLine();

		Write("////////////////////////////////////////////////////////////////////////////////////////////////////");
		NextLine();
		Write("// vars");
		NextLine();

		for(auto* intNode : node->variables_)
			intNode->Visit(this);

		NextLine();

		Write("////////////////////////////////////////////////////////////////////////////////////////////////////");
		NextLine();
		Write("// functions");
		NextLine();

		for(auto* intNode : node->functions_)
		{
			intNode->Visit(this);
			NextLine();
		}

		return false;
	}

	void ShaderBackendHLSL::VisitExit(AST::NodeShaderFile* node) {}

	bool ShaderBackendHLSL::VisitEnter(AST::NodeAttribute* node)
	{
		// TODO: Only write if an HLSL compatible attribute.

		if(node->parameters_.size() == 0)
		{
			Write("[%s]\n", node->name_.c_str());
		}
		else
		{
			Write("[%s(", node->name_.c_str());
			const i32 num = node->parameters_.size();
			for(i32 idx = 0; idx < num; ++idx)
			{
				const auto& param = node->parameters_[idx];
				Write("%s", param.c_str());
				if(idx < (num - 1))
					Write(", ");
			}
			Write(")]");
			NextLine();
		}
		return true;
	}

	void ShaderBackendHLSL::VisitExit(AST::NodeAttribute* node) {}

	bool ShaderBackendHLSL::VisitEnter(AST::NodeStorageClass* node)
	{
		Write("%s", node->name_.c_str());
		return true;
	}

	void ShaderBackendHLSL::VisitExit(AST::NodeStorageClass* node) {}

	bool ShaderBackendHLSL::VisitEnter(AST::NodeModifier* node)
	{
		Write("%s", node->name_.c_str());
		return true;
	}

	void ShaderBackendHLSL::VisitExit(AST::NodeModifier* node) {}

	bool ShaderBackendHLSL::VisitEnter(AST::NodeType* node) { return true; }

	void ShaderBackendHLSL::VisitExit(AST::NodeType* node) {}

	bool ShaderBackendHLSL::VisitEnter(AST::NodeTypeIdent* node)
	{
		for(auto* mod : node->baseModifiers_)
		{
			mod->Visit(this);
			Write(" ");
		}
		Write("%s", node->baseType_->name_.c_str());

		if(node->templateType_ != nullptr)
		{
			Write("<");
			for(auto* mod : node->templateModifiers_)
			{
				mod->Visit(this);
				Write(" ");
			}
			Write("%s>", node->templateType_->name_.c_str());
		}
		return false;
	}

	void ShaderBackendHLSL::VisitExit(AST::NodeTypeIdent* node) {}

	bool ShaderBackendHLSL::VisitEnter(AST::NodeStruct* node)
	{
		for(auto* attrib : node->attributes_)
			attrib->Visit(this);

		Write("struct %s", node->name_.c_str());
		NextLine();
		Write("{");
		NextLine();

		++indent_;

		for(auto* member : node->type_->members_)
		{
			member->Visit(this);
		}

		--indent_;

		Write("};");
		NextLine();
		return false;
	}

	void ShaderBackendHLSL::VisitExit(AST::NodeStruct* node) {}

	bool ShaderBackendHLSL::VisitEnter(AST::NodeDeclaration* node)
	{
		if(writeInternalDeclaration_)
		{
			int a = 0;
			++a;
		}

		if(writeInternalDeclaration_ != nullptr && !IsInternal(node, writeInternalDeclaration_))
			return false;
		else if(writeInternalDeclaration_ == nullptr && IsInternal(node))
			return false;

		if(!writeInternalDeclaration_)
		{
			for(auto* attrib : node->attributes_)
				attrib->Visit(this);
		}

		for(auto* storageClass : node->storageClasses_)
			Write("%s ", storageClass->name_.c_str());

		for(auto* attrib : node->attributes_)
			attrib->Visit(this);

		node->type_->Visit(this);
		Write(" %s", node->name_.c_str());

		if(node->isFunction_)
		{
			++inParams_;
			Write("(");
			const i32 num = node->parameters_.size();
			for(i32 idx = 0; idx < num; ++idx)
			{
				auto* param = node->parameters_[idx];
				param->Visit(this);
				if(idx < (num - 1))
					Write(", ");
			}
			Write(")");
			--inParams_;

			if(node->semantic_.size() > 0)
				Write(" : %s", node->semantic_.c_str());

			NextLine();
			Write("{");
			node->value_->Visit(this);
			NextLine();
			Write("}");
			NextLine();
		}
		else
		{
			if(node->semantic_.size() > 0)
				Write(" : %s", node->semantic_.c_str());

			if(node->value_)
			{
				// Only output simple values.
				if(node->value_->nodeType_ == AST::Nodes::VALUE)
				{
					Write(" = ");
					node->value_->Visit(this);
				}
			}

			if(inParams_ == 0)
			{
				Write(";");
				NextLine();
			}
		}

		return false;
	}

	void ShaderBackendHLSL::VisitExit(AST::NodeDeclaration* node) {}

	bool ShaderBackendHLSL::VisitEnter(AST::NodeValue* node)
	{
		Write("%s", node->data_.c_str());
		return false;
	}

	void ShaderBackendHLSL::VisitExit(AST::NodeValue* node) {}

	bool ShaderBackendHLSL::VisitEnter(AST::NodeValues* node) { return false; }

	void ShaderBackendHLSL::VisitExit(AST::NodeValues* node) {}

	bool ShaderBackendHLSL::VisitEnter(AST::NodeMemberValue* node) { return false; }

	void ShaderBackendHLSL::VisitExit(AST::NodeMemberValue* node) {}

	void ShaderBackendHLSL::Write(const char* msg, ...)
	{
		if(isNewLine_)
		{
			isNewLine_ = false;
			for(i32 ind = 0; ind < indent_; ++ind)
				Write("    ");
		}

		{
			va_list args;
			va_start(args, msg);
			Core::Log(msg, args);
			va_end(args);
		}

		{
			va_list args;
			va_start(args, msg);
			outCode_.Appendfv(msg, args);
			va_end(args);
		}
	}

	void ShaderBackendHLSL::NextLine()
	{
		Write("\n");
		isNewLine_ = true;
	}

	bool ShaderBackendHLSL::IsInternal(AST::Node* node, const char* internalType) const
	{
		switch(node->nodeType_)
		{
		case AST::Nodes::DECLARATION:
		{
			auto* typedNode = static_cast<AST::NodeDeclaration*>(node);
			if(auto attribNode = typedNode->FindAttribute("internal"))
			{
				if(internalType)
				{
					return (attribNode->HasParameter(0) && attribNode->GetParameter(0) == internalType);
				}
				return true;
			}

			if(typedNode->type_->baseType_->struct_)
				return IsInternal(typedNode->type_->baseType_->struct_, internalType);
		}
		break;
		case AST::Nodes::STRUCT:
		{
			auto* typedNode = static_cast<AST::NodeStruct*>(node);
			if(auto attribNode = typedNode->FindAttribute("internal"))
			{
				if(internalType)
				{
					return (attribNode->HasParameter(0) && attribNode->GetParameter(0) == internalType);
				}
				return true;
			}
		}
		break;
		default:
			return false;
		}

		return false;
	}

} /// namespace Graphics
