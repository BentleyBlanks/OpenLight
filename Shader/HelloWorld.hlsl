struct ModelViewProjection
{
    matrix MVP;
};

ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

struct VSIn
{
    float3 Position : POSITION;
    float3 Color    : COLOR;
};

struct VSOut
{
    float4 Color : COLOR;
    float4 Position : SV_Position;
};

VSOut vsmain(VertexPosColor In)
{
    VSOut O;
    O.Position = mul(ModelViewProjectionCB.MVP, float4(In.Position, 1.0f));
    O.Color    = float4(In.Color, 1.0f);

    return O;
}

struct PSIn
{
    float4 Color : COLOR;
};

float4 psmain(PSIn In) : SV_TARGET
{
    return In.Color;
}