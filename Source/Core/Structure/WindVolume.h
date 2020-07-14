#pragma once
#include"Utility.h"
#include<string>
#include<vector>
#include "ObjMeshLoader.h"
#include "Structure/CBStruct.h"
#include "Resource/GPUResource.h"
#include "Structure/AABB.h"

using namespace OpenLight;
struct DirectionalMotor
{
    // 风场方向和强度
    // xyz: 方向
    // w  : 强度
    XMFLOAT4 windDirAndStrength;
    // Motor 的位置和有效半径平方
    // xyz: 位置
    // w  : 有效半径平方
    XMFLOAT4 motorPosAndRadiusSqr;
};

struct CBMotors
{
    DirectionalMotor directionalMotors[4];
    int              directionalMotorsCount;
};

struct CBWindVolumeInfo
{
    // 风场的起始位置
    // xyz: 起始位置
    XMFLOAT3 windVolumePos;

    // diff
    float diff;

    // wind volume scale
    float volumeScale;

    // reciprocal of wind volume scale
    float invVolumeScale;

    // \Delta_t
    float deltaT;
};

class WindVolume
{
public:

    WindVolume(ID3D12Device5* device);
    ~WindVolume()
    {
        ReleaseCom(mApplyMotorsRootSignature);
        ReleaseCom(mApplyMotorsPSO);
        ReleaseCom(mDiffusionRootSignature);
        ReleaseCom(mDiffusionPSO);
        ReleaseCom(mWindVolume[0]);
        ReleaseCom(mWindVolume[1]);
    }

    void addMotors(const DirectionalMotor& motor);
    

    ID3D12Device5*          mDevice                        = nullptr;
    ID3D12RootSignature*    mApplyMotorsRootSignature      = nullptr;
    ID3D12PipelineState*    mApplyMotorsPSO                = nullptr;
    ID3D12RootSignature*    mDiffusionRootSignature        = nullptr;
    ID3D12PipelineState*    mDiffusionPSO                  = nullptr;

    // 风场：只包含速度场
    ID3D12Resource*         mWindVolume[2]                 = { nullptr,nullptr };
    DescriptorIndex         mWindVolummeSRVAndUAV[2]       = { {},{} };

    CBMotors                mCBMotors                      = {};
    CBWindVolumeInfo        mCBWindVolumeInfo              = {};


};

