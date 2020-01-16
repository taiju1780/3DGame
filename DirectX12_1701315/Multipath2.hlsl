
//テクスチャデータ
Texture2D<float4> tex : register(t0); //通常テクスチャ

Texture2D<float> depth : register(t1); //深度

Texture2D<float4> bloom : register(t2); //ブルームかけたいテクスチャ

Texture2D<float4> texcol : register(t3); //モデルのテクスチャカラーのみ

Texture2D<float4> outline : register(t4); //アウトライン

Texture2D<float4> mask : register(t5); //マスク

Texture2D<float4> depthoffield : register(t6); //被写界深度

Texture2D<float4> distortion : register(t7); //歪み用ノーマルマップ

SamplerState smp : register(s0);

struct Output
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};

struct POut
{
    float4 col : SV_Target0;
    float4 outline : SV_Target1;
    float4 distortion : SV_Target2;
    float4 depthoffield : SV_Target3;
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

float4 Get5x5GaussianBlur(Texture2D<float4> tx, SamplerState s, float2 uv, float dx, float dy)
{
    float4 ret = 
    tx.Sample(s, uv + int2(-2 * dx, -2 * dy)) * 1 +
    tx.Sample(s, uv + int2(-1 * dx, -2 * dy)) * 4 +
    tx.Sample(s, uv + int2(0 * dx, -2 * dy)) * 6 +
    tx.Sample(s, uv + int2(1 * dx, -2 * dy)) * 4 +
    tx.Sample(s, uv + int2(2 * dx, -2 * dy)) * 1 +
    
    tx.Sample(s, uv + int2(-2 * dx, -1 * dy)) * 4 +
    tx.Sample(s, uv + int2(-1 * dx, -1 * dy)) * 16 +
    tx.Sample(s, uv + int2(0 * dx, -1 * dy)) * 24 +
    tx.Sample(s, uv + int2(1 * dx, -1 * dy)) * 16 +
    tx.Sample(s, uv + int2(2 * dx, -1 * dy)) * 4 +
    
    tx.Sample(s, uv + int2(-2 * dx, 0 * dy)) * 6 +
    tx.Sample(s, uv + int2(-1 * dx, 0 * dy)) * 24 +
    tx.Sample(s, uv + int2(0 * dx, 0 * dy)) * 36 +
    tx.Sample(s, uv + int2(1 * dx, 0 * dy)) * 24 +
    tx.Sample(s, uv + int2(2 * dx, 0 * dy)) * 6 +
    
    tx.Sample(s, uv + int2(-2 * dx, 1 * dy)) * 4 +
    tx.Sample(s, uv + int2(-1 * dx, 1 * dy)) * 16 +
    tx.Sample(s, uv + int2(0 * dx, 1 * dy)) * 24 +
    tx.Sample(s, uv + int2(1 * dx, 1 * dy)) * 16 +
    tx.Sample(s, uv + int2(2 * dx, 1 * dy)) * 4 +
    
    tx.Sample(s, uv + int2(-2 * dx, 2 * dy)) * 1 +
    tx.Sample(s, uv + int2(-1 * dx, 2 * dy)) * 4 +
    tx.Sample(s, uv + int2(0 * dx, 2 * dy)) * 6 +
    tx.Sample(s, uv + int2(1 * dx, 2 * dy)) * 4 +
    tx.Sample(s, uv + int2(2 * dx, 2 * dy)) * 1;
    
    return ret / 256;
}

//ピクセルシェーダ
POut ps(Output input)
{
    POut o;
    
    //テクスチャ情報
    float4 ret = tex.Sample(smp, input.uv);

    //輝度値
    float3 b = float3(0.298912f, 0.586611f, 0.114478f);

    float w, h, level;
    tex.GetDimensions(0, w, h, level);
    float dx = 1.0f / w; //一ピクセル分
    float dy = 1.0f / h; //一ピクセル分
    
    //歪みノーマルマップ適用
    float2 distuv = distortion.Sample(smp, input.uv).xy;
    distuv = distuv * 2.0f - 1.0f;
    
    //アウトライン出力
    float edge = 1;
    
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
    //outret -= noise.Sample(smp, input.uv);
    
    outret = pow((1.0f - outret), 50);
    texret = pow((1.0f - texret), 50);
    
    //アウトライン
    o.outline = float4(outret.rgb * texret.rgb, 1.0f);

    //歪み
    o.distortion = texcol.Sample(smp, input.uv + distuv * 0.1f);
    
    //ブルームずらした結果
    float4 shrinkCol = GetBokehColor(bloom, smp, input.uv * float2(1, 0.5)) +
                        GetBokehColor(bloom, smp, input.uv * float2(0.5, 0.25) + float2(0, 0.5)) +
                        GetBokehColor(bloom, smp, input.uv * float2(0.25, 0.125) + float2(0, 0.75)) +
                        GetBokehColor(bloom, smp, input.uv * float2(0.125, 0.0625) + float2(0, 0.875));
    
    o.col = float4(shrinkCol.rgb + tex.Sample(smp, input.uv).rgb, 1);
    
    //被写界深度用
    //画面真ん中からの深度の差を測る 
    float depthDiff =
        abs(depth.Sample(smp, float2(0.5, 0.5)) - depth.Sample(smp, input.uv));
        depthDiff = pow(depthDiff, 0.5f);
        float2 uvSize = float2(1, 0.5);
        float2 uvOfst = float2(0, 0);
        float t = depthDiff * 8;
        float no;
        t = modf(t, no);
    
        float4 retColor[2];
        retColor[0] = tex.Sample(smp, input.uv); //通常テクスチャ 
    
    if (no == 0.0f)
    {
        retColor[1] = Get5x5GaussianBlur(depthoffield, smp, input.uv * uvSize + uvOfst, dx, dy);
    }
    else
    {
        for (int i = 1; i <= 8; ++i)
        {
            if (i - no < 0)
                continue;
            retColor[i - no] = Get5x5GaussianBlur(depthoffield, smp, input.uv * uvSize + uvOfst, dx, dy);
            uvOfst.y += uvSize.y;
            uvSize *= 0.5f;
            if (i - no > 1)
            {
                break;
            }
        }
    }
    
    o.depthoffield = float4(lerp(retColor[0], retColor[1], t));
    //o.depthoffield = float4(depthoffield.Sample(smp,input.uv));
    //o.depthoffield = float4(texcol.Sample(smp,input.uv));
 
    //通常
    return o;
}

