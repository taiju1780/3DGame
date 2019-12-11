
//テクスチャデータ
Texture2D<float4> tex : register(t0); //通常テクスチャ

Texture2D<float> depth : register(t1); //深度

Texture2D<float4> bloom : register(t2); //ブルームかけたいテクスチャ

Texture2D<float4> texcol : register(t3); //モデルのテクスチャカラーのみ

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

float4 GetBokehColor(Texture2D<float4> tx, SamplerState s, float2 uv)
{
    return tx.Sample(s, uv, int2(-1, -1)) / 16 +
  tx.Sample(s, uv, int2(0, -1)) / 8 +
  tx.Sample(s, uv, int2(1, -1)) / 16 +
  tx.Sample(s, uv, int2(-1, 0)) / 8 +
  tx.Sample(s, uv) / 4 +
  tx.Sample(s, uv, int2(1, 0)) / 8 +
  tx.Sample(s, uv, int2(-1, 1)) / 16 +
  tx.Sample(s, uv, int2(0, 1)) / 8 +
  tx.Sample(s, uv, int2(1, 1)) / 16;
}

//ピクセルシェーダ
float4 ps(Output input) : SV_Target
{

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

    //エンボス
    //ret += tex.Sample(smp, input.uv + float2(-dx, -dy)) * -2;
    //ret += tex.Sample(smp, input.uv + float2(0, -dy)) * -1;
    //ret += tex.Sample(smp, input.uv + float2(dx, -dy)) * 0;

    //ret += tex.Sample(smp, input.uv + float2(-dx, 0)) * -1;
    //ret += tex.Sample(smp, input.uv + float2(dx, 0)) * 1;

    //ret += tex.Sample(smp, input.uv + float2(-dx, dy)) * 0;
    //ret += tex.Sample(smp, input.uv + float2(0, dy)) * 1;
    //ret += tex.Sample(smp, input.uv + float2(dx, dy)) * 2;
    //return float4(ret.rgb, 0.25f);

    //線画
    //四画素分一気にやる

    //隣り合う画素との差分を調べる
    ret = ret * 4
        - tex.Sample(smp, input.uv * 5 + float2(-dx, 0))
        - tex.Sample(smp, input.uv * 5 + float2(dx, 0))
        - tex.Sample(smp, input.uv * 5 + float2(0, dy))
        - tex.Sample(smp, input.uv * 5 + float2(0, -dy));

        //線を黒周りを白にしたいので反転させる
    float brightnass = dot(b.rgb, 1 - ret.rgb);

        //線を強調
    brightnass = pow(brightnass, 10);
    
    if (input.uv.x < 0.2f && input.uv.y < 0.2f)
    {
        float4 ret = tex.Sample(smp, input.uv * 5);
        ret = ret * 4
        - tex.Sample(smp, input.uv * 5 + float2(-dx, 0))
        - tex.Sample(smp, input.uv * 5 + float2(dx, 0))
        - tex.Sample(smp, input.uv * 5 + float2(0, dy))
        - tex.Sample(smp, input.uv * 5 + float2(0, -dy));

        //線を黒周りを白にしたいので反転させる
        float brightnass = dot(b.rgb, 1 - ret.rgb);

        //線を強調
        brightnass = pow(brightnass, 10);

        return float4(brightnass, brightnass, brightnass, 1);
    }
    
    else if (input.uv.x < 0.2f && input.uv.y < 0.4f)
    {
        float _depth = depth.Sample(smp, input.uv * 5);
        _depth = 1.0f - pow(_depth, 30);
        return float4(_depth, _depth, _depth, 1);
    }
    
    else if (input.uv.x < 0.2f && input.uv.y < 0.6f)
    {
        return float4(texcol.Sample(smp, input.uv * 5).rgb, 1);
    } 

    ////ポスタリゼーション

    //float3 post = ret.rgb - fmod(ret.rgb, 0.25f);
    //return float4(post, 1);
    ////↑↓どちらでも可
    //return float4(trunc(post * 4) / 4.0f, 1);

    float4 shrinkCol = GetBokehColor(bloom, smp, input.uv * float2(1, 0.5)) +
                        GetBokehColor(bloom, smp, input.uv * float2(0.5, 0.25) + float2(0, 0.5)) +
                        GetBokehColor(bloom, smp, input.uv * float2(0.25, 0.125) + float2(0, 0.75)) +
                        GetBokehColor(bloom, smp, input.uv * float2(0.125, 0.0625) + float2(0, 0.875));
    
    return float4(shrinkCol.rgb + tex.Sample(smp, input.uv).rgb, tex.Sample(smp, input.uv).a);
 
    //通常
    return ret;
}

