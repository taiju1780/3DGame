
Texture2D<float4> tex : register(t0);

Texture2D<float4> sph : register(t1);

Texture2D<float4> spa : register(t2);

Texture2D<float4> toon : register(t3);

SamplerState smp : register(s0);

SamplerState toonsmp : register(s1);

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
    float3 normal : NORMAL;
    float3 vnormal : NORMAL1;
    float2 uv : TEXCOORD;
    //float2 adduv : ADDUV;
    //float2 adduv2 : ADDUV1;
    //float2 adduv3 : ADDUV2;
    //float2 adduv4 : ADDUV3;
    //int weighttype : WEIGHT_TYPE;
    //int4 boneindex : BONEINDEX;
    //float4 weight : WEIGHT;
};

//定数レジスタ１
//マテリアル用
cbuffer Material : register(b1)
{
    float4 diffuse;
    float4 specular;
    float3 ambient;
}

cbuffer Bones : register(b2)
{
    matrix boneMatrices[512];
}

//頂点シェーダ
Out vs(float3 pos : POSITION, float2 uv : TEXCOORD, float3 normal : NORMAL
            //float2 adduv : ADDUV, float2 adduv2 : ADDUV2, float2 adduv3 : ADDUV3, float2 adduv4 : ADDUV4,
            //    int weighttype : WEIGHT_TYPE, int4 boneindex : BONEINDEX, float4 weight : WEIGHT
)
{
    Out o;

    //float w = weight / 100.0f;
    //matrix m = 0;
    //o.weighttype = weighttype;
    //if (o.weighttype == 0)
    //{
    //    m = boneMatrices[boneindex.x] * w + boneMatrices[boneindex.y] * (1 - w);
    //}
    //pos = mul(m, float4(pos, 1));
    o.pos = mul(world, float4(pos, 1));
    o.svpos = mul(wvp, float4(pos, 1));
    o.uv = uv;
    o.normal = mul(world, float4(normal, 1));
    o.vnormal = mul(world, float4(o.normal, 1));
    //o.boneindex = boneindex;
    //o.weight = weight;

    return o;
}

//ピクセルシェーダ
float4 ps(Out o) : SV_Target
{
    //return float4((float2) (o.boneno % 2), 0, 1);

     //視線
    float3 eye = float3(0, 18, -20);

    //視線ベクトル
    float3 ray = o.pos.xyz - eye;

    //上ベクトル
    float3 up = float3(0, 1, 0);

    //ライト
    float3 light = normalize(float3(1, -1, 1));

    //正規化後クランプ
    light = normalize(light);
    float diffuseB = saturate(dot(-light, o.normal));

    //光の反射(拡散反射)
    float3 mirror = normalize(reflect(light, o.normal.xyz));
    float spec = pow(saturate(dot(mirror, -ray)), specular.a);

    //スフィアマップ
    float2 normalUV = (o.vnormal.xy + float2(1, -1)) * float2(0.5, -0.5);

    //調整
    diffuseB = saturate(1 - acos(diffuseB) / 3.141592f);

    //toon
    float4 toonDif = toon.Sample(toonsmp, float2(0, 1.0 - diffuseB));
   
    //return saturate(float4(tex.Sample(smp, o.uv)) * toonDif * diffuse);
    
    //表示
    return saturate(
            toonDif
            * diffuse
            * tex.Sample(smp, o.uv)
            * sph.Sample(smp, normalUV))
            + saturate(float4(spec * specular.rgb, 1))
            + float4(tex.Sample(smp, o.uv).rgb * ambient * 0.2, 1);

}
