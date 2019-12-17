
//テクスチャデータ
Texture2D<float4> tex : register(t0); //通常テクスチャ

Texture2D<float4> outline : register(t1); //アウトライン

Texture2D<float4> distortion : register(t2); //歪ませ後のテクスチャ

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

//ピクセルシェーダ
float4 ps(Output input) : SV_Target
{
    //テクスチャ情報
    float4 ret = tex.Sample(smp, input.uv);

    //輝度値
    float3 b = float3(0.298912f, 0.586611f, 0.114478f);

    float w, h, level;
    tex.GetDimensions(0, w, h, level);
    float dx = 1.0f / w; //一ピクセル分
    float dy = 1.0f / h; //一ピクセル分
    
    
    float4 outret = outline.Sample(smp, input.uv);
    float4 distret = distortion.Sample(smp, input.uv);
    
    if (input.uv.x < 0.2f && input.uv.y < 0.2f)
    {
        return outline.Sample(smp, input.uv * 5);
    }
 
    else if (input.uv.x < 0.2f && input.uv.y < 0.4f)
    {
        return distortion.Sample(smp, input.uv * 5);
    }
    
    //通常
    return float4((outret * distret).rgb, 1);
}

