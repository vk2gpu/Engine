#include "graphics/converters/shader_ast.h"

namespace Graphics
{
	namespace AST
	{
		void NodeShaderFile::Visit(IVisitor* visitor)
		{
			if(auto visit = ScopedVisit<NodeShaderFile>(visitor, this))
			{

				for(auto* node : structs_)
					node->Visit(visitor);
				for(auto* node : variables_)
					node->Visit(visitor);
				for(auto* node : functions_)
					node->Visit(visitor);
			}
		}

		NodeDeclaration* NodeShaderFile::FindVariable(const char* name)
		{
			for(auto* parameter : variables_)
				if(parameter->name_ == name)
					return parameter;
			return nullptr;
		}

		NodeDeclaration* NodeShaderFile::FindFunction(const char* name)
		{
			for(auto* function : functions_)
				if(function->name_ == name)
					return function;
			return nullptr;
		}

		void NodeAttribute::Visit(IVisitor* visitor) { ScopedVisit<NodeAttribute> visit(visitor, this); }

		void NodeStorageClass::Visit(IVisitor* visitor) { ScopedVisit<NodeStorageClass> visit(visitor, this); }

		void NodeModifier::Visit(IVisitor* visitor) { ScopedVisit<NodeModifier> visit(visitor, this); }

		void NodeType::Visit(IVisitor* visitor)
		{
			if(auto visit = ScopedVisit<NodeType>(visitor, this))
			{
				for(auto* node : members_)
					node->Visit(visitor);
			}
		}

		NodeDeclaration* NodeType::FindMember(const char* name) const
		{
			for(auto* member : members_)
				if(member->name_ == name)
					return member;
			return nullptr;
		}

		bool NodeType::HasEnumValue(const char* name) const
		{
			if(enumValueFn_ == nullptr)
				return false;
			i32 val = 0;
			return enumValueFn_(val, name);
		}

		const char* NodeType::FindEnumName(i32 val) const { return enumNameFn_(val); }

		void NodeTypeIdent::Visit(IVisitor* visitor)
		{
			if(auto visit = ScopedVisit<NodeTypeIdent>(visitor, this))
			{
				for(auto* node : baseModifiers_)
					node->Visit(visitor);
				baseType_->Visit(visitor);
				for(auto* node : templateModifiers_)
					node->Visit(visitor);
				if(templateType_)
					templateType_->Visit(visitor);
			}
		}

		void NodeStruct::Visit(IVisitor* visitor)
		{
			if(auto visit = ScopedVisit<NodeStruct>(visitor, this))
			{
				for(auto* node : attributes_)
					node->Visit(visitor);
				type_->Visit(visitor);
			}
		}

		NodeAttribute* NodeStruct::FindAttribute(const char* name) const
		{
			for(auto* attribute : attributes_)
				if(attribute->name_ == name)
					return attribute;
			return nullptr;
		}

		void NodeDeclaration::Visit(IVisitor* visitor)
		{
			if(auto visit = ScopedVisit<NodeDeclaration>(visitor, this))
			{
				for(auto* node : attributes_)
					node->Visit(visitor);
				for(auto* node : storageClasses_)
					node->Visit(visitor);
				type_->Visit(visitor);
				for(auto* node : parameters_)
					node->Visit(visitor);
				if(value_)
					value_->Visit(visitor);
			}
		}

		NodeAttribute* NodeDeclaration::FindAttribute(const char* name) const
		{
			for(auto* attribute : attributes_)
				if(attribute->name_ == name)
					return attribute;
			return nullptr;
		}

		void NodeValue::Visit(IVisitor* visitor) { ScopedVisit<NodeValue> visit(visitor, this); }

		void NodeValues::Visit(IVisitor* visitor)
		{
			if(auto visit = ScopedVisit<NodeValues>(visitor, this))
			{
				for(auto* node : values_)
					node->Visit(visitor);
			}
		}

		void NodeMemberValue::Visit(IVisitor* visitor)
		{
			if(auto visit = ScopedVisit<NodeMemberValue>(visitor, this))
			{
				value_->Visit(visitor);
			}
		}


	} // namespace AST
} // namespace Graphics
