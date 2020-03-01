struct VSOutput
{
	float4 positionH : SV_POSITION;
	float4 color : COLOR;
};

VSOutput VSMain(float4 position : POSITION, float4 color : COLOR)
{
	VSOutput vout ;
	vout.positionH = position;
	vout.color = color;
	return vout;
}
