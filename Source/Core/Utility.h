#pragma once

#include <Windows.h>
#include <algorithm>

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))	throw std::exception();
}

