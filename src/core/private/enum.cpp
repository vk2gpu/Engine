#include "core/enum.h"

#include <cstring>

namespace Core
{
	bool EnumFromString(i32& outVal, const char* str, const char*(*convFn)(i32))
	{
		for(i32 i = 0; ; ++i)
		{
			const char* valStr = convFn(i);
			// Hit end of enum.
			if(valStr == nullptr)
				break;

			if(_stricmp(valStr, str) == 0)
			{
				outVal = i;
				return true;
			}
		}
		return false;
	}

} // namespace Core