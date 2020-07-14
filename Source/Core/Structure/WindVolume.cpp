#include "WindVolume.h"

WindVolume::WindVolume(ID3D12Device5* device)
{
	mDevice                          = device;
	mCBMotors.directionalMotorsCount = 0;
	mCBWindVolumeInfo.windVolumePos  = XMFLOAT3(0.f, 0.f, 0.f);
	mCBWindVolumeInfo.diff           = 1.f;
	mCBWindVolumeInfo.volumeScale    = 1.f;
	mCBWindVolumeInfo.invVolumeScale = 1.f;
	mCBWindVolumeInfo.deltaT         = 0.f;

	// Apply Motor Part
	{
		// 创建 Root Signature
		CD3DX12_DESCRIPTOR_RANGE1 descRanges[1];
		descRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 2, 0);
		CD3DX12_ROOT_PARAMETER1 rootParams[2];
		rootParams[0].InitAsDescriptorTable(1, &descRanges[0], D3D12_SHADER_VISIBILITY_ALL);
		rootParams[1].InitAsUnorderedAccessView(0, 0);

		// 根签名描述
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParams), rootParams,
			0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		// 创建根签名
		WRL::ComPtr<ID3DBlob> pSignatureBlob = nullptr;
		WRL::ComPtr<ID3DBlob> pErrorBlob = nullptr;
		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(
			&rootSignatureDesc,
			D3D_ROOT_SIGNATURE_VERSION_1_1,
			&pSignatureBlob,
			&pErrorBlob));
		ThrowIfFailed(mDevice->CreateRootSignature(0,
			pSignatureBlob->GetBufferPointer(),
			pSignatureBlob->GetBufferSize(),
			IID_PPV_ARGS(&mApplyMotorsRootSignature)));

		std::vector<BYTE> csBlob;
		csBlob = RawMemoryParser::readData("F:\\OpenLight\\Shader\\WindSimulation\\ApplyMotorCS.cso");

		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {  };
		psoDesc.pRootSignature = mApplyMotorsRootSignature;
		psoDesc.CachedPSO = {};
		psoDesc.CS.pShaderBytecode = &csBlob[0];
		psoDesc.CS.BytecodeLength = csBlob.size();
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		psoDesc.NodeMask = 0;
		ThrowIfFailed(mDevice->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&mApplyMotorsPSO)));
	}

	// Diffusion Part
	{
		CD3DX12_DESCRIPTOR_RANGE1 descRanges[1];
		descRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

		CD3DX12_ROOT_PARAMETER1 rootParams[3];
		rootParams[0].InitAsConstantBufferView(0, 0);
		rootParams[1].InitAsDescriptorTable(1, &descRanges[0]);
		rootParams[2].InitAsUnorderedAccessView(0, 0);

		// 根签名描述
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParams), rootParams,
			0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		// 创建根签名
		WRL::ComPtr<ID3DBlob> pSignatureBlob = nullptr;
		WRL::ComPtr<ID3DBlob> pErrorBlob = nullptr;
		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(
			&rootSignatureDesc,
			D3D_ROOT_SIGNATURE_VERSION_1_1,
			&pSignatureBlob,
			&pErrorBlob));
		ThrowIfFailed(mDevice->CreateRootSignature(0,
			pSignatureBlob->GetBufferPointer(),
			pSignatureBlob->GetBufferSize(),
			IID_PPV_ARGS(&mDiffusionRootSignature)));

		std::vector<BYTE> csBlob;
		csBlob = RawMemoryParser::readData("F:\\OpenLight\\Shader\\WindSimulation\\DiffusionCS.cso");

		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {  };
		psoDesc.pRootSignature = mDiffusionRootSignature;
		psoDesc.CachedPSO = {};
		psoDesc.CS.pShaderBytecode = &csBlob[0];
		psoDesc.CS.BytecodeLength = csBlob.size();
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		psoDesc.NodeMask = 0;
		ThrowIfFailed(mDevice->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&mDiffusionPSO)));
		
	}


}

void WindVolume::addMotors(const DirectionalMotor& motor)
{
	if (mCBMotors.directionalMotorsCount >= 4)
	{
		assert(false);
		return;
	}
	mCBMotors.directionalMotors[mCBMotors.directionalMotorsCount++] = motor;
}

