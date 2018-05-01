struct VS_OUTPUT
{
	float4 pos: SV_POSITION;
	float2 uv : TEXCOORD;
};

VS_OUTPUT main(float3 pos : POSITION)
{
	VS_OUTPUT output;
	output.pos = float4(pos.x, pos.y, pos.z, 1.0f);
	output.uv = 0.5 * (pos.xy + float2(1.0, 1.0));
    return output;
}
