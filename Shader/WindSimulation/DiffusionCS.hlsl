cbuffer cbDiffusionInfo:register(b0)
{
    //
    float dt;
    float diff;
    float invDeltaX;
}


// 物理场输入
Texture3D valueInput: register(s0);

// 物理场输出
RWTexture3D<float3> valueOutput: register(u0);

/*
    Threads in Group: (8 * 4 *8)
    groupSharedMemory resolution: (10 * 6 * 10)
*/
#define CacheSize 600
#define CacheIndex(x,y,z) ( (z + 1) * 60 + (y + 1) * 10 + x + 1)
groupshared float3 groupCache[CacheSize];

[numthreads(8,4,8)]
void DiffusionCSMain(
    int3 groupID:SV_GroupID,
    int3 groupThreadID:SV_GroupThreadID,
    int  groupIndex:SV_GroupIndex,
    uint3 dispatchThreadID:SV_DispatchThreadID)
{
    // 初始化 GroupShared Memory
    if(groupIndex == 0)
    {
        [unroll]
        for(int i = 0;i< CacheSize;++i)
        {
            groupCache[i] = float3(0.f,0.f,0.f);
        }
    }
    // 同步
    GroupMemoryBarrierWithGroupSync();

    // 提前读取内容进入 Cache
    groupCache[CacheIndex(groupThreadID.x,groupThreadID.y,groupThreadID.z)] = valueOutput[dispatchThreadID];

    // 同步
    GroupMemoryBarrierWithGroupSync();

    // Diffusion 迭代
    // Gaussian-Seidel
    // x_(i,j) = (x_0_i_j + a * (x_(i+1,j) + x_(i-1,j) + x_(i,j+1) + x_(i,j-1)) / (1 + 4 * a)
    float a = dt * diff * invDeltaX * invDeltaX;
    int i = groupThreadID.x;
    int j = groupThreadID.y;
    int k = groupThreadID.z; 
    valueOutput[dispatchThreadID] = (valueInput[dispatchThreadID] + 
        a * (
                groupCache[CacheIndex(i + 1,j,k)] + groupCache[CacheIndex(i - 1,j,k)] + 
                groupCache[CacheIndex(i,j + 1,k)] + groupCache[CacheIndex(i,j - 1,k)] + 
                groupCache[CacheIndex(i,j,k + 1)] + groupCache[CacheIndex(i,j,k - 1)]
            ) )/ (1 + 6 * a); 
}