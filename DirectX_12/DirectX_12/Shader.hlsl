
Texture2D<float4> tex : register(t0);

SamplerState smp : register(s0);

matrix mat : register(b0);

//出力
struct Out
{
    float4 pos : POSITION;
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};

//頂点シェーダ
Out vs(float4 pos : POSITION, float2 uv : TEXCOORD)
{
    Out o;
    o.svpos = pos;
    o.pos = mul(mat, pos);
    o.uv = uv;
    return o;
}

//ピクセルシェーダ
float4 ps(Out o) : SV_Target
{
    float4 ret = tex.Sample(smp, o.uv);
    return ret;

    //return float4((o.pos.xy + float2(1, 1))/2,1,1);
    //return float4(1,1,1,1);
}
