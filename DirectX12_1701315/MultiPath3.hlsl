
//�e�N�X�`���f�[�^
Texture2D<float4> tex : register(t0); //�ʏ�e�N�X�`��

Texture2D<float4> outline : register(t1); //�A�E�g���C��

Texture2D<float4> distortion : register(t2); //�c�܂���̃e�N�X�`��

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
    //�e�N�X�`�����
    float4 ret = tex.Sample(smp, input.uv);

    //�P�x�l
    float3 b = float3(0.298912f, 0.586611f, 0.114478f);

    float w, h, level;
    tex.GetDimensions(0, w, h, level);
    float dx = 1.0f / w; //��s�N�Z����
    float dy = 1.0f / h; //��s�N�Z����
    
    
    float4 outret = outline.Sample(smp, input.uv);
    float4 distret = distortion.Sample(smp, input.uv);
    
    if (input.uv.x < 0.2f && input.uv.y < 0.2f)
    {
        return outline.Sample(smp, input.uv * 5);
    }
 
    else if (input.uv.x < 0.2f && input.uv.y < 0.4f)
    {
        return distortion.Sample(smp, input.uv * 5);
    }
    
    //�ʏ�
    return float4((outret * distret).rgb, 1);
}

