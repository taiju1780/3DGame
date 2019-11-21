
//�e�N�X�`���f�[�^
Texture2D<float4> tex : register(t0); //�ʏ�e�N�X�`��

Texture2D<float> depth : register(t1); //�[�x

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

//�s�N�Z���V�F�[�_
float4 ps(Output input) : SV_Target
{

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

    //�G���{�X
    //ret += tex.Sample(smp, input.uv + float2(-dx, -dy)) * -2;
    //ret += tex.Sample(smp, input.uv + float2(0, -dy)) * -1;
    //ret += tex.Sample(smp, input.uv + float2(dx, -dy)) * 0;

    //ret += tex.Sample(smp, input.uv + float2(-dx, 0)) * -1;
    //ret += tex.Sample(smp, input.uv + float2(dx, 0)) * 1;

    //ret += tex.Sample(smp, input.uv + float2(-dx, dy)) * 0;
    //ret += tex.Sample(smp, input.uv + float2(0, dy)) * 1;
    //ret += tex.Sample(smp, input.uv + float2(dx, dy)) * 2;
    //return float4(ret.rgb, 0.25f);

    //����
    //�l��f����C�ɂ��
    //�ׂ荇����f�Ƃ̍����𒲂ׂ�
    //ret = ret * 4
    //- tex.Sample(smp, input.uv + float2(-dx, 0))
    //- tex.Sample(smp, input.uv + float2(dx, 0))
    //- tex.Sample(smp, input.uv + float2(0, dy))
    //- tex.Sample(smp, input.uv + float2(0, -dy));

    ////����������𔒂ɂ������̂Ŕ��]������
    //float brightnass = dot(b.rgb, 1 - ret.rgb);

    ////��������
    //brightnass = pow(brightnass, 10);

    //return float4(brightnass, brightnass, brightnass, 1);

    //�֊s�����o
    //ret = ret * -4
    //+ tex.Sample(smp, input.uv + float2(-dx, 0))
    //+ tex.Sample(smp, input.uv + float2(dx, 0))
    //+ tex.Sample(smp, input.uv + float2(0, dy))
    //+ tex.Sample(smp, input.uv + float2(0, -dy));

    //float brightnass = dot(b.rgb, 1 - ret.rgb);
    
    //brightnass = pow(brightnass, 10);

    //return float4(brightnass, brightnass, brightnass, 1);

    //�|�X�^���[�[�V����

    //float3 post = ret.rgb - fmod(ret.rgb, 0.25f);
    //return float4(post, 1);
    ////�����ǂ���ł���
    //return float4(trunc(post * 4) / 4.0f, 1);

   
    //���]

    //if (input.uv.y < 0.8 && input.uv.y > 0.7)
    //{
    //    float4 col = tex.Sample(smp, input.uv);
    //    return float4(1 - col.rgb, col.a);
    //}

    //�ʏ�
    return ret;
}