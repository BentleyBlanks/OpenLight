

/*
    速度场输入
*/
Texture3D<float3>           velocityInput:register(t0);

/*
    速度场输出
*/
RWTexture3D<float3>         velocityOutput:register(u0);


/*
    物理场输出
*/