
//出力
struct Out
{
    float4 pos : POSITION;
    float4 svpos : SV_POSITION;
};

//頂点シェーダ
Out vs(float4 pos : POSITION)
{
    Out o;
    o.svpos = pos;
    o.pos = pos;
    return o;
}

//ピクセルシェーダ
float4 ps(Out o) : SV_Target
{
    return float4((o.pos.xy + float2(1, 1))/2,1,1);
    //return float4(1,1,1,1);
}
