#pragma once

#include <d3d12.h>

class Shader
{
public:
	Shader(ID3D12Device2* Device , const char* path , const char* vsEntry , const char* psEntry);

	ID3D12RootSignature* GetRootSignature() const;
	ID3D12PipelineState* GetPipelineState() const;

private:
	ID3D12RootSignature* mRootSignature = nullptr;
	ID3D12PipelineState* mPipelineState = nullptr;
};
