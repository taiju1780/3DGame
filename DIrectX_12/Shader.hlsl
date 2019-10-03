
Texture2D<float4> tex : register(t0);

SamplerState smp : register(s0);

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
};

//���_�V�F�[�_
Out vs(float3 pos : POSITION, float2 uv : TEXCOORD, float3 normal : NORMAL)
{
    Out o;
    //pos = mul(world, pos);
    //pos = mul(view, pos);
    //pos = mul(proj, pos);
    o.pos = mul(world, float4(pos, 1));
    o.svpos = mul(wvp, float4(pos, 1));
    o.uv = uv;
    normal = mul(world, float4(normal, 1));
    o.normal = normal;

    return o;
}

//�s�N�Z���V�F�[�_
float4 ps(Out o) : SV_Target
{
    //float4 ret = tex.Sample(smp, o.uv);
    //return ret;

    //return float4((o.pos.xy + float2(1, 1)) / 2,1,1);
    float3 light = float3(-1, 1, -1);
    light = normalize(light);
    float brightness = dot(o.normal, light);
    return float4(brightness, brightness, brightness, 1);

    //return float4(o.normal, 1);

}
