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
    // �糡�����ǿ��
    // xyz: ����
    // w  : ǿ��
    XMFLOAT4 windDirAndStrength;
    // Motor ��λ�ú���Ч�뾶ƽ��
    // xyz: λ��
    // w  : ��Ч�뾶ƽ��
    XMFLOAT4 motorPosAndRadiusSqr;
};

struct CBMotors
{
    DirectionalMotor directionalMotors[4];
    int              directionalMotorsCount;
};

struct CBWindVolumeInfo
{
    // �糡����ʼλ��
    // xyz: ��ʼλ��
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

    // �糡��ֻ�����ٶȳ�
    ID3D12Resource*         mWindVolume[2]                 = { nullptr,nullptr };
    DescriptorIndex         mWindVolummeSRVAndUAV[2]       = { {},{} };

    CBMotors                mCBMotors                      = {};
    CBWindVolumeInfo        mCBWindVolumeInfo              = {};


};

