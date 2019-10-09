
Texture2D<float4> tex : register(t0);

Texture2D<float4> sph : register(t1);

Texture2D<float4> spa : register(t2);

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

//�萔���W�X�^�P
//�}�e���A���p
cbuffer Material : register(b1)
{
    float4 diffuse;
    float4 specular;
    float3 ambient;
}

//���_�V�F�[�_
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

//�s�N�Z���V�F�[�_
float4 ps(Out o) : SV_Target
{
    // //����
    //float3 eye = float3(0, 15, -15);

    ////�����x�N�g��
    //float3 ray = o.pos.xyz - eye;

    ////��x�N�g��
    //float3 up = float3(0, 1, 0);

    ////���C�g
    //float3 light = float3(1, -1, 1);

    ////���C�g�J���[
    //float3 lightcolor = float3(1, 1, 1);

    ////���K����N�����v
    //light = normalize(light);
    //float brightness = saturate(dot(-light, o.normal));

    ////���̔���
    //float3 mirror = normalize(reflect(light, o.normal.xyz));
    //float spec = pow(saturate(dot(mirror, -ray)), specular.a);

    ////�X�t�B�A�}�b�v
    //float2 normalUV = (o.normal.xy + float2(1, -1)) * float2(0.5, -0.5);

    ////����
    //brightness = saturate(1 - acos(brightness) / 3.141592f);

    //float4 texColor = tex.Sample(smp, o.uv);
    
    //return float4(brightness, brightness, brightness, 1)
    //                * diffuse 
    //                * texColor
    //                * sph.Sample(smp, normalUV) 
    //                + (spa.Sample(smp, normalUV) * texColor)
    //                + float4(texColor * (ambient, 1)) * 0.5;


     //����
    float3 eye = float3(0, 15, -15);

    //�����x�N�g��
    float3 ray = o.pos.xyz - eye;

    //��x�N�g��
    float3 up = float3(0, 1, 0);
    
    //�����x�N�g���ƓK���ɂƂ�����x�N�g���ŊO�ς�����
    //�E�x�N�g��������
    float3 rightV = normalize(cross(up, ray));

    //�E�x�N�g���Ǝ����x�N�g���ŊO�ς��Ƃ���
    //���m�ȏ�x�N�g���ɂȂ�
    up = normalize(cross(ray, rightV));

    //�����x�N�g���Ɩʂɑ΂��Ă̎ˉe���Ƃ�
    float projx = dot(o.normal.xyz, rightV); //u
    float projy = dot(o.normal.xyz, up); //v

    //��
    float3 light = normalize(float3(1.0, 1.0, -1.0));
    float brightness = saturate(dot(o.normal.xyz, light));

    brightness = saturate(1 - acos(brightness) / 3.141592f);

	//speculer
    float3 mirror = normalize(reflect(-light, o.normal.xyz));
    float3 spe = pow(saturate(dot(mirror, -ray)), specular.a);

    //����UV
    float2 suv = (float2(projx, projy) + float2(1, -1)) * float2(0.5, -0.5);

    //�J���[
    float3 texColor = tex.Sample(smp, o.uv)
		* sph.Sample(smp, suv)
		+ spa.Sample(smp, suv);

    float3 matColor = saturate(diffuse.rgb + (specular.rgb * spe) + ambient.rgb * 0.5f);

    return float4(texColor * matColor, diffuse.a);
}
