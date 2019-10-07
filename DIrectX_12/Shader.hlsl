
Texture2D<float4> tex : register(t0);

SamplerState smp : register(s0);

//マトリクス(定数バッファ)
cbuffer mat : register(b0)
{
    matrix world; //4×4×4 = 64
    matrix view;
    matrix proj;
    matrix wvp;
    matrix lvp;
}

//出力
struct Out
{
    float4 pos : POSITION;
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

//定数レジスタ１
//マテリアル用
cbuffer Material : register(b1)
{
    float4 diffuse;
    float4 specular;
    float3 ambient;
}

//頂点シェーダ
Out vs(float3 pos : POSITION, float2 uv : TEXCOORD, float3 normal : NORMAL)
{
    Out o;
    o.pos = mul(world, float4(pos, 1));
    o.svpos = mul(wvp, float4(pos, 1));
    o.uv = uv;
    normal = mul(world, float4(normal, 1));
    o.normal = normal;

    return o;
}

//ピクセルシェーダ
float4 ps(Out o) : SV_Target
{
     //視線
    float3 eye = float3(0, 15, -15);

    //視線ベクトル
    float3 ray = o.pos.xyz - eye;

    //上ベクトル
    float3 up = float3(0, 1, 0);

    //float4 ret = tex.Sample(smp, o.uv);
    //return ret;

    //return float4((o.pos.xy + float2(1, 1)) / 2,1,1);
    float3 light = float3(-1, 1, -1);
    light = normalize(light);
    float brightness = saturate(dot(o.normal, light));
    brightness = saturate(1 - acos(brightness) / 3.141592f);

    float3 mirror = normalize(reflect(-light, o.normal));
    float3 spec = pow(saturate(dot(mirror, -ray)), specular.a);
    float3 matColor = saturate(diffuse.rgb + (specular.rgb * spec));
    float3 color = mul(matColor, brightness);
    return float4(color, diffuse.a) * tex.Sample(smp, o.uv);
    //return float4(o.normal, 1);

    //return float4(brightness * diffuse.r, brightness * diffuse.g, brightness * diffuse.b,1);
}
