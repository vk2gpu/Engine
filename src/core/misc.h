#pragma once

template<typename TYPE>
TYPE Min(TYPE a, TYPE b)
{
	return a < b ? a : b;
}

template<typename TYPE>
TYPE Max(TYPE a, TYPE b)
{
	return a > b ? a : b;
}
