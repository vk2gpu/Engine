#pragma once

#include "graphics/converters/shader_ast.h"
#include "gpu/enum.h"
#include "gpu/resources.h"

namespace Graphics
{
	struct ShaderSamplerState
	{
		Core::String name_;
		GPU::SamplerState state_;
	};

	struct ShaderRenderState
	{
		Core::String name_;
		GPU::RenderState state_;
	};

	struct ShaderTechnique
	{
		Core::String name_;
		Core::String vs_;
		Core::String gs_;
		Core::String hs_;
		Core::String ds_;
		Core::String ps_;
		Core::String cs_;
		ShaderRenderState rs_;
	};

	class ShaderBackendMetadata : public AST::IVisitor
	{
	public:
		ShaderBackendMetadata();
		virtual ~ShaderBackendMetadata();

		bool VisitEnter(AST::NodeShaderFile* node) override;
		void VisitExit(AST::NodeShaderFile* node) override;
		bool VisitEnter(AST::NodeDeclaration* node) override;

	private:
		bool IsDeclSamplerState(AST::NodeDeclaration* node) const;
		bool IsDeclRenderState(AST::NodeDeclaration* node) const;
		bool IsDeclTechnique(AST::NodeDeclaration* node) const;

		template<typename PARSE_TYPE>
		class BaseEval : public AST::IVisitor
		{
		public:
			BaseEval(PARSE_TYPE& data, AST::NodeShaderFile* file)
			    : data_(data)
			    , file_(file)
			{
			}

			bool VisitEnter(AST::NodeValue* node) override { return true; }
			bool VisitEnter(AST::NodeValues* node) override { return true; }
			bool VisitEnter(AST::NodeMemberValue* node) override
			{
				if(node->value_->type_ == AST::ValueType::IDENTIFIER)
				{
					auto it = parseFns_.find(node->member_);
					if(it != parseFns_.end())
					{
						it->second(data_, node->value_->data_);
						return false;
					}
				}
				return true;
			}


			using ParseFn = void (*)(PARSE_TYPE& data, const Core::String& value);

			PARSE_TYPE& data_;
			AST::NodeShaderFile* file_;
			Core::Map<Core::String, ParseFn> parseFns_;
		};

		class SamplerStateEval : public BaseEval<ShaderSamplerState>
		{
		public:
			SamplerStateEval(ShaderSamplerState& samp, AST::NodeShaderFile* file)
			    : BaseEval(samp, file)
			{
				parseFns_["AddressU"] = [](ShaderSamplerState& data, const Core::String& val) {
					Core::EnumFromString(data.state_.addressU_, val.c_str());
				};
				parseFns_["AddressV"] = [](ShaderSamplerState& data, const Core::String& val) {
					Core::EnumFromString(data.state_.addressV_, val.c_str());
				};
				parseFns_["AddressW"] = [](ShaderSamplerState& data, const Core::String& val) {
					Core::EnumFromString(data.state_.addressW_, val.c_str());
				};
				parseFns_["MinFilter"] = [](ShaderSamplerState& data, const Core::String& val) {
					Core::EnumFromString(data.state_.minFilter_, val.c_str());
				};
				parseFns_["MagFilter"] = [](ShaderSamplerState& data, const Core::String& val) {
					Core::EnumFromString(data.state_.magFilter_, val.c_str());
				};
				parseFns_["MipLODBias"] = [](ShaderSamplerState& data, const Core::String& val) {
					data.state_.mipLODBias_ = (f32)atof(val.c_str());
				};
				parseFns_["MaxAnisotropy"] = [](ShaderSamplerState& data, const Core::String& val) {
					data.state_.maxAnisotropy_ = atoi(val.c_str());
				};
				parseFns_["BorderColor"] = [](ShaderSamplerState& data, const Core::String& val) { DBG_BREAK; };
				parseFns_["MinLOD"] = [](ShaderSamplerState& data, const Core::String& val) {
					data.state_.minLOD_ = (f32)atof(val.c_str());
				};
				parseFns_["MaxLOD"] = [](ShaderSamplerState& data, const Core::String& val) {
					data.state_.maxLOD_ = (f32)atof(val.c_str());
				};
			}
		};

		class RenderStateEval : public BaseEval<ShaderRenderState>
		{
		public:
			RenderStateEval(ShaderRenderState& rend, AST::NodeShaderFile* file)
			    : BaseEval(rend, file)
			{
			}
		};


		class TechniqueEval : public BaseEval<ShaderTechnique>
		{
		public:
			TechniqueEval(ShaderTechnique& tech, AST::NodeShaderFile* file)
			    : BaseEval(tech, file)
			{
				parseFns_["VertexShader"] = [](ShaderTechnique& data, const Core::String& val) { data.vs_ = val; };
				parseFns_["GeometryShader"] = [](ShaderTechnique& data, const Core::String& val) { data.gs_ = val; };
				parseFns_["HullShader"] = [](ShaderTechnique& data, const Core::String& val) { data.hs_ = val; };
				parseFns_["DomainShader"] = [](ShaderTechnique& data, const Core::String& val) { data.ds_ = val; };
				parseFns_["PixelShader"] = [](ShaderTechnique& data, const Core::String& val) { data.ps_ = val; };
				parseFns_["ComputeShader"] = [](ShaderTechnique& data, const Core::String& val) { data.cs_ = val; };
			}
		};

		AST::NodeShaderFile* file_ = nullptr;
		Core::Vector<ShaderSamplerState> samplerStates_;
		Core::Vector<ShaderRenderState> renderStates_;
		Core::Vector<ShaderTechnique> techniques_;

		Core::String outCode_;
	};
} // namespace Graphics
