
Texture2D<float4> tex : register(t0);

Texture2D<float4> sph : register(t1);

Texture2D<float4> spa : register(t2);

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
    // //視線
    //float3 eye = float3(0, 15, -15);

    ////視線ベクトル
    //float3 ray = o.pos.xyz - eye;

    ////上ベクトル
    //float3 up = float3(0, 1, 0);

    ////ライト
    //float3 light = float3(1, -1, 1);

    ////ライトカラー
    //float3 lightcolor = float3(1, 1, 1);

    ////正規化後クランプ
    //light = normalize(light);
    //float brightness = saturate(dot(-light, o.normal));

    ////光の反射
    //float3 mirror = normalize(reflect(light, o.normal.xyz));
    //float spec = pow(saturate(dot(mirror, -ray)), specular.a);

    ////スフィアマップ
    //float2 normalUV = (o.normal.xy + float2(1, -1)) * float2(0.5, -0.5);

    ////調整
    //brightness = saturate(1 - acos(brightness) / 3.141592f);

    //float4 texColor = tex.Sample(smp, o.uv);
    
    //return float4(brightness, brightness, brightness, 1)
    //                * diffuse 
    //                * texColor
    //                * sph.Sample(smp, normalUV) 
    //                + (spa.Sample(smp, normalUV) * texColor)
    //                + float4(texColor * (ambient, 1)) * 0.5;


     //視線
    float3 eye = float3(0, 15, -15);

    //視線ベクトル
    float3 ray = o.pos.xyz - eye;

    //上ベクトル
    float3 up = float3(0, 1, 0);
    
    //視線ベクトルと適当にとった上ベクトルで外積をだし
    //右ベクトルをつくる
    float3 rightV = normalize(cross(up, ray));

    //右ベクトルと視線ベクトルで外積をとって
    //正確な上ベクトルになる
    up = normalize(cross(ray, rightV));

    //視線ベクトルと面に対しての射影をとる
    float projx = dot(o.normal.xyz, rightV); //u
    float projy = dot(o.normal.xyz, up); //v

    //光
    float3 light = normalize(float3(1.0, 1.0, -1.0));
    float brightness = saturate(dot(o.normal.xyz, light));

    brightness = saturate(1 - acos(brightness) / 3.141592f);

	//speculer
    float3 mirror = normalize(reflect(-light, o.normal.xyz));
    float3 spe = pow(saturate(dot(mirror, -ray)), specular.a);

    //球のUV
    float2 suv = (float2(projx, projy) + float2(1, -1)) * float2(0.5, -0.5);

    //カラー
    float3 texColor = tex.Sample(smp, o.uv)
		* sph.Sample(smp, suv)
		+ spa.Sample(smp, suv);

    float3 matColor = saturate(diffuse.rgb + (specular.rgb * spe) + ambient.rgb * 0.5f);

    return float4(texColor * matColor, diffuse.a);
}
