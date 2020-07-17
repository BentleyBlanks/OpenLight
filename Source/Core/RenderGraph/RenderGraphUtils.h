#pragma once

#include<d3d12.h>
#include<iostream>
#include<variant>

struct RenderPassID
{
	RenderPassID() {}
	explicit RenderPassID(int _id) :id(_id) {}
	explicit RenderPassID(size_t _id) : id(_id) {}

	bool operator==(const RenderPassID& rhs) const
	{
		return id == rhs.id;
	}

	bool IsValid() const {
		return id >= 0;
	}
	
protected:
	int id = -1;
};

struct LogicalResourceID
{
	LogicalResourceID() {}
	explicit LogicalResourceID(int _id) :id(_id) {}
	explicit LogicalResourceID(size_t _id) : id(_id) {}

	bool operator==(const LogicalResourceID& rhs) const
	{
		return id == rhs.id;
	}

	bool IsValid() const {
		return id >= 0;
	}

protected:
	int id = -1;
};

struct PhysicalResourceID
{
	PhysicalResourceID() {}
	explicit PhysicalResourceID(int _id) :id(_id) {}
	explicit PhysicalResourceID(size_t _id) : id(_id) {}

	bool operator==(const PhysicalResourceID& rhs) const
	{
		return id == rhs.id;
	}

	bool IsValid() const {
		return id >= 0;
	}

protected:
	int id = -1;
};

struct DescriptorResourceID
{
	DescriptorResourceID() {}
	explicit DescriptorResourceID(int _id) :id(_id) {}
	explicit DescriptorResourceID(size_t _id) : id(_id) {}

	bool operator==(const DescriptorResourceID& rhs) const
	{
		return id == rhs.id;
	}

	bool IsValid() const {
		return id >= 0;
	}

protected:
	int id = -1;
};

using D3DViewDesc = std::variant<
	D3D12_CONSTANT_BUFFER_VIEW_DESC,
	D3D12_SHADER_RESOURCE_VIEW_DESC,
	D3D12_UNORDERED_ACCESS_VIEW_DESC,
	D3D12_RENDER_TARGET_VIEW_DESC,
	D3D12_DEPTH_STENCIL_VIEW_DESC>;


