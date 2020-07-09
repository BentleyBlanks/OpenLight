struct DirectionalMotor
{
    // 风场方向和强度
    // xyz: 方向
    // w  : 强度
    float4 windDirAndStrength;     
    // Motor 的位置和有效半径平方
    // xyz: 位置
    // w  : 有效半径平方
    float4 motorPosAndRadiusSqr;
};

void applyDirectionalMotor(in float3 cellPosW,in DirectionalMotor motor,in float deltaT,
    inout float3 velocityW)    
{
    float3 dir = cellPosW - motor.motorPosAndRadiusSqr.xyz;
    float3 dist2 = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
    
    if(dist2 < motor.motorPosAndRadiusSqr.w)
    {
        float3 force = motor.windDirAndStrength.xyz * motor.windDirAndStrength.w * deltaT;
        velocityW += force;
    }
}


cbuffer cbMotors:register(b0)
{
    DirectionalMotor directionalMotors[4];
    int              directionalMotorsCount;
}


cbuffer cbWindVolumeInfo:register(b1)
{
    // 风场的起始位置
    // xyz: 起始位置
    // w  : \Delta_t 
    float4 windVolumePosAndDt;
}

/*
    
*/
RWTexture3D<float3> windVolume:register(u0);
[numthreads(8,4,8)]
void UpdateWindVolumeCSMain(
    int3 dispatchThreadID:SV_DispatchThreadID
)
{
    float3 cellPosW = windVolumePosAndDt.xyz + float3(dispatchThreadID.x,dispatchThreadID.y,dispatchThreadID.z);
    float3 velocityW = windVolume[dispatchThreadID];
    applyDirectionalMotor(cellPosW,directionalMotors[0],windVolumePosAndDt.w,velocityW);
    windVolume[dispatchThreadID] = velocityW;
}