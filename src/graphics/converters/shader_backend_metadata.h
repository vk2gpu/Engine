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

	struct ShaderBlendState
	{
		Core::String name_;
		GPU::BlendState state_;
	};

	struct ShaderStencilFaceState
	{
		Core::String name_;
		GPU::StencilFaceState state_;
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
		bool VisitEnter(AST::NodeStruct* node) override { return false; }
		bool VisitEnter(AST::NodeDeclaration* node) override;

	private:
		bool IsDeclSamplerState(AST::NodeDeclaration* node) const;
		bool IsDeclBlendState(AST::NodeDeclaration* node) const;
		bool IsDeclStencilFaceState(AST::NodeDeclaration* node) const;
		bool IsDeclRenderState(AST::NodeDeclaration* node) const;
		bool IsDeclTechnique(AST::NodeDeclaration* node) const;

		template<typename PARSE_TYPE>
		class BaseEval : public AST::IVisitor
		{
		public:
			BaseEval(PARSE_TYPE& data, AST::NodeShaderFile* file, ShaderBackendMetadata& backend)
			    : data_(data)
			    , file_(file)
			    , backend_(backend)
			{
			}

			bool VisitEnter(AST::NodeValue* node) override { return true; }
			bool VisitEnter(AST::NodeValues* node) override { return true; }
			bool VisitEnter(AST::NodeMemberValue* node) override
			{
				//if(node->value_->type_ != AST::ValueType::ARRAY || node->value_->type_ == AST::ValueType::ENUM)
				{
					{
						auto it = parseFns_.find(node->member_);
						if(it != parseFns_.end())
						{
							it->second(data_, node->value_);
							return false;
						}
					}
					{
						auto it = descendFns_.find(node->member_);
						if(it != descendFns_.end())
						{
							it->second(data_, file_, backend_, node->value_);
							return false;
						}
					}
				}
				return true;
			}


			using ParseFn = void (*)(PARSE_TYPE& data, AST::NodeValue* node);
			using DescendFn = void (*)(
			    PARSE_TYPE& data, AST::NodeShaderFile* file, ShaderBackendMetadata& backend, AST::NodeValue* node);

			PARSE_TYPE& data_;
			AST::NodeShaderFile* file_;
			ShaderBackendMetadata& backend_;
			Core::Map<Core::String, ParseFn> parseFns_;
			Core::Map<Core::String, DescendFn> descendFns_;
		};

		class SamplerStateEval : public BaseEval<ShaderSamplerState>
		{
		public:
			SamplerStateEval(ShaderSamplerState& samp, AST::NodeShaderFile* file, ShaderBackendMetadata& backend);
		};

		class BlendStateEval : public BaseEval<ShaderBlendState>
		{
		public:
			BlendStateEval(ShaderBlendState& blend, AST::NodeShaderFile* file, ShaderBackendMetadata& backend);
		};

		class StencilFaceStateEval : public BaseEval<ShaderStencilFaceState>
		{
		public:
			StencilFaceStateEval(
			    ShaderStencilFaceState& sten, AST::NodeShaderFile* file, ShaderBackendMetadata& backend);
		};
		class RenderStateEval : public BaseEval<ShaderRenderState>
		{
		public:
			RenderStateEval(ShaderRenderState& rend, AST::NodeShaderFile* file, ShaderBackendMetadata& backend);
		};

		class TechniqueEval : public BaseEval<ShaderTechnique>
		{
		public:
			TechniqueEval(ShaderTechnique& tech, AST::NodeShaderFile* file, ShaderBackendMetadata& backend);
		};

		AST::NodeShaderFile* file_ = nullptr;
		Core::Vector<ShaderSamplerState> samplerStates_;
		Core::Vector<ShaderBlendState> blendStates_;
		Core::Vector<ShaderStencilFaceState> stencilFaceStates_;
		Core::Vector<ShaderRenderState> renderStates_;
		Core::Vector<ShaderTechnique> techniques_;

		Core::String outCode_;
	};
} // namespace Graphics
