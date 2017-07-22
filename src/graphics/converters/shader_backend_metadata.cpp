#include "graphics/converters/shader_backend_metadata.h"

#include <cstdarg>

namespace Graphics
{
	ShaderBackendMetadata::SamplerStateEval::SamplerStateEval(
	    ShaderSamplerState& samp, AST::NodeShaderFile* file, ShaderBackendMetadata& backend)
	    : BaseEval(samp, file, backend)
	{
		parseFns_["AddressU"] = [](ShaderSamplerState& data, AST::NodeValue* node) {
			Core::EnumFromString(data.state_.addressU_, node->data_.c_str());
		};
		parseFns_["AddressV"] = [](ShaderSamplerState& data, AST::NodeValue* node) {
			Core::EnumFromString(data.state_.addressV_, node->data_.c_str());
		};
		parseFns_["AddressW"] = [](ShaderSamplerState& data, AST::NodeValue* node) {
			Core::EnumFromString(data.state_.addressW_, node->data_.c_str());
		};
		parseFns_["MinFilter"] = [](ShaderSamplerState& data, AST::NodeValue* node) {
			Core::EnumFromString(data.state_.minFilter_, node->data_.c_str());
		};
		parseFns_["MagFilter"] = [](ShaderSamplerState& data, AST::NodeValue* node) {
			Core::EnumFromString(data.state_.magFilter_, node->data_.c_str());
		};
		parseFns_["MipLODBias"] = [](
		    ShaderSamplerState& data, AST::NodeValue* node) { data.state_.mipLODBias_ = node->dataFloat_; };
		parseFns_["MaxAnisotropy"] = [](
		    ShaderSamplerState& data, AST::NodeValue* node) { data.state_.maxAnisotropy_ = (u32)node->dataInt_; };
		parseFns_["BorderColor"] = [](ShaderSamplerState& data, AST::NodeValue* node) { DBG_BREAK; };
		parseFns_["MinLOD"] = [](
		    ShaderSamplerState& data, AST::NodeValue* node) { data.state_.minLOD_ = node->dataFloat_; };
		parseFns_["MaxLOD"] = [](
		    ShaderSamplerState& data, AST::NodeValue* node) { data.state_.maxLOD_ = node->dataFloat_; };
	}


	ShaderBackendMetadata::BlendStateEval::BlendStateEval(
	    ShaderBlendState& blend, AST::NodeShaderFile* file, ShaderBackendMetadata& backend)
	    : BaseEval(blend, file, backend)
	{
		parseFns_["Enable"] = [](
		    ShaderBlendState& data, AST::NodeValue* node) { data.state_.enable_ = (u32)node->dataInt_; };
		parseFns_["SrcBlend"] = [](ShaderBlendState& data, AST::NodeValue* node) {
			Core::EnumFromString(data.state_.srcBlend_, node->data_.c_str());
		};
		parseFns_["DestBlend"] = [](ShaderBlendState& data, AST::NodeValue* node) {
			Core::EnumFromString(data.state_.destBlend_, node->data_.c_str());
		};
		parseFns_["BlendOp"] = [](ShaderBlendState& data, AST::NodeValue* node) {
			Core::EnumFromString(data.state_.blendOp_, node->data_.c_str());
		};
		parseFns_["SrcBlendAlpha"] = [](ShaderBlendState& data, AST::NodeValue* node) {
			Core::EnumFromString(data.state_.srcBlendAlpha_, node->data_.c_str());
		};
		parseFns_["DestBlendAlpha"] = [](ShaderBlendState& data, AST::NodeValue* node) {
			Core::EnumFromString(data.state_.destBlendAlpha_, node->data_.c_str());
		};
		parseFns_["BlendOpAlpha"] = [](ShaderBlendState& data, AST::NodeValue* node) {
			Core::EnumFromString(data.state_.blendOpAlpha_, node->data_.c_str());
		};
		parseFns_["WriteMask"] = [](
		    ShaderBlendState& data, AST::NodeValue* node) { data.state_.writeMask_ = (u8)node->dataInt_; };
	}

