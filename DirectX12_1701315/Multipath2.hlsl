
//�e�N�X�`���f�[�^
Texture2D<float4> tex : register(t0); //�ʏ�e�N�X�`��

Texture2D<float> depth : register(t1); //�[�x

Texture2D<float4> bloom : register(t2); //�u���[�����������e�N�X�`��

Texture2D<float4> texcol : register(t3); //���f���̃e�N�X�`���J���[�̂�

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
    ret = ret * 4
        - tex.Sample(smp, input.uv * 5 + float2(-dx, 0))
        - tex.Sample(smp, input.uv * 5 + float2(dx, 0))
        - tex.Sample(smp, input.uv * 5 + float2(0, dy))
        - tex.Sample(smp, input.uv * 5 + float2(0, -dy));

        //����������𔒂ɂ������̂Ŕ��]������
    float brightnass = dot(b.rgb, 1 - ret.rgb);

        //��������
    brightnass = pow(brightnass, 10);
    
    if (input.uv.x < 0.2f && input.uv.y < 0.2f)
    {
        float4 ret = tex.Sample(smp, input.uv * 5);
        ret = ret * 4
        - tex.Sample(smp, input.uv * 5 + float2(-dx, 0))
        - tex.Sample(smp, input.uv * 5 + float2(dx, 0))
        - tex.Sample(smp, input.uv * 5 + float2(0, dy))
        - tex.Sample(smp, input.uv * 5 + float2(0, -dy));

        //����������𔒂ɂ������̂Ŕ��]������
        float brightnass = dot(b.rgb, 1 - ret.rgb);

        //��������
        brightnass = pow(brightnass, 10);

        return float4(brightnass, brightnass, brightnass, 1);
    }
    
    else if (input.uv.x < 0.2f && input.uv.y < 0.4f)
    {
        float _depth = depth.Sample(smp, input.uv * 5);
        _depth = 1.0f - pow(_depth, 30);
        return float4(_depth, _depth, _depth, 1);
    }
    
    else if (input.uv.x < 0.2f && input.uv.y < 0.6f)
    {
        return float4(texcol.Sample(smp, input.uv * 5).rgb, 1);
    } 

    ////�|�X�^���[�[�V����

    //float3 post = ret.rgb - fmod(ret.rgb, 0.25f);
    //return float4(post, 1);
    ////�����ǂ���ł���
    //return float4(trunc(post * 4) / 4.0f, 1);

    float4 shrinkCol = GetBokehColor(bloom, smp, input.uv * float2(1, 0.5)) +
                        GetBokehColor(bloom, smp, input.uv * float2(0.5, 0.25) + float2(0, 0.5)) +
                        GetBokehColor(bloom, smp, input.uv * float2(0.25, 0.125) + float2(0, 0.75)) +
                        GetBokehColor(bloom, smp, input.uv * float2(0.125, 0.0625) + float2(0, 0.875));
    
    return float4(shrinkCol.rgb + tex.Sample(smp, input.uv).rgb, tex.Sample(smp, input.uv).a);
 
    //�ʏ�
    return ret;
}

