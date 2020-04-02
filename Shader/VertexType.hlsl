struct StandardVSInput
{
    float3 positionL    :POSITION;
    float3 normalL      :NORMAL;
    float3 tangentL     :TANGENT;
    float2 texcoord     :TEXCOORD;
};

struct StandardPSInput
{
    float4 positionH    :SV_POSITION;
    float3 positionW    :POSITION;
    float3 normalW      :NORMAL;
    float3 tangentW     :TANGENT;
    float2 texcoord     :TEXCOORD;
};

struct StandardQuadVSInput
{
    float3 positionH    :POSITION;
    float2 texcoord     :TEXCOORD;
};

struct StandardQuadPSInput
{
    float4 positionH    :SV_POSITION;
    float2 texcoord     :TEXCOORD;
};

struct SkyBoxPSInput
{
    float4 positionH    :SV_POSITION;
    float3 positionL    :POSITION;
};