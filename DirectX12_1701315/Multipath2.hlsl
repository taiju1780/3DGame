
//テクスチャデータ
Texture2D<float4> tex : register(t0); //通常テクスチャ

Texture2D<float> depth : register(t1); //深度

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
    //ret = ret * 4
    //- tex.Sample(smp, input.uv + float2(-dx, 0))
    //- tex.Sample(smp, input.uv + float2(dx, 0))
    //- tex.Sample(smp, input.uv + float2(0, dy))
    //- tex.Sample(smp, input.uv + float2(0, -dy));

    //線を黒周りを白にしたいので反転させる

    //float brightnass = dot(b.rgb, 1 - ret.rgb);

    //線を強調
    //brightnass = pow(brightnass, 10);

    //return float4(brightnass, brightnass, brightnass, 1);

    ////ポスタリゼーション

    //float3 post = ret.rgb - fmod(ret.rgb, 0.25f);
    //return float4(post, 1);
    ////↑↓どちらでも可
    //return float4(trunc(post * 4) / 4.0f, 1);

   
    //反転
    //if (input.uv.y < 0.6 && input.uv.y > 0.4)
    //{
    //    float4 col = tex.Sample(smp, input.uv);
    //    return float4(1 - col.rgb, col.a);
    //}

    float4 shrinkCol = GetBokehColor(tex, smp, input.uv * float2(1, 0.5)) +
                        GetBokehColor(tex, smp,input.uv * float2(0.5, 0.25) + float2(0, 0.5)) +
                        GetBokehColor(tex, smp,input.uv * float2(0.25, 0.125) + float2(0, 0.75)) +
                        GetBokehColor(tex, smp,input.uv * float2(0.125, 0.0625) + float2(0, 0.875));

    return float4(shrinkCol.rgb * 0.3, ret.a);
 

    //float4 shrinkCol = tex.Sample(smp, input.uv * float2(1, 0.5)) +
    //            tex.Sample(smp, input.uv * float2(0.5, 0.25) + float2(0, 0.5)) +
    //            tex.Sample(smp, input.uv * float2(0.25, 0.125) + float2(0, 0.75)) +
    //            tex.Sample(smp, input.uv * float2(0.125, 0.0625) + float2(0, 0.875));

    //通常
    return shrinkCol;
}

