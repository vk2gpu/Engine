#include "graphics/converters/shader_ast.h"

namespace Graphics
{
	namespace AST
	{
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

		NodeAttribute* NodeStruct::FindAttribute(const char* name) const
		{
			for(auto* attribute : attributes_)
				if(attribute->name_ == name)
					return attribute;
			return nullptr;
		}

		NodeAttribute* NodeDeclaration::FindAttribute(const char* name) const
		{
			for(auto* attribute : attributes_)
				if(attribute->name_ == name)
					return attribute;
			return nullptr;
		}
	} // namespace AST
} // namespace Graphics
