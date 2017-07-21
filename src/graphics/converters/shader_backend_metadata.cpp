#include "graphics/converters/shader_backend_metadata.h"

#include <cstdarg>

namespace Graphics
{
	ShaderBackendMetadata::ShaderBackendMetadata() {}

	ShaderBackendMetadata::~ShaderBackendMetadata() {}

	bool ShaderBackendMetadata::VisitEnter(AST::NodeShaderFile* node)
	{
		file_ = node;
		return true;
	}

	void ShaderBackendMetadata::VisitExit(AST::NodeShaderFile* node) { file_ = nullptr; }

	bool ShaderBackendMetadata::VisitEnter(AST::NodeDeclaration* node)
	{
		if(IsDeclSamplerState(node))
		{
			ShaderSamplerState samp;
			samp.name_ = node->name_;

			samplerStates_.push_back(samp);
		}
		else if(IsDeclRenderState(node))
		{
			ShaderRenderState rend;
			rend.name_ = node->name_;

			renderStates_.push_back(rend);
		}
		else if(IsDeclTechnique(node))
		{
			ShaderTechnique tech;
			tech.name_ = node->name_;

			if(node->value_)
			{
				TechniqueEval eval(tech, file_);
				node->value_->Visit(&eval);
				techniques_.push_back(tech);
			}
		}

		return false;
	}

	bool ShaderBackendMetadata::IsDeclSamplerState(AST::NodeDeclaration* node) const
	{
		auto* nodeStruct = node->type_->baseType_->struct_;
		if(nodeStruct)
			if(auto attrib = nodeStruct->FindAttribute("internal"))
				if(attrib->HasParameter(0))
					return attrib->GetParameter(0) == "SamplerState";
		return false;
	}

	bool ShaderBackendMetadata::IsDeclRenderState(AST::NodeDeclaration* node) const
	{
		auto* nodeStruct = node->type_->baseType_->struct_;
		if(nodeStruct)
			if(auto attrib = nodeStruct->FindAttribute("internal"))
				if(attrib->HasParameter(0))
					return attrib->GetParameter(0) == "RenderState";
		return false;
	}

	bool ShaderBackendMetadata::IsDeclTechnique(AST::NodeDeclaration* node) const
	{
		auto* nodeStruct = node->type_->baseType_->struct_;
		if(nodeStruct)
			if(auto attrib = nodeStruct->FindAttribute("internal"))
				if(attrib->HasParameter(0))
					return attrib->GetParameter(0) == "Technique";
		return false;
	}

} /// namespace Graphics