	ShaderBackendMetadata::StencilFaceStateEval::StencilFaceStateEval(
	    ShaderStencilFaceState& sten, AST::NodeShaderFile* file, ShaderBackendMetadata& backend)
	    : BaseEval(sten, file, backend)
	{
		parseFns_["Fail"] = [](ShaderStencilFaceState& data, AST::NodeValue* node) {
			Core::EnumFromString(data.state_.fail_, node->data_.c_str());
		};
		parseFns_["DepthFail"] = [](ShaderStencilFaceState& data, AST::NodeValue* node) {
			Core::EnumFromString(data.state_.depthFail_, node->data_.c_str());
		};
		parseFns_["Pass"] = [](ShaderStencilFaceState& data, AST::NodeValue* node) {
			Core::EnumFromString(data.state_.pass_, node->data_.c_str());
		};
		parseFns_["Func"] = [](ShaderStencilFaceState& data, AST::NodeValue* node) {
			Core::EnumFromString(data.state_.func_, node->data_.c_str());
		};
	}


	ShaderBackendMetadata::RenderStateEval::RenderStateEval(
	    ShaderRenderState& rend, AST::NodeShaderFile* file, ShaderBackendMetadata& backend)
	    : BaseEval(rend, file, backend)
	{
		parseFns_["DepthEnable"] = [](
		    ShaderRenderState& data, AST::NodeValue* node) { data.state_.depthEnable_ = (u32)node->dataInt_; };
		parseFns_["DepthWriteMask"] = [](
		    ShaderRenderState& data, AST::NodeValue* node) { data.state_.depthWriteMask_ = (u32)node->dataInt_; };
		parseFns_["DepthFunc"] = [](ShaderRenderState& data, AST::NodeValue* node) {
			Core::EnumFromString(data.state_.depthFunc_, node->data_.c_str());
		};
		parseFns_["StencilEnable"] = [](
		    ShaderRenderState& data, AST::NodeValue* node) { data.state_.stencilEnable_ = (u32)node->dataInt_; };
		parseFns_["StencilRef"] = [](
		    ShaderRenderState& data, AST::NodeValue* node) { data.state_.stencilRef_ = (u32)node->dataInt_; };
		parseFns_["StencilRead"] = [](
		    ShaderRenderState& data, AST::NodeValue* node) { data.state_.stencilRead_ = (u8)node->dataInt_; };
		parseFns_["StencilWrite"] = [](
		    ShaderRenderState& data, AST::NodeValue* node) { data.state_.stencilWrite_ = (u8)node->dataInt_; };
		parseFns_["FillMode"] = [](ShaderRenderState& data, AST::NodeValue* node) {
			Core::EnumFromString(data.state_.fillMode_, node->data_.c_str());
		};
		parseFns_["CullMode"] = [](ShaderRenderState& data, AST::NodeValue* node) {
			Core::EnumFromString(data.state_.cullMode_, node->data_.c_str());
		};
		parseFns_["DepthBias"] = [](
		    ShaderRenderState& data, AST::NodeValue* node) { data.state_.depthBias_ = node->dataFloat_; };
		parseFns_["SlopeScaledDepthBias"] = [](
		    ShaderRenderState& data, AST::NodeValue* node) { data.state_.depthBias_ = node->dataFloat_; };
		parseFns_["AntialiasedLineEnable"] = [](ShaderRenderState& data, AST::NodeValue* node) {
			data.state_.antialiasedLineEnable_ = (u32)node->dataInt_;
		};


		descendFns_["BlendStates"] = [](
		    ShaderRenderState& data, AST::NodeShaderFile* file, ShaderBackendMetadata& backend, AST::NodeValue* node) {
			if(node->type_ == AST::ValueType::IDENTIFIER)
			{
				for(const auto& blend : backend.blendStates_)
					if(blend.name_ == node->data_)
					{
						// TODO: ARRAY.
						data.state_.blendStates_[0] = blend.state_;
						break;
					}
			}
			else
			{
				// descend into BlendState.
				ShaderBlendState blend;
				BlendStateEval eval(blend, file, backend);
				node->Visit(&eval);
				data.state_.blendStates_[0] = blend.state_;
			}
		};

		descendFns_["StencilFront"] = [](
		    ShaderRenderState& data, AST::NodeShaderFile* file, ShaderBackendMetadata& backend, AST::NodeValue* node) {
			if(node->type_ == AST::ValueType::IDENTIFIER)
			{
				for(const auto& sten : backend.stencilFaceStates_)
					if(sten.name_ == node->data_)
					{
						data.state_.stencilFront_ = sten.state_;
						break;
					}
			}
			else
			{
				// descend into StencilFaceState.
				ShaderStencilFaceState sten;
				StencilFaceStateEval eval(sten, file, backend);
				node->Visit(&eval);
				data.state_.stencilFront_ = sten.state_;
			}
		};

		descendFns_["StencilBack"] = [](
		    ShaderRenderState& data, AST::NodeShaderFile* file, ShaderBackendMetadata& backend, AST::NodeValue* node) {
			if(node->type_ == AST::ValueType::IDENTIFIER)
			{
				for(const auto& sten : backend.stencilFaceStates_)
					if(sten.name_ == node->data_)
					{
						data.state_.stencilBack_ = sten.state_;
						break;
					}
			}
			else
			{
				// descend into StencilFaceState.
				ShaderStencilFaceState sten;
				StencilFaceStateEval eval(sten, file, backend);
				node->Visit(&eval);
				data.state_.stencilBack_ = sten.state_;
			}
		};
	}

