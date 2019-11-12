
Texture2D<float> shadow : register(t0); //深度

SamplerState smp : register(s0);

struct Output
{
    float4 svpos : SV_POSITION;
    float4 pos : POSITION;
    float2 uv : TEXCOORD;
    float4 normal : NORMAL;
};

//マトリクス(定数バッファ)
cbuffer mat : register(b0)
{
    matrix world; //4×4×4 = 64
    matrix view;
    matrix proj;
    matrix wvp;
    matrix lvp;
}

Output vs(float3 pos : POSITION, float3 normal : NORMAL, float2 uv : TEXCOORD)
{
    Output output;

    output.pos = mul(world, float4(pos, 1));
    output.svpos = mul(wvp, float4(pos, 1));
    output.uv = uv;
    output.normal = mul(world, float4(normal, 1));

    return output;
}

float4 ps(Output input) : SV_Target
{
    float4 Pos = input.pos;

    //ライトから見たポジション
    Pos = mul(lvp, Pos);

    //uv座標変換
    float2 suv = (Pos.xy + float2(1, -1)) * float2(0.5, -0.5);

    //マップに格納された深度値
    float depth = pow(shadow.Sample(smp, suv), 100);

    //ライトと深度値の比較

    if (Pos.z > depth)
    {
        return float4(0.5, 0.5, 0.5, 1);
    }

    return float4(1, 1, 1, 1);
}