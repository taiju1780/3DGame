
//テクスチャデータ
Texture2D<float4> tex : register(t0); //通常テクスチャ

Texture2D<float> depth : register(t1); //深度

Texture2D<float4> bloom : register(t2); //ブルームかけたいテクスチャ

Texture2D<float4> texcol : register(t3); //モデルのテクスチャカラーのみ

Texture2D<float4> outline : register(t4); //アウトライン

Texture2D<float4> mask : register(t5); //マスク

Texture2D<float4> noise : register(t6); //ノイズ

Texture2D<float4> distortion : register(t7); //歪み用ノーマルマップ

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
    
    ////ポスタリゼーション

    //float3 post = ret.rgb - fmod(ret.rgb, 0.25f);
    //return float4(post, 1);
    ////↑↓どちらでも可
    //return float4(trunc(post * 4) / 4.0f, 1);
    
    if (input.uv.x < 0.2f && input.uv.y < 0.2f)
    {
        float _depth = depth.Sample(smp, input.uv * 5);
        _depth = 1.0f - pow(_depth, 30);
        return float4(_depth, _depth, _depth, 1);
    }
    
    else if (input.uv.x < 0.2f && input.uv.y < 0.4f)
    {
        
        return float4(texcol.Sample(smp, input.uv * 5).rgb, 1);
    }
    
    else if (input.uv.x < 0.2f && input.uv.y < 0.6f)
    {
        return float4(outline.Sample(smp, input.uv * 5).rgb, 1);
    }
    
    else if (input.uv.x < 0.2f && input.uv.y < 0.8f)
    {
        //アウトライン出力
        float edge = 2;
        //隣り合う画素との差分を調べる
    
        float4 outret = outline.Sample(smp, input.uv * 5);
    
        outret = outret * 4
        - outline.Sample(smp, input.uv * 5 + float2(-dx * edge, 0))
        - outline.Sample(smp, input.uv * 5 + float2(dx * edge, 0))
        - outline.Sample(smp, input.uv * 5 + float2(0, dy * edge))
        - outline.Sample(smp, input.uv * 5 + float2(0, -dy * edge));
    
        outret.rgb -= mask.Sample(smp, input.uv * 5).rgb;
        outret.rgb -= noise.Sample(smp, input.uv * 5).rgb;
    
        return float4((1.0f - outret.rgb), 1.0f);
    }

    //歪みノーマルマップ適用
    float2 distuv = distortion.Sample(smp, input.uv).xy;
    distuv = distuv * 2.0f - 1.0f;
    
    //アウトライン出力
    float edge = 1.1;
    
    //隣り合う画素との差分を調べる
    float4 outret = outline.Sample(smp, input.uv);
    
    //アウトライン用に反転したもの
    outret = outret * 4
        - outline.Sample(smp, input.uv + float2(-dx * edge, 0))
        - outline.Sample(smp, input.uv + float2(dx * edge, 0))
        - outline.Sample(smp, input.uv + float2(0, dy * edge))
        - outline.Sample(smp, input.uv + float2(0, -dy * edge));
    
    //テクスチャカラー
    float4 texret = texcol.Sample(smp, input.uv);
    texret = texret * 4
        - texcol.Sample(smp, input.uv + float2(-dx * edge, 0))
        - texcol.Sample(smp, input.uv + float2(dx * edge, 0))
        - texcol.Sample(smp, input.uv + float2(0, dy * edge))
        - texcol.Sample(smp, input.uv + float2(0, -dy * edge));
    
    outret -= mask.Sample(smp, input.uv);
    outret -= noise.Sample(smp, input.uv);
    
    outret = pow((1.0f - outret), 1);
    texret = pow((1.0f - texret), 1);
    
    //アウトラインをしっかりと出すため
    //return float4(outret.rgb * texret.rgb, 1.0f);

    return float4(outret.rgb * texret.rgb * texcol.Sample(smp, input.uv).rgb, 1.0f);
    
    //return float4((1.0f - outret.rgb), 1.0f);
    //線を黒周りを白にしたいので反転させる
    //float brightnass = dot(float3(0.3,0.3,0.4).rgb, 1 - outret.rgb);
    //線を強調
   // brightnass = pow(brightnass, 5);
    
    //return float4(brightnass, brightnass, brightnass, 1);

    //ブルームずらした結果
    float4 shrinkCol = GetBokehColor(bloom, smp, input.uv * float2(1, 0.5)) +
                        GetBokehColor(bloom, smp, input.uv * float2(0.5, 0.25) + float2(0, 0.5)) +
                        GetBokehColor(bloom, smp, input.uv * float2(0.25, 0.125) + float2(0, 0.75)) +
                        GetBokehColor(bloom, smp, input.uv * float2(0.125, 0.0625) + float2(0, 0.875));
    
    //return float4(shrinkCol.rgb + tex.Sample(smp, input.uv).rgb * brightnass,
    //tex.Sample(smp, input.uv).a);
 
    //通常
    return ret;
}