	ShaderBackendMetadata::TechniqueEval::TechniqueEval(
	    ShaderTechnique& tech, AST::NodeShaderFile* file, ShaderBackendMetadata& backend)
	    : BaseEval(tech, file, backend)
	{
		parseFns_["VertexShader"] = [](ShaderTechnique& data, AST::NodeValue* node) { data.vs_ = node->data_; };
		parseFns_["GeometryShader"] = [](ShaderTechnique& data, AST::NodeValue* node) { data.gs_ = node->data_; };
		parseFns_["HullShader"] = [](ShaderTechnique& data, AST::NodeValue* node) { data.hs_ = node->data_; };
		parseFns_["DomainShader"] = [](ShaderTechnique& data, AST::NodeValue* node) { data.ds_ = node->data_; };
		parseFns_["PixelShader"] = [](ShaderTechnique& data, AST::NodeValue* node) { data.ps_ = node->data_; };
		parseFns_["ComputeShader"] = [](ShaderTechnique& data, AST::NodeValue* node) { data.cs_ = node->data_; };

		descendFns_["RenderState"] = [](
		    ShaderTechnique& data, AST::NodeShaderFile* file, ShaderBackendMetadata& backend, AST::NodeValue* node) {
			if(node->type_ == AST::ValueType::IDENTIFIER)
			{
				for(const auto& rend : backend.renderStates_)
					if(rend.name_ == node->data_)
					{
						data.rs_ = rend;
						break;
					}
			}
			else
			{
				// descend into RenderState.
				RenderStateEval eval(data.rs_, file, backend);
				node->Visit(&eval);
			}
		};
	}


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

			if(node->value_)
			{
				SamplerStateEval eval(samp, file_, *this);
				node->value_->Visit(&eval);
				samplerStates_.push_back(samp);
			}
		}
		else if(IsDeclRenderState(node))
		{
			ShaderRenderState rend;
			rend.name_ = node->name_;

			if(node->value_)
			{
				RenderStateEval eval(rend, file_, *this);
				node->value_->Visit(&eval);
				renderStates_.push_back(rend);
			}
		}
		else if(IsDeclTechnique(node))
		{
			ShaderTechnique tech;
			tech.name_ = node->name_;

			if(node->value_)
			{
				TechniqueEval eval(tech, file_, *this);
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

	bool ShaderBackendMetadata::IsDeclBlendState(AST::NodeDeclaration* node) const
	{
		auto* nodeStruct = node->type_->baseType_->struct_;
		if(nodeStruct)
			if(auto attrib = nodeStruct->FindAttribute("internal"))
				if(attrib->HasParameter(0))
					return attrib->GetParameter(0) == "BlendState";
		return false;
	}

	bool ShaderBackendMetadata::IsDeclStencilFaceState(AST::NodeDeclaration* node) const
	{
		auto* nodeStruct = node->type_->baseType_->struct_;
		if(nodeStruct)
			if(auto attrib = nodeStruct->FindAttribute("internal"))
				if(attrib->HasParameter(0))
					return attrib->GetParameter(0) == "StencilFaceState";
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
