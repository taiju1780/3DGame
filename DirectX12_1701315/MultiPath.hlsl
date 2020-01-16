
//テクスチャデータ
Texture2D<float4> tex : register(t0); //高輝度

Texture2D<float> depth : register(t1); //深度

Texture2D<float4> depthoffield : register(t2); //被写界深度用

SamplerState smp : register(s0);

struct Output
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};

//頂点シェーダ
Output vs(float4 pos : POSITION, float2 uv : TEXCOORD)
{
    Output output;
    output.svpos = pos;
    output.uv = uv;
    return output;
}

struct POut
{
    float4 bloom : SV_Target0;
    float4 depthoffiled : SV_Target1;
};

//ピクセルシェーダ
POut ps(Output input)
{
    POut o;

    float d = pow(depth.Sample(smp, input.uv), 100);
    
    //テクスチャ情報
    float4 ret = tex.Sample(smp, input.uv);

    //輝度値
    float3 b = float3(0.298912f, 0.586611f, 0.114478f);

    //モノクロ
    //輝度値　*　テクスチャRGB
    //float bright = dot(b, ret.rgb);

    //return float4(bright, bright, bright, 1);

    float w, h, level;
    tex.GetDimensions(0, w, h, level);
    float dx = 1.0f / w; //一ピクセル分
    float dy = 1.0f / h; //一ピクセル分

    //高輝度
    o.bloom = tex.Sample(smp, input.uv);
    
    //被写界深度用
    o.depthoffiled = depthoffield.Sample(smp, input.uv);
    
    return o;
}

