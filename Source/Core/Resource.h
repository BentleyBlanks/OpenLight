#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <string.h>

class Resource
{
public:
	Resource(const std::wstring& name = L"");
	Resource(const D3D12_RESOURCE_DESC& resourceDesc,
			 const D3D12_CLEAR_VALUE* clearValue = nullptr,
			 const std::wstring& name = L"");

	Resource(ID3D12Resource* resource, const std::wstring& name = L"");
	Resource(const Resource& other);
	Resource(Resource&& other);

	Resource& operator=(const Resource& other);
	Resource& operator=(Resource&& other);
	
	virtual ~Resource();

	bool IsValid() const
	{
		return m_d3d12Resource != nullptr;
	}

	ID3D12Resource* GetD3D12Resource() const
	{
		return m_d3d12Resource;
	}

	D3D12_RESOURCE_DESC GetD3D12ResourceDesc() const
	{
		D3D12_RESOURCE_DESC resDesc = {};
		if (m_d3d12Resource)
		{
			resDesc = m_d3d12Resource->GetDesc();
		}
		return resDesc;
	}

	virtual void SetD3D12Resource(ID3D12Resource* d3d12Resource, const D3D12_CLEAR_VALUE* clearValue = nullptr);

	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const = 0;

	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const = 0;

	void SetName(const std::wstring& name);

	virtual void Reset();

protected:
	ID3D12Resource* m_d3d12Resource;
	std::unique_ptr<D3D12_CLEAR_VALUE> m_d3dClearValue;
	std::wstring m_ResourceName;
};