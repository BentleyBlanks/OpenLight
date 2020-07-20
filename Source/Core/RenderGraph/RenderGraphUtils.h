#pragma once

#include<d3d12.h>
#include<iostream>
#include<variant>

#include<assert.h>


using RenderPassID = int;
using LogicalResourceID = int;
using PhysicalResourceID = int;
using DescriptorResourceID = int;


__forceinline bool IsValidID(int id)
{
	return id >= 0;
}

__forceinline int InvalidID()
{
	return -1;
}

using D3DViewDesc = std::variant<
	D3D12_CONSTANT_BUFFER_VIEW_DESC,
	D3D12_SHADER_RESOURCE_VIEW_DESC,
	D3D12_UNORDERED_ACCESS_VIEW_DESC,
	D3D12_RENDER_TARGET_VIEW_DESC,
	D3D12_DEPTH_STENCIL_VIEW_DESC>;


template<typename T>
bool bitwise_equal(const T& lhs, const T& rhs) noexcept {
	using U =
		std::conditional_t<alignof(T) == 1,
		std::uint8_t,
		std::conditional_t<alignof(T) == 2,
		std::uint16_t,
		std::conditional_t<alignof(T) == 4,
		std::uint32_t,
		std::uint64_t
		>
		>
		>;

	const U* u_lhs = reinterpret_cast<const U*>(&lhs);
	const U* u_rhs = reinterpret_cast<const U*>(&rhs);
	for (size_t i = 0; i < sizeof(T) / sizeof(U); i++, u_lhs++, u_rhs++) {
		if (*u_lhs != *u_rhs)
			return false;
	}

	return true;
}

template<typename T>
void bitwise_copy(T& dest, const T& src) noexcept
{
	assert(&dest != &src);
	std::memcpy(&dest, &src, sizeof(T));
}