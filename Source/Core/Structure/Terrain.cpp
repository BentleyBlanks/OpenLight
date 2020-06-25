#include "Terrain.h"
#include<fstream>
#include<algorithm>
#include"TextureManager.h"
extern void loadTextureFromFile(const std::string& fileName,
    std::vector<XMFLOAT4>* rawData,
    int* pWidth,
    int* pHeight);

Terrain::Terrain(
    ID3D12Device5* device,
    ID3D12GraphicsCommandList* commandList,
    ID3D12CommandQueue* commandQueue,
    GPUDescriptorHeapWrap* descriptorHeap,
    const std::string& fileName, float scale, float widthScale, float heightScale)
{
    mScale       = scale;
    mWidthScale  = widthScale;
    mHeightScale = heightScale;
    mDevice      = device;
    loadHeightMap(fileName, scale);
    constructTerrain();

    diffuseMap = TextureMgr::LoadTexture2DFromFile2("F:\\OpenLight\\Resource\\stone.png",
        mDevice,
        commandList,
        commandQueue);
    diffuseSRVIndex = descriptorHeap->Allocate(1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    descriptorHeap->AddSRV(&diffuseSRVIndex, diffuseMap, srvDesc);

    for (auto& p : mVertices)
        aabb.ExtendAABB(p.position);
}
Terrain::~Terrain()
{
    ReleaseCom(vb);
    ReleaseCom(ib);
    ReleaseCom(diffuseMap);
}
StandardVertex& Terrain::operator()(UINT i, UINT j)
{

    assert(i < mWidth && j < mHeight);
    return mVertices[calIndex(i,j)];
}
const StandardVertex& Terrain::operator()(UINT i, UINT j) const
{
    // TODO: 在此处插入 return 语句
    assert(i < mWidth&& j < mHeight);

    return mVertices[calIndex(i, j)];
}
float Terrain::getHeight(float x, float y) const
{
    x /= mWidthScale;
    y /= mHeightScale;
    assert(x < mHalfWidth && x > -mHalfWidth && y < mHalfHeight && y > -mHalfHeight);

    x = x  + mHalfWidth;
    y = y  + mHalfHeight;

    UINT xIndex = UINT(x);
    UINT yIndex = UINT(y);
    UINT xIndexPlusOne = std::min(xIndex + 1, mWidth - 1);
    UINT yIndexPlusOne = std::min(yIndex + 1, mHeight - 1);
    float xOffset = x - xIndex;
    float yOffset = y - yIndex;

    return (1 - xOffset) * (1 - yOffset) * mVertices[calIndex(xIndex, yIndex)].position.y
        + (xOffset) * (1 - yOffset) * mVertices[calIndex(xIndex + 1, yIndex)].position.y
        + (1 - xOffset) * yOffset * mVertices[calIndex(xIndex, yIndex + 1)].position.y
        + xOffset * yOffset * mVertices[calIndex(xIndex + 1, yIndex + 1)].position.y;
}
void Terrain::loadHeightMap(const std::string& fileName, float scale)
{
#if 0
    std::ifstream inFile;
    //二进制方式打开文件
    inFile.open(fileName, std::ios::binary);

    assert(inFile);

    //文件指针移动到末尾
    inFile.seekg(0, std::ios::end);
    //大小为当前缓冲区大小
    std::vector<BYTE> inData(inFile.tellg());
    //文件指针移动到开头
    inFile.seekg(std::ios::beg);
    //读取高度信息
    inFile.read((char*)&inData[0], inData.size());
    inFile.close();

    mTerrainHeight.resize(inData.size());
    mWidth = std::sqrtf(mTerrainHeight.size());
    mHeight = std::sqrtf(mTerrainHeight.size());
    mHalfWidth = 0.5f * float(mWidth);
    mHalfHeight = 0.5f * float(mHeight);
    for (int i = 0; i < inData.size(); ++i)
    {
        mTerrainHeight[i] = float(inData[i] / 255.f) * mScale;
    }
#endif // 0
    std::vector<XMFLOAT4> rawData;
    int width;
    int height;

    loadTextureFromFile(fileName, &rawData, &width, &height);
    mTerrainHeight.resize(width * height);
    mWidth = width;
    mHeight = height;
    mHalfWidth = 0.5f * float(mWidth);
    mHalfHeight = 0.5f * float(mHeight);
    for (int i = 0; i < rawData.size(); ++i)
    {
        mTerrainHeight[i] = rawData[i].x * mScale;
    }

}

void Terrain::constructTerrain()
{
    mVertices.clear();
    mVertices.resize(mWidth * mHeight);
    mIndices.clear();
    mIndices.reserve(getGridCol() * getGridRow() * 2 * 3);
    ReleaseCom(vb);
    ReleaseCom(ib);
    for (UINT height = 0; height < mHeight; ++height)
    {
        for (UINT width = 0; width < mWidth; ++width)
        {
            auto& vertex    = mVertices[calIndex(width, height)];
            vertex.position = XMFLOAT3(
                (float(width) - mHalfWidth) * mWidthScale,
//                std::sinf(float(width) / float(mWidth) * 6.28f) * 50,
                mTerrainHeight[calIndex(width, height)],
//                   0,
                (float(height) - mHalfHeight) * mHeightScale);
            vertex.texcoord = XMFLOAT2(float(width) / mWidth, float(height) / mHeight);
            vertex.normal   = XMFLOAT3(0.f,0.f,0.f);
            vertex.tangent  = XMFLOAT3(0.f, 0.f, 0.f);

        }
    }

    for (UINT height = 0; height < mHeight - 1; ++height)
    {
        for (UINT width = 0; width < mWidth - 1; ++width)
        {
            auto v0 = calIndex(width, height);
            auto v1 = calIndex(width, height + 1);
            auto v2 = calIndex(width + 1, height + 1);
            auto v3 = calIndex(width + 1, height);
            
            mIndices.push_back(v0);
            mIndices.push_back(v1);
            mIndices.push_back(v2);
            XMFLOAT3 normal = calNormal(mVertices[v0].position,
                mVertices[v1].position,
                mVertices[v2].position);
            mVertices[v0].normal = mVertices[v0].normal + normal;
            mVertices[v1].normal = mVertices[v1].normal + normal;
            mVertices[v2].normal = mVertices[v2].normal + normal;
            //mVertices[v0].normal =  normal;
            //mVertices[v1].normal =  normal;
            //mVertices[v2].normal =  normal;
            mIndices.push_back(v2);
            mIndices.push_back(v3);
            mIndices.push_back(v0);
            normal = calNormal(mVertices[v2].position,
                mVertices[v3].position,
                mVertices[v0].position);
            mVertices[v2].normal = mVertices[v2].normal + normal;
            mVertices[v3].normal = mVertices[v3].normal + normal;
            mVertices[v0].normal = mVertices[v0].normal + normal;
            //mVertices[v2].normal = normal;
            //mVertices[v3].normal = normal;
            //mVertices[v0].normal = normal;
        }
    }

    for (auto& v : mVertices)
        v.normal = normalize(v.normal);

    // 创建 GPU 资源
    ThrowIfFailed(mDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(mVertices.size() * sizeof(StandardVertex)),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&vb)));
    void* p = nullptr;
    ThrowIfFailed(vb->Map(0, nullptr, &p));
    std::memcpy(p, &mVertices[0], mVertices.size() * sizeof(StandardVertex));
    vb->Unmap(0, nullptr);
    vbView.BufferLocation = vb->GetGPUVirtualAddress();
    vbView.SizeInBytes    = mVertices.size() * sizeof(StandardVertex);
    vbView.StrideInBytes  = sizeof(StandardVertex);

    ThrowIfFailed(mDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(mIndices.size() * sizeof(UINT)),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&ib)));
    ThrowIfFailed(ib->Map(0, nullptr, &p));
    std::memcpy(p, &mIndices[0], mIndices.size() * sizeof(UINT));
    ib->Unmap(0, nullptr);
    ibView.BufferLocation = ib->GetGPUVirtualAddress();
    ibView.SizeInBytes    = mIndices.size() * sizeof(UINT);
    ibView.Format         = DXGI_FORMAT_R32_UINT;

}

XMFLOAT3 Terrain::calNormal(const XMFLOAT3& p0, const XMFLOAT3& p1, const XMFLOAT3& p2) const
{
    auto v01 = normalize(p1 - p0);
    auto v02 = normalize(p2 - p0);
    auto n = normalize(cross(v01, v02));
    return n;
}

Grass::Grass(ID3D12Device5* device, 
    ID3D12CommandAllocator* commandAllocator,
    ID3D12GraphicsCommandList* commandList,
    ID3D12CommandQueue* commandQueue,
    GPUDescriptorHeapWrap* descriptorHeap, 
    Terrain* terrain )
{
    mDevice = device;
    StandardVertex v;
    
    if (terrain)
    {   
        int i = 0;
        for (auto& p : terrain->mVertices)
        {
            v.position = p.position;
            v.texcoord = p.texcoord;
 //           if(i % 8 == 0)
                mGrassRoots.push_back(v);
            i++;
        }
    }
    else
    {
        v.position = XMFLOAT3(0.f, 0.f, 0.f);
        mGrassRoots.push_back(v);
    }
    

    ThrowIfFailed(mDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(mGrassRoots.size() * sizeof(StandardVertex)),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&vb)));
    void* p = nullptr;
    ThrowIfFailed(vb->Map(0, nullptr, &p));
    std::memcpy(p, &mGrassRoots[0], mGrassRoots.size() * sizeof(StandardVertex));
    vb->Unmap(0, nullptr);
    vbView.BufferLocation = vb->GetGPUVirtualAddress();
    vbView.SizeInBytes = mGrassRoots.size() * sizeof(StandardVertex);
    vbView.StrideInBytes = sizeof(StandardVertex);
     
    grassInfo.grassSize = XMFLOAT4(0.5f, 5, 0.05f, 0.05f);
    grassInfo.windTime = XMFLOAT4(0.f, 0.f, 0.f, 0.f);
    if (terrain)
    {
        grassInfo.maxDepth = XMFLOAT4(terrain->aabb.Extent(0), terrain->aabb.Extent(1), terrain->aabb.Extent(2), 1);
    }
    else
    {
        grassInfo.maxDepth = XMFLOAT4(1000, 1000, 1000, 1);
    }
    //grassInfo.lodInfo[0] = XMFLOAT4(5, 1.f, 1.f,1.f);
    //grassInfo.lodInfo[1] = XMFLOAT4(3, 1.667f, 1.667f, 1.f);
    //grassInfo.lodInfo[2] = XMFLOAT4(1.f, 5.f, 5.f, 1.f);

    ThrowIfFailed(commandAllocator->Reset());
    ThrowIfFailed(commandList->Reset(commandAllocator, nullptr));
    diffuseMap = TextureMgr::LoadTexture2DFromFile2("F:\\OpenLight\\Resource\\grassDiffuse.png",
        mDevice,
        commandList,
        commandQueue);
    ThrowIfFailed(commandAllocator->Reset());
    ThrowIfFailed(commandList->Reset(commandAllocator, nullptr));
    alphaMap = TextureMgr::LoadTexture2DFromFile2("F:\\OpenLight\\Resource\\grassAlpha.png",
        mDevice,
        commandList,
        commandQueue);
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    diffuseAlphaIndex = descriptorHeap->Allocate(2, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    descriptorHeap->AddSRV(&diffuseAlphaIndex, diffuseMap, srvDesc);
    descriptorHeap->AddSRV(&diffuseAlphaIndex, alphaMap, srvDesc);


    
}
