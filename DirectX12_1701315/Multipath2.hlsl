
//�e�N�X�`���f�[�^
Texture2D<float4> tex : register(t0); //�ʏ�e�N�X�`��

Texture2D<float> depth : register(t1); //�[�x

Texture2D<float4> bloom : register(t2); //�u���[�����������e�N�X�`��

Texture2D<float4> texcol : register(t3); //���f���̃e�N�X�`���J���[�̂�

Texture2D<float4> outline : register(t4); //�A�E�g���C��

Texture2D<float4> mask : register(t5); //�}�X�N

Texture2D<float4> noise : register(t6); //�m�C�Y

Texture2D<float4> distortion : register(t7); //�c�ݗp�m�[�}���}�b�v

SamplerState smp : register(s0);

struct Output
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};

struct POut
{
    float4 col : SV_Target0;
    float4 outline : SV_Target1;
    float4 distortion : SV_Target2;
    float4 depthoutline : SV_Target3;
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
POut ps(Output input)
{
    POut o;
    
    //�e�N�X�`�����
    float4 ret = tex.Sample(smp, input.uv);

    //�P�x�l
    float3 b = float3(0.298912f, 0.586611f, 0.114478f);

    float w, h, level;
    tex.GetDimensions(0, w, h, level);
    float dx = 1.0f / w; //��s�N�Z����
    float dy = 1.0f / h; //��s�N�Z����
    
    //if (input.uv.x < 0.2f && input.uv.y < 0.2f)
    //{
    //    float _depth = depth.Sample(smp, input.uv * 5);
    //    _depth = 1.0f - pow(_depth, 30);
    //    return float4(_depth, _depth, _depth, 1);
    //}
    
    //else if (input.uv.x < 0.2f && input.uv.y < 0.4f)
    //{
        
    //    return float4(texcol.Sample(smp, input.uv * 5).rgb, 1);
    //}
    
    //else if (input.uv.x < 0.2f && input.uv.y < 0.6f)
    //{
    //    return float4(outline.Sample(smp, input.uv * 5).rgb, 1);
    //}
    
    //else if (input.uv.x < 0.2f && input.uv.y < 0.8f)
    //{
    //    //�A�E�g���C���o��
    //    float edge = 2;
    //    //�ׂ荇����f�Ƃ̍����𒲂ׂ�
    
    //    float4 outret = outline.Sample(smp, input.uv * 5);
    
    //    outret = outret * 4
    //    - outline.Sample(smp, input.uv * 5 + float2(-dx * edge, 0))
    //    - outline.Sample(smp, input.uv * 5 + float2(dx * edge, 0))
    //    - outline.Sample(smp, input.uv * 5 + float2(0, dy * edge))
    //    - outline.Sample(smp, input.uv * 5 + float2(0, -dy * edge));
    
    //    outret.rgb -= mask.Sample(smp, input.uv * 5).rgb;
    //    outret.rgb -= noise.Sample(smp, input.uv * 5).rgb;
    
    //    return float4((1.0f - outret.rgb), 1.0f);
    //}

    //�c�݃m�[�}���}�b�v�K�p
    float2 distuv = distortion.Sample(smp, input.uv).xy;
    distuv = distuv * 2.0f - 1.0f;
    
    //�A�E�g���C���o��
    float edge = 1;
    
    //�ׂ荇����f�Ƃ̍����𒲂ׂ�
    float4 outret = outline.Sample(smp, input.uv);
    
    //�A�E�g���C���p�ɔ��]��������
    outret = outret * 4
        - outline.Sample(smp, input.uv + float2(-dx * edge, 0))
        - outline.Sample(smp, input.uv + float2(dx * edge, 0))
        - outline.Sample(smp, input.uv + float2(0, dy * edge))
        - outline.Sample(smp, input.uv + float2(0, -dy * edge));
    
    //�e�N�X�`���J���[
    float4 texret = texcol.Sample(smp, input.uv);
    texret = texret * 4
        - texcol.Sample(smp, input.uv + float2(-dx * edge, 0))
        - texcol.Sample(smp, input.uv + float2(dx * edge, 0))
        - texcol.Sample(smp, input.uv + float2(0, dy * edge))
        - texcol.Sample(smp, input.uv + float2(0, -dy * edge));
    
    outret -= mask.Sample(smp, input.uv);
    outret -= noise.Sample(smp, input.uv);
    
    outret = pow((1.0f - outret), 50);
    texret = pow((1.0f - texret), 50);
    
    //�A�E�g���C��
    o.outline = float4(outret.rgb * texret.rgb, 1.0f);

    //�c��
    o.distortion = texcol.Sample(smp, input.uv + distuv * 0.1f);
    
    //�u���[�����炵������
    float4 shrinkCol = GetBokehColor(bloom, smp, input.uv * float2(1, 0.5)) +
                        GetBokehColor(bloom, smp, input.uv * float2(0.5, 0.25) + float2(0, 0.5)) +
                        GetBokehColor(bloom, smp, input.uv * float2(0.25, 0.125) + float2(0, 0.75)) +
                        GetBokehColor(bloom, smp, input.uv * float2(0.125, 0.0625) + float2(0, 0.875));
    
    o.col = float4(shrinkCol.rgb + tex.Sample(smp, input.uv).rgb, 1);
 
    //�ʏ�
    return o;
}

