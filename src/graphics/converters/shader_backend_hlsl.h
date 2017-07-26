#pragma once

#include "graphics/converters/shader_ast.h"

namespace Graphics
{
	class ShaderBackendHLSL : public AST::IVisitor
	{
	public:
		ShaderBackendHLSL();
		virtual ~ShaderBackendHLSL();
		bool VisitEnter(AST::NodeShaderFile* node) override;
		void VisitExit(AST::NodeShaderFile* node) override;
		bool VisitEnter(AST::NodeAttribute* node) override;
		void VisitExit(AST::NodeAttribute* node) override;
		bool VisitEnter(AST::NodeStorageClass* node) override;
		void VisitExit(AST::NodeStorageClass* node) override;
		bool VisitEnter(AST::NodeModifier* node) override;
		void VisitExit(AST::NodeModifier* node) override;
		bool VisitEnter(AST::NodeType* node) override;
		void VisitExit(AST::NodeType* node) override;
		bool VisitEnter(AST::NodeTypeIdent* node) override;
		void VisitExit(AST::NodeTypeIdent* node) override;
		bool VisitEnter(AST::NodeStruct* node) override;
		void VisitExit(AST::NodeStruct* node) override;
		bool VisitEnter(AST::NodeDeclaration* node) override;
		void VisitExit(AST::NodeDeclaration* node) override;
		bool VisitEnter(AST::NodeValue* node) override;
		void VisitExit(AST::NodeValue* node) override;
		bool VisitEnter(AST::NodeValues* node) override;
		void VisitExit(AST::NodeValues* node) override;
		bool VisitEnter(AST::NodeMemberValue* node) override;
		void VisitExit(AST::NodeMemberValue* node) override;

		const Core::String& GetOutputCode() const { return outCode_; }

	private:
		void Write(const char* msg, ...);
		void NextLine();
		bool IsInternal(AST::Node* node, const char* internalType = nullptr) const;

		bool isNewLine_ = false;
		i32 indent_ = 0;
		i32 inParams_ = 0;
		i32 inCBuffer_ = 0;

		i32 cbufferReg_ = 0;
		i32 samplerReg_ = 0;
		i32 srvReg_ = 0;
		i32 uavReg_ = 0;

		const char* writeInternalDeclaration_ = nullptr;

		Core::String outCode_;
	};
} // namespace Graphics
