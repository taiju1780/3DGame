
//サンプラ
SamplerState smp : register(s0); //0番

//マトリクス(定数バッファ)
cbuffer mat : register(b0)
{
    matrix world; //4×4×4 = 64
    matrix view;
    matrix proj;
    matrix wvp;
    matrix lvp;
}

//定数レジスタ
cbuffer Bones : register(b1)
{
    matrix boneMats[512];
}

//出力
struct Out
{
    float4 svpos : SV_POSITION;
    float4 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    min16uint2 boneno : BONENO;
    min16uint weight : WEIGHT;
};

//頂点シェーダ
Out vs(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD, min16uint2 boneno : BONENO, min16uint weight : WEIGHT)
{
    Out o;
    float w = weight / 100.0f;
    matrix m = boneMats[boneno.x] * w + boneMats[boneno.y] * (1 - w);

    pos = mul(m, pos);

    o.pos = mul(world, pos);
    o.svpos = mul(lvp, o.pos);
    o.uv = uv;
    o.normal = mul(world, normal);
    o.boneno = boneno;
    o.weight = weight;
    return o;
}

//ピクセルシェーダ
float4 ps(Out o) : SV_Target
{
    return float4(1, 1, 1, 1);
}