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

	} // namespace AST
} // namespace Graphics
