#include "math/float.h"


namespace Math
{
	bool CheckFloat(f32 T)
	{
		f32 Copy = T;
		u32 IntFloat = *((u32*)&(Copy));
		u32 Exp = IntFloat & F32_EXP_MASK;
		u32 Frac = IntFloat & F32_FRAC_MASK;
		if((Exp == 0) && (Frac != 0))
			return false;
		if(Exp == F32_EXP_MASK)
			return false;
		if((Exp == F32_EXP_MASK) && ((Frac & F32_SNAN_MASK) == 0))
			return false;
		return true;
	}
}
