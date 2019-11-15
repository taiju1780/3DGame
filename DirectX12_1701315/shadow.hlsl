
//�T���v��
SamplerState smp : register(s0); //0��

//�}�g���N�X(�萔�o�b�t�@)
cbuffer mat : register(b0)
{
    matrix world; //4�~4�~4 = 64
    matrix view;
    matrix proj;
    matrix wvp;
    matrix lvp;
}

//�萔���W�X�^
cbuffer Bones : register(b1)
{
    matrix boneMatrices[512];
}

//�o��
struct Out
{
    float4 pos : POSITION;
    float4 svpos : SV_POSITION;
    float3 normal : NORMAL;
    float3 vnormal : NORMAL1;
    float2 uv : TEXCOORD;
    float4 adduv : ADDUV0;
    float4 adduv2 : ADDUV1;
    float4 adduv3 : ADDUV2;
    float4 adduv4 : ADDUV3;
    min16uint weighttype : WEIGHT_TYPE;
    int4 boneindex : BONEINDEX;
    float4 weight : WEIGHT;
};

//���_�V�F�[�_
Out vs(float3 pos : POSITION, float2 uv : TEXCOORD, float3 normal : NORMAL,
        float4 adduv : ADDUV0, float4 adduv2 : ADDUV1, float4 adduv3 : ADDUV2, float4 adduv4 : ADDUV3,
            min16uint weighttype : WEIGHT_TYPE, int4 boneindex : BONEINDEX, float4 weight : WEIGHT)
{
    Out o;
    
    o.weighttype = weighttype;

    matrix m = boneMatrices[boneindex.x];

    if (o.weighttype == 1)
    {
        m = boneMatrices[boneindex.x] * float(weight.x) + boneMatrices[boneindex.y] * (1 - float(weight.x));
    }
    else if (o.weighttype == 2)
    {
        m =
        boneMatrices[boneindex.x] * float(weight.x) +
        boneMatrices[boneindex.y] * float(weight.y) +
        boneMatrices[boneindex.z] * float(weight.z) +
        boneMatrices[boneindex.w] * float(weight.w);
    }
    else if (o.weighttype == 3)
    {
        m = boneMatrices[boneindex.x] * float(weight.x) + boneMatrices[boneindex.y] * (1 - float(weight.x));
    }

    pos = mul(m, float4(pos, 1));
    o.pos = mul(world, float4(pos, 1));
    o.svpos = mul(lvp, float4(pos, 1));
    o.uv = uv;
    o.normal = mul(world, float4(normal, 1));
    o.vnormal = mul(world, float4(o.normal, 1));
    o.weight = weight;
    o.boneindex = boneindex;
    o.weighttype = weighttype;

    return o;
}

//�s�N�Z���V�F�[�_
float4 ps(Out o) : SV_Target
{
    return float4(1, 1, 1, 1);
}