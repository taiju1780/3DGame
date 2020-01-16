
//�e�N�X�`���f�[�^
Texture2D<float4> tex : register(t0); //���P�x

Texture2D<float> depth : register(t1); //�[�x

Texture2D<float4> depthoffield : register(t2); //��ʊE�[�x�p

SamplerState smp : register(s0);

struct Output
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};

//���_�V�F�[�_
Output vs(float4 pos : POSITION, float2 uv : TEXCOORD)
{
    Output output;
    output.svpos = pos;
    output.uv = uv;
    return output;
}

struct POut
{
    float4 bloom : SV_Target0;
    float4 depthoffiled : SV_Target1;
};

//�s�N�Z���V�F�[�_
POut ps(Output input)
{
    POut o;

    float d = pow(depth.Sample(smp, input.uv), 100);
    
    //�e�N�X�`�����
    float4 ret = tex.Sample(smp, input.uv);

    //�P�x�l
    float3 b = float3(0.298912f, 0.586611f, 0.114478f);

    //���m�N��
    //�P�x�l�@*�@�e�N�X�`��RGB
    //float bright = dot(b, ret.rgb);

    //return float4(bright, bright, bright, 1);

    float w, h, level;
    tex.GetDimensions(0, w, h, level);
    float dx = 1.0f / w; //��s�N�Z����
    float dy = 1.0f / h; //��s�N�Z����

    //���P�x
    o.bloom = tex.Sample(smp, input.uv);
    
    //��ʊE�[�x�p
    o.depthoffiled = depthoffield.Sample(smp, input.uv);
    
    return o;
}

