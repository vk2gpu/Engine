#include "math/float.h"

#define BC_FLOAT_SIGN_MASK 0x80000000
#define BC_FLOAT_EXP_MASK 0x7F800000
#define BC_FLOAT_FRAC_MASK 0x007FFFFF
#define BC_FLOAT_SNAN_MASK 0x00400000

bool CheckFloat(f32 T)
{
	f32 Copy = T;
	u32 IntFloat = *((u32*)&(Copy));
	u32 Exp = IntFloat & BC_FLOAT_EXP_MASK;
	u32 Frac = IntFloat & BC_FLOAT_FRAC_MASK;
	if ((Exp == 0) && (Frac != 0)) 
		return false;
	if (Exp == BC_FLOAT_EXP_MASK) 
		return false;
	if ((Exp == BC_FLOAT_EXP_MASK) && ((Frac & BC_FLOAT_SNAN_MASK) == 0)) 
		return false;
	return true;
}
