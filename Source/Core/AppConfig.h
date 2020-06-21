#pragma once

#include <iostream>
#include <DirectXMath.h>
namespace AppConfig
{
	static const uint8_t NumFrames = 3;
	static uint32_t ClientWidth    = 1280;
	static uint32_t ClientHeight   = 720;

	static bool UseWarp          = false;
	static bool VSync            = false;
	static bool TearingSupported = false;
	static bool Fullscreen       = false;

	static const char* GlobalFresnelMaterialName[] = {
	"Water",
	"PlasticLow",
	"PlasticHigh",
	"Glass",
	"Diamond",
	"Iron",
	"Copper",
	"Gold",
	"Aluminum",
	"Silver",
	};
	// Linear RGB
	const DirectX::XMFLOAT4 GlobalFresnelMaterial[] = { DirectX::XMFLOAT4(0.02f,0.02f,0.02f,1.f),
										DirectX::XMFLOAT4(0.03f,0.03f,0.03f,1.f),
										DirectX::XMFLOAT4(0.05f,0.05f,0.05f,1.f),
										DirectX::XMFLOAT4(0.08f,0.08f,0.08f,1.f),
										DirectX::XMFLOAT4(0.17f,0.17f,0.17f,1.f),
										DirectX::XMFLOAT4(0.56f,0.57f,0.58f,1.f),
										DirectX::XMFLOAT4(0.95f,0.64f,0.54f,1.f),
										DirectX::XMFLOAT4(1.00f,0.71f,0.29f,1.f),
										DirectX::XMFLOAT4(0.91f,0.92f,0.92f,1.f),
										DirectX::XMFLOAT4(0.95F,0.93F,0.88F,1.f) };
};