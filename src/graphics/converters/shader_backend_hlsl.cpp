#include "graphics/converters/shader_backend_hlsl.h"

#include <cstdarg>

namespace
{
	// clang-format off
	const char* HLSL_ATTRIBUTES[] = 
	{
		"domain",
		"earlydepthstencil",
		"instance",
		"maxtessfactor",
		"numthreads",
		"outputcontrolpoints",
		"outputtopology",
		"partitioning",
		"patchconstantfunc",
	};
	// clang-format on
}

namespace Graphics
{
	ShaderBackendHLSL::ShaderBackendHLSL(const BindingMap& bindingMap, bool autoReg)
	    : bindingMap_(bindingMap)
	    , autoReg_(autoReg)
	{
		for(const char* attribute : HLSL_ATTRIBUTES)
			hlslAttributes_.insert(attribute);
	}

	ShaderBackendHLSL::~ShaderBackendHLSL() {}

	bool ShaderBackendHLSL::VisitEnter(AST::NodeShaderFile* node)
	{
		// Collect all types that we need to export to HLSL.
		for(auto* intNode : node->structs_)
			intNode->Visit(this);

		for(auto* intNode : node->variables_)
			intNode->Visit(this);


		// Write out generated shader.
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

		for(auto* intNode : structs_)
			WriteStruct(intNode);
		NextLine();

		Write("////////////////////////////////////////////////////////////////////////////////////////////////////");
		NextLine();
		Write("// variables");
		NextLine();

		for(auto* intNode : variables_)
			WriteVariable(intNode);
		NextLine();

		Write("////////////////////////////////////////////////////////////////////////////////////////////////////");
		NextLine();
		Write("// sampler states");
		NextLine();

		for(auto* intNode : samplerStates_)
			if(bindingMap_.size() == 0 || bindingMap_.find(intNode->name_) != bindingMap_.end())
				WriteVariable(intNode);
		NextLine();

		Write("////////////////////////////////////////////////////////////////////////////////////////////////////");
		NextLine();
		Write("// binding sets");
		NextLine();

		for(auto* intNode : bindingSets_)
			WriteBindingSet(intNode);
		NextLine();

		Write("////////////////////////////////////////////////////////////////////////////////////////////////////");
		NextLine();
		Write("// functions");
		NextLine();

		for(auto* intNode : node->functions_)
			WriteFunction(intNode);
		NextLine();

		return false;
	}

	void ShaderBackendHLSL::VisitExit(AST::NodeShaderFile* node) {}

	bool ShaderBackendHLSL::VisitEnter(AST::NodeAttribute* node)
	{
		if(hlslAttributes_.find(node->name_) != hlslAttributes_.end())
		{
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
		if(node->typeName_ == "struct" && !IsInternal(node))
		{
			structs_.push_back(node);
		}
		else if(node->typeName_ == "BindingSet")
		{
			bindingSets_.push_back(node);
		}

		return false;
	}

	void ShaderBackendHLSL::VisitExit(AST::NodeStruct* node) {}

	bool ShaderBackendHLSL::VisitEnter(AST::NodeDeclaration* node)
	{
		if(node->isFunction_)
		{
			functions_.push_back(node);
		}
		else
		{
			bool isSampler = false;
			if(node->type_->baseType_->struct_)
			{
				if(auto attr = node->type_->baseType_->struct_->FindAttribute("internal"))
				{
					isSampler = attr->HasParameter(0) && attr->GetParameter(0) == "SamplerState";
				}
			}

			if(isSampler)
				samplerStates_.push_back(node);
			else
				variables_.push_back(node);
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

#if 0
		{
			va_list args;
			va_start(args, msg);
			Core::LogV(msg, args);
			va_end(args);
		}
#endif

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

	void ShaderBackendHLSL::WriteStruct(AST::NodeStruct* node)
	{
		for(auto* attrib : node->attributes_)
			attrib->Visit(this);

		Write("%s %s", "struct", node->name_.c_str());
		NextLine();
		Write("{");
		NextLine();

		++indent_;

		for(auto* member : node->type_->members_)
		{
			WriteParameter(member);

			Write(";");
			NextLine();
		}

		--indent_;

		Write("};");
		NextLine();
		NextLine();
	}

	void ShaderBackendHLSL::WriteBindingSet(AST::NodeStruct* node)
	{
		Write("// - %s", node->name_.c_str());
		NextLine();

		bool writeBindingSet = false;
		for(auto* member : node->type_->members_)
			writeBindingSet |= (bindingMap_.size() == 0 || bindingMap_.find(member->name_) != bindingMap_.end());

		if(writeBindingSet)
		{
			for(auto* member : node->type_->members_)
				WriteVariable(member);

			usedBindingSets_.insert(node->name_);
		}

		NextLine();
	}

	void ShaderBackendHLSL::WriteFunction(AST::NodeDeclaration* node)
	{
		for(auto* attrib : node->attributes_)
			attrib->Visit(this);

		for(auto* storageClass : node->storageClasses_)
			Write("%s ", storageClass->name_.c_str());

		node->type_->Visit(this);
		Write(" %s", node->name_.c_str());

		Write("(");
		const i32 num = node->parameters_.size();
		for(i32 idx = 0; idx < num; ++idx)
		{
			auto* param = node->parameters_[idx];
			WriteParameter(param);
			if(idx < (num - 1))
				Write(", ");
		}
		Write(")");

		if(node->semantic_.size() > 0)
			Write(" : %s", node->semantic_.c_str());

		if(node->value_)
		{
			NextLine();
			if(node->line_ >= 0)
				Write("#line %i %s\n", node->line_, node->file_.c_str());

			Write("{");
			node->value_->Visit(this);
			NextLine();
			Write("}");
			NextLine();
		}
		else
		{
			Write(";");
			NextLine();

			if(node->line_ >= 0)
				Write("#line %i %s\n", node->line_, node->file_.c_str());
		}
	}

	void ShaderBackendHLSL::WriteVariable(AST::NodeDeclaration* node)
	{
		bool isSamplerState = false;
		if(node->type_->baseType_->struct_)
		{
			if(auto attr = node->type_->baseType_->struct_->FindAttribute("internal"))
			{
				isSamplerState = attr->HasParameter(0) && attr->GetParameter(0) == "SamplerState";

				if(isSamplerState == false)
					return;
			}
		}

		for(auto* storageClass : node->storageClasses_)
			Write("%s ", storageClass->name_.c_str());

		node->type_->Visit(this);
		Write(" %s", node->name_.c_str());

		for(auto arrayDim : node->arrayDims_)
		{
			if(arrayDim > 0)
				Write("[%u]", arrayDim);
			else if(arrayDim < 0)
				Write("[]");
		}

		// Determine register.
		Core::String reg;
		if(auto attrib = node->FindAttribute("register"))
		{
			if(attrib->HasParameter(0))
				node->register_ = attrib->GetParameter(0);
			if(attrib->HasParameter(1))
				node->space_ = attrib->GetParameter(1);
		}

		if(node->register_.size() > 0)
			if(node->space_.size() == 0)
				reg.Printf("register(%s)", node->register_.c_str());
			else
				reg.Printf("register(%s, %s)", node->register_.c_str(), node->space_.c_str());

		else if(autoReg_)
			if(node->type_->baseType_->metaData_ == "SRV")
				reg.Printf("register(t%i)", srvReg_++);
			else if(node->type_->baseType_->metaData_ == "UAV")
				reg.Printf("register(u%i)", uavReg_++);
			else if(node->type_->baseType_->metaData_ == "CBV")
				reg.Printf("register(b%i)", cbufferReg_++);
			else if(isSamplerState)
				reg.Printf("register(s%i)", samplerReg_++);

		if(reg.size() > 0)
			Write(" : %s", reg.c_str());

		if(node->value_)
		{
			// Only output simple values.
			if(node->value_->nodeType_ == AST::Nodes::VALUE)
			{
				Write(" = ");
				node->value_->Visit(this);
			}
		}

		Write(";");
		NextLine();
	}

	void ShaderBackendHLSL::WriteParameter(AST::NodeDeclaration* node)
	{
		for(auto* storageClass : node->storageClasses_)
			Write("%s ", storageClass->name_.c_str());

		node->type_->Visit(this);
		Write(" %s", node->name_.c_str());

		for(auto arrayDim : node->arrayDims_)
		{
			if(arrayDim > 0)
				Write("[%u]", arrayDim);
			else if(arrayDim < 0)
				Write("[]");
		}

		if(node->semantic_.size() > 0)
			Write(" : %s", node->semantic_.c_str());
	}

} /// namespace Graphics
