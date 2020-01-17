
//テクスチャデータ
Texture2D<float4> tex : register(t0); //通常テクスチャ

Texture2D<float4> outline : register(t1); //アウトライン

Texture2D<float4> distortion : register(t2); //歪ませ後のテクスチャ

Texture2D<float4> depthoffield : register(t3); //被写界深度

Texture2D<float> depth : register(t4); //normal

SamplerState smp : register(s0);

struct Output
{
    float4 pos : POSITION;
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};

//頂点シェーダ
Output vs(float4 pos : POSITION, float2 uv : TEXCOORD)
{
    Output output;
    output.pos = pos;
    output.svpos = pos;
    output.uv = uv;
    return output;
}

cbuffer General : register(b0)
{
    float time;
    uint depthfieldflags;
}

float SDFSphere3D(float3 pos, float r)
{
    return length(pos - float3(0,0,5)) - r;
}

//球
float SDFSpherelattice3D(float3 pos, float divider, float r)
{
    return length(fmod(pos,divider) - divider / 2) - r;
}

//四角
float SDFBoxlattice3D(float3 pos, float divider, float3 b)
{
    return length(max(abs(fmod(pos, divider) - divider / 2) - b, 0.0));
}


//四角生成
float SDFBox3D(float3 pos, float3 size)
{
    return length(max(abs(pos) - size, 0.0));
}

//回転
float3 RotatePos(float3 pos)
{
    float _time = time;
    _time *= 0.01f;
    float2x2 mat = float2x2(cos(_time), -sin(_time), sin(_time), cos(_time));
    
    pos.xy = mul(pos.xy, mat);
    return pos;
}

//あまり
float3 f_mod(float3 a,float3 b)
{
    return a - floor(a / b) * b;
}

//増やす
float3 SDFBOXLattice3D(float3 pos, float divider)
{
    return f_mod(pos, divider) - divider / 2;
}

float GetBoxDistance(float3 pos, float r, float3 size)
{
    float3 _pos = pos;
    _pos = SDFBOXLattice3D(_pos, r);
    
    _pos = RotatePos(_pos);
    
    return SDFBox3D(_pos, size);
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
    
    //レイマーチング
    float d = depth.Sample(smp, input.uv);
    if (pow(d, 100) >= 0.99999f)
    {
        float m = min(w, h);
        float2 aspect = float2(w / m, h / m);
        
        float3 eye = float3(0, 0, -2.5); //視点
        
        float3 tpos = float3(input.pos.xy * aspect, 0);
        
        float3 ray = normalize(tpos - eye); //レイベクトル
        
        float r = 10.0f;
        
        for (int i = 0; i < 64; ++i)
        {
            float len = GetBoxDistance(eye, r, float3(1, 1, 1));
            eye += ray * len;
            if (len < 0.001f)
            {
                return float4((float) (64 - i) / 64.0f, (float) (64 - i) / 64.0f, (float) (64 - i) / 64.0f, 1);
            }
        }
    }
    //被写界深度
    if (depthfieldflags == true)
    {
        return depthoffield.Sample(smp, input.uv);
        
    }
    else if (depthfieldflags == false)
    {
        return tex.Sample(smp, input.uv);
    }
    
    
    //通常
    return float4((outret * distret).rgb, 1);
}

