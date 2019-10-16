
Texture2D<float4> tex : register(t0);

Texture2D<float4> sph : register(t1);

Texture2D<float4> spa : register(t2);

Texture2D<float4> toon : register(t3);

SamplerState smp : register(s0);

SamplerState toonsmp : register(s1);

//�}�g���N�X(�萔�o�b�t�@)
cbuffer mat : register(b0)
{
    matrix world; //4�~4�~4 = 64
    matrix view;
    matrix proj;
    matrix wvp;
    matrix lvp;
}

//�o��
struct Out
{
    float4 pos : POSITION;
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float2 boneno : BONENO;
    float weight : WEIGHT;
};

//�萔���W�X�^�P
//�}�e���A���p
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

//���_�V�F�[�_
Out vs(float3 pos : POSITION, float2 uv : TEXCOORD, float3 normal : NORMAL, float2 boneno : BONENO, float weight : WEIGHT)
{
    Out o;

    float w = weight / 100.0f;
    matrix m = boneMatrices[boneno.x] * w + boneMatrices[boneno.y] * (1 - w);

    pos = mul(m, float4(pos, 1));
    o.pos = mul(world, float4(pos, 1));
    o.svpos = mul(wvp, float4(pos, 1));
    o.uv = uv;
    normal = mul(world, float4(normal, 1));
    o.normal = normal;
    o.boneno = boneno;
    o.weight = weight;

    return o;
}

//�s�N�Z���V�F�[�_
float4 ps(Out o) : SV_Target
{
     //����
    float3 eye = float3(0, 18, -20);

    //�����x�N�g��
    float3 ray = o.pos.xyz - eye;

    //��x�N�g��
    float3 up = float3(0, 1, 0);

    //���C�g
    float3 light = normalize(float3(1, -1, 1));

    //���K����N�����v
    light = normalize(light);
    float diffuseB = saturate(dot(-light, o.normal));

    //���̔���(�g�U����)
    float3 mirror = normalize(reflect(light, o.normal.xyz));
    float spec = pow(saturate(dot(mirror, -ray)), specular.a);

    //�X�t�B�A�}�b�v
    float2 normalUV = (o.normal.xy + float2(1, -1)) * float2(0.5, -0.5);

    //����
    diffuseB = saturate(1 - acos(diffuseB) / 3.141592f);

    //toon
    float4 toonDif = toon.Sample(toonsmp, float2(0, 1.0 - diffuseB));
    
    return saturate(
            toonDif
            * diffuse
            * tex.Sample(smp, o.uv)
            * sph.Sample(smp, normalUV))
            + saturate(float4(spec * specular.rgb, 1))
            + float4(tex.Sample(smp, o.uv).rgb * ambient * 0.2, 1);

    //return float4((float2) (o.boneno % 2), 0,1);

}
