
//�o��
struct Out
{
    float4 pos : POSITION;
    float4 svpos : SV_POSITION;
};

//���_�V�F�[�_
Out vs(float4 pos : POSITION, float4 svpos : SV_POSITION)
{
    Out o;
    o.svpos = svpos;
    o.pos = pos;
    return o;
}

//�s�N�Z���V�F�[�_
float4 ps(Out o) : SV_Target
{
    return float4((o.pos.xy + float2(1, 1))/2,1,1);
}
