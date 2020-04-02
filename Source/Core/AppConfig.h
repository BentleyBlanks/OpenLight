#pragma once

#include <iostream>

namespace AppConfig
{
	static const uint8_t NumFrames = 3;
	static uint32_t ClientWidth    = 1280;
	static uint32_t ClientHeight   = 720;

	static bool UseWarp          = false;
	static bool VSync            = false;
	static bool TearingSupported = false;
	static bool Fullscreen       = false;
};