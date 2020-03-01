
struct PSInput
{
	float4 positionH : SV_POSITION;
	float4 color : COLOR;
};

float4 PSMain(PSInput pin):SV_TARGET
{
	return pin.color;
}