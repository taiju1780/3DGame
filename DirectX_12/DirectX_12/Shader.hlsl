
//�o��
struct Out
{
    float4 pos : POSITION;
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};

//���_�V�F�[�_
Out vs(float4 pos : POSITION, float2 uv : TEXCOORD)
{
    Out o;
    o.svpos = pos;
    o.pos = pos;
    o.uv = uv;
    return o;
}

//�s�N�Z���V�F�[�_
float4 ps(Out o) : SV_Target
{
    return float4((o.pos.xy + float2(1, 1))/2,1,1);
    //return float4(1,1,1,1);
}
