cbuffer cbPerlinNoiseInfo:register(b0)
{

    /*
        x: seed in axis-x
        y: seed in axis-y
    */
    float2 seeds;
    /*
        x: fbm iteartions
    */
    int2   iterations; 
    /*
        x: X resolution of noise map
        y: Y resolution of noise map
        z: pixels per lattice
        w: pixels per lattice
    */ 
    int4 noiseMapAndLatticeRes;
}

//RWTexture2D<float2>  LatticeVecs;
RWTexture2D<float2>   NoiseMap:register(u0);





// https://www.shadertoy.com/view/ltB3zD
inline float goldNoise(float2 pos, float seed)
{
    static float PHI = (1.61803398874989484820459 * 0.1);
    static float PI = (3.14159265358979323846264 * 0.1);
    static float SQ2 = (1.41421356237309504880169 * 10000.0);
    return frac(tan(distance(pos * (PHI + seed), float2(PHI, PI))) * SQ2) * 2 - 1;
}

float smoothLerp(float a,float b,float t)
{
    float t2 = t  * t;
    float t3 = t2 * t;
    float t4 = t2 * t2;
    float t5 = t4 * t;
    
    float v = 6 * t5 -15 * t4 +10 * t3;
    
    return a * (1 - v) + b * v;
}



float2 latticeGradientVector(int2 pos,float seed)
{
    static float BIAS_X =  (1.31);
    static float BIAS_Y = (1.17);
    float2 vec = float2(
        goldNoise(float2(pos.x,pos.y),seed * BIAS_X) + 1e-4f,
        goldNoise(float2(pos.x,pos.y),seed + BIAS_Y) + 1e-4f
    );
    return vec;
}

// [numthreads(8,8,1)]
// void ComputeLatticeVectorsMainCS(
//     int3 dispatchThreadID : SV_DispatchThreadID)
// {
//     LatticeVecs[dispatchThreadID.xy] = normalize(latticeGradientVector(dispatchThreadID.xy,timeAndSeed.y));
// }

int2 calLatticeIndex(int2 latticeIndex, int2 latticeSize)
{
    int2 maxLatticeSize = noiseMapAndLatticeRes.zw;
    int2 scale = maxLatticeSize / latticeSize;

    return latticeIndex * scale;
}

[numthreads(16,16,1)]
void ComputePerlinNoiseMainCS(
    int3 dispatchThreadID: SV_DispatchThreadID)
{
    float2 randNoise = float2(0.f,0.f);
    int2 latticeSize = noiseMapAndLatticeRes.zw;
    float amplitude = 1.f;
    
    int2 pixelPos = dispatchThreadID.xy;

    for(int i = 0; i< iterations.x; ++i)
    {
        int2 latticeBaseIndex = pixelPos / latticeSize;
        float2 pos = frac(float2(pixelPos)/latticeSize);

        int2 latticeIndices[4] = {
            calLatticeIndex(latticeBaseIndex + int2(0,0),latticeSize),
            calLatticeIndex(latticeBaseIndex + int2(1,0),latticeSize),
            calLatticeIndex(latticeBaseIndex + int2(0,1),latticeSize),
            calLatticeIndex(latticeBaseIndex + int2(1,1),latticeSize)
        };
        float2 latticePos[4] = {
            float2(0,0),
            float2(1,0),
            float2(0,1),
            float2(1,1)
        };

        float2 delta[4];
        float2 v[4];
        [unroll]
        for(int j =0;j<4;++j)
        {
//            latticeVecs[j] = LatticeVecs[latticeIndices[j]];

            delta[j] = pos - latticePos[j];
            v[j].x = dot(normalize(latticeGradientVector(latticeIndices[j],seeds.x)),delta[j]);
            v[j].y = dot(normalize(latticeGradientVector(latticeIndices[j],seeds.y)),delta[j]);
        }
        float x1 = smoothLerp(v[0].x,v[1].x,pos.x);
        float x2 = smoothLerp(v[2].x,v[3].x,pos.x);
        float value = smoothLerp(x1,x2,pos.y);
        randNoise.x += value * amplitude;
        x1 = smoothLerp(v[0].y,v[1].y,pos.x);
        x2 = smoothLerp(v[2].y,v[3].y,pos.x);
        value = smoothLerp(x1,x2,pos.y);
        randNoise.y += value * amplitude;

        latticeSize = latticeSize >> 1;
        amplitude *= 0.5f;
    }
    NoiseMap[pixelPos] = randNoise * 0.5f + 0.5f;
}