////////////////////////////////////////////////////////////////////////////////////////////////
//
//  full.fx ver1.2+
//  作成: 舞力介入P
//　Modified: Furia
//	エッジカラー対応
//	複数枚ノーマルマップ追加 (変更部分はノーマルマップで検索）
//
////////////////////////////////////////////////////////////////////////////////////////////////


//ノーマルマップ用テクスチャ/////////////////////////////////////////////////////////
//ノーマルマップ用のテクスチャの枚数を指定します。（※適用する材質数ではありません）
//標準では１０枚まで可(0～10)
#define MAKE_NORMAL_TEX_COUNT 1

//ノーマルマップを使用する材質番号を指定しましょう（MMEリファレンス Subset項参照）
//同じノーマルマップテクスチャを、複数材質に適用する場合に工夫して記述しましょう
//使用しない場合、コメントアウトすると通常のfull.fxと同等
//使用するノーマルマップ数にあわせて、コメントをはずしてください。
#define USE_NORMAL_MAP_MATERIALS0 "0-2"
//#define USE_NORMAL_MAP_MATERIALS1 "1"
//#define USE_NORMAL_MAP_MATERIALS2 "2"
//#define USE_NORMAL_MAP_MATERIALS3 "3"
//#define USE_NORMAL_MAP_MATERIALS4 "4"
//#define USE_NORMAL_MAP_MATERIALS5 "5"
//#define USE_NORMAL_MAP_MATERIALS6 "6"
//#define USE_NORMAL_MAP_MATERIALS7 "7"
//#define USE_NORMAL_MAP_MATERIALS8 "8"
//#define USE_NORMAL_MAP_MATERIALS9 "9"


//テクスチャフィルタ一括設定////////////////////////////////////////////////////////////////////
//異方向性フィルタ（なめらか表示＆遠くても綺麗　※TEXFILTER_MAXANISOTROPYも設定しましょう）
//#define TEXFILTER_SETTING ANISOTROPIC
//バイリニア（なめらか表示：標準）
#define TEXFILTER_SETTING LINEAR
//ニアレストネイバー（くっきり表示：テクスチャのドットが綺麗に出る）
//#define TEXFILTER_SETTING POINT

//異方向性フィルタを使うときの倍率
//1:Off or 2の倍数で指定(大きいほどなめらか効果・負荷大)
//対応しているかどうかは、ハードウェア次第（RadeonHD機種なら16までは対応しているはず
#define TEXFILTER_MAXANISOTROPY 2

//異方向性フィルタについて
//テクスチャのMiplevelsがある程度あるとより綺麗になります。
//※テクスチャ読み込みもMMEで書く必要があります
//2の累乗サイズのテクスチャであると比較的古いハードウェアにも優しいです。


//テクスチャアドレッシング一括設定//////////////////////////////////////////////////////////////
//テクスチャ別に設定したいときは、サンプラーステート記述部で行いましょう。
//U軸ループ
//#define TEXADDRESS_U WRAP 
//U軸ミラー＆ループ
//#define TEXADDRESS_U MIRROR
//U軸0to1制限：MMD標準
#define TEXADDRESS_U CLAMP
//U軸-1to1制限：-方向はミラーリング
//#define TEXADDRESS_U MIRRORONCE

//V軸ループ
//#define TEXADDRESS_V WRAP 
//V軸ミラー＆ループ
//#define TEXADDRESS_V MIRROR
//V軸0to1制限：MMD標準
#define TEXADDRESS_V CLAMP
//V軸-1to1制限：-方向はミラーリング
//#define TEXADDRESS_V MIRRORONCE

// パラメータ宣言

// 座法変換行列
float4x4 WorldViewProjMatrix      : WORLDVIEWPROJECTION;
float4x4 WorldMatrix              : WORLD;
float4x4 ViewMatrix               : VIEW;
float4x4 LightWorldViewProjMatrix : WORLDVIEWPROJECTION < string Object = "Light"; >;

float3   LightDirection    : DIRECTION < string Object = "Light"; >;
float3   CameraPosition    : POSITION  < string Object = "Camera"; >;

// マテリアル色
float4   MaterialDiffuse   : DIFFUSE  < string Object = "Geometry"; >;
float3   MaterialAmbient   : AMBIENT  < string Object = "Geometry"; >;
float3   MaterialEmmisive  : EMISSIVE < string Object = "Geometry"; >;
float3   MaterialSpecular  : SPECULAR < string Object = "Geometry"; >;
float    SpecularPower     : SPECULARPOWER < string Object = "Geometry"; >;
float3   MaterialToon      : TOONCOLOR;
// ライト色
float3   LightDiffuse      : DIFFUSE   < string Object = "Light"; >;
float3   LightAmbient      : AMBIENT   < string Object = "Light"; >;
float3   LightSpecular     : SPECULAR  < string Object = "Light"; >;
static float4 DiffuseColor  = MaterialDiffuse  * float4(LightDiffuse, 1.0f);
static float3 AmbientColor  = saturate(MaterialAmbient  * LightAmbient + MaterialEmmisive);
static float3 SpecularColor = MaterialSpecular * LightSpecular;

//輪郭色
float4	 EgColor;
bool     parthf;   // パースペクティブフラグ
bool     transp;   // 半透明フラグ
bool	 spadd;    // スフィアマップ加算合成フラグ
#define SKII1    1500
#define SKII2    8000
#define Toon     3

// オブジェクトのテクスチャ
texture ObjectTexture: MATERIALTEXTURE;
sampler ObjTexSampler = sampler_state {
    texture = <ObjectTexture>;
    MINFILTER = TEXFILTER_SETTING;
    MAGFILTER = TEXFILTER_SETTING;
    MIPFILTER = LINEAR;
	AddressU = TEXADDRESS_U;
	AddressV = TEXADDRESS_V;
	MAXANISOTROPY = TEXFILTER_MAXANISOTROPY;
};

// スフィアマップのテクスチャ
texture ObjectSphereMap: MATERIALSPHEREMAP;
sampler ObjSphareSampler = sampler_state {
    texture = <ObjectSphereMap>;
    MINFILTER = LINEAR;
    MAGFILTER = LINEAR;
};

// MMD本来のsamplerを上書きしないための記述です。削除不可。
sampler MMDSamp0 : register(s0);
sampler MMDSamp1 : register(s1);
sampler MMDSamp2 : register(s2);


//ノーマルマップ用テクスチャ/////////////////////////////////////////////////////////
//実定義部
#if MAKE_NORMAL_TEX_COUNT > 0
	#define NORMAL_TEX_0 NORMALMAP_TEX_BASE( 0,"TB_suit_n.png" ,1)
	#define MAKE_NORMAL_TEX_LIST NORMAL_TEX_0
#endif

#undef NORMALMAP_TEX_BASE
#define NORMALMAP_TEX_BASE(NUM,TEXNAME,MIPMAPLEVEL) \
texture NormalMapTex##NUM < \
	string ResourceName = TEXNAME; \
	int Miplevels = MIPMAPLEVEL; \
>; \
sampler NormalMapTexSampler##NUM = sampler_state { \
	texture = <NormalMapTex##NUM>; \
	MINFILTER = TEXFILTER_SETTING; \
    MAGFILTER = TEXFILTER_SETTING; \
	MIPFILTER = LINEAR; \
	AddressU = WRAP; \
	AddressV = WRAP; \
	MAXANISOTROPY = 16; \
};

MAKE_NORMAL_TEX_LIST

////////////////////////////////////////////////////////////////////////////////////////////////
//接空間取得
float3x3 compute_tangent_frame(float3 Normal, float3 View, float2 UV)
{
  float3 dp1 = ddx(View);
  float3 dp2 = ddy(View);
  float2 duv1 = ddx(UV);
  float2 duv2 = ddy(UV);

  float3x3 M = float3x3(dp1, dp2, cross(dp1, dp2));
  float2x3 inverseM = float2x3(cross(M[1], M[2]), cross(M[2], M[0]));
  float3 Tangent = mul(float2(duv1.x, duv2.x), inverseM);
  float3 Binormal = mul(float2(duv1.y, duv2.y), inverseM);

  return float3x3(normalize(Tangent), normalize(Binormal), Normal);
}
/*
	float3x3 tangentFrame = compute_tangent_frame(In.Normal, In.Eye, In.Tex);
	float3 normal = normalize(mul(2.0f * tex2D(normalSamp, In.Tex) - 1.0f, tangentFrame));
*/

////////////////////////////////////////////////////////////////////////////////////////////////
// 輪郭描画

// 頂点シェーダ
float4 ColorRender_VS(float4 Pos : POSITION) : POSITION 
{
    // カメラ視点のワールドビュー射影変換
    return mul( Pos, WorldViewProjMatrix );
}

// ピクセルシェーダ
float4 ColorRender_PS() : COLOR
{
    // 黒で塗りつぶし
    return EgColor;//float4(0,0,0,1);
}

// 輪郭描画用テクニック
technique EdgeTec < string MMDPass = "edge"; > {
    pass DrawEdge {
        AlphaBlendEnable = FALSE;
        AlphaTestEnable  = FALSE;

        VertexShader = compile vs_3_0 ColorRender_VS();
        PixelShader  = compile ps_3_0 ColorRender_PS();
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////
// 影（非セルフシャドウ）描画

// 頂点シェーダ
float4 Shadow_VS(float4 Pos : POSITION) : POSITION
{
    // カメラ視点のワールドビュー射影変換
    return mul( Pos, WorldViewProjMatrix );
}

// ピクセルシェーダ
float4 Shadow_PS() : COLOR
{
    // アンビエント色で塗りつぶし
    return float4(AmbientColor.rgb, 0.65f);
}

// 影描画用テクニック
technique ShadowTec < string MMDPass = "shadow"; > {
    pass DrawShadow {
        VertexShader = compile vs_3_0 Shadow_VS();
        PixelShader  = compile ps_3_0 Shadow_PS();
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////
// オブジェクト描画（セルフシャドウOFF）

struct VS_OUTPUT {
    float4 Pos        : POSITION;    // 射影変換座標
    float2 Tex        : TEXCOORD1;   // テクスチャ
    float3 Normal     : TEXCOORD2;   // 法線
    float3 Eye        : TEXCOORD3;   // カメラとの相対位置
    float2 SpTex      : TEXCOORD4;	 // スフィアマップテクスチャ座標
    float4 Color      : COLOR0;      // ディフューズ色
};

// 頂点シェーダ
VS_OUTPUT Basic_VS(float4 Pos : POSITION, float3 Normal : NORMAL, float2 Tex : TEXCOORD0, uniform bool useTexture, uniform bool useSphereMap, uniform bool useToon)
{
    VS_OUTPUT Out = (VS_OUTPUT)0;
    
    // カメラ視点のワールドビュー射影変換
    Out.Pos = mul( Pos, WorldViewProjMatrix );
    
    // カメラとの相対位置
    Out.Eye = CameraPosition - mul( Pos, WorldMatrix );
    // 頂点法線
    Out.Normal = normalize( mul( Normal, (float3x3)WorldMatrix ) );
    
    // ディフューズ色＋アンビエント色 計算
    Out.Color.rgb = AmbientColor;
    if ( !useToon ) {
        Out.Color.rgb += max(0,dot( Out.Normal, -LightDirection )) * DiffuseColor.rgb;
    }
    Out.Color.a = DiffuseColor.a;
    Out.Color = saturate( Out.Color );
    
    // テクスチャ座標
    Out.Tex = Tex;
    
    if ( useSphereMap ) {
        // スフィアマップテクスチャ座標
        float2 NormalWV = mul( Out.Normal, (float3x3)ViewMatrix );
        Out.SpTex.x = NormalWV.x * 0.5f + 0.5f;
        Out.SpTex.y = NormalWV.y * -0.5f + 0.5f;
    }
    
    return Out;
}

// ピクセルシェーダ
float4 Basic_PS(VS_OUTPUT IN, uniform bool useTexture, uniform bool useSphereMap, uniform bool useToon
, uniform sampler DiffuseSamp
) : COLOR0
{
    // ディフューズ色＋アンビエント色 計算
    IN.Color.rgb = AmbientColor;
    if ( !useToon ) {
        IN.Color.rgb += max(0,dot( IN.Normal, -LightDirection )) * DiffuseColor.rgb;
    }
    IN.Color.a = DiffuseColor.a;
    IN.Color = saturate( IN.Color );
    
    // スペキュラ色計算
    float3 HalfVector = normalize( normalize(IN.Eye) + -LightDirection );
    float3 Specular = pow( max(0,dot( HalfVector, normalize(IN.Normal) )), SpecularPower ) * SpecularColor;
    
    float4 Color = IN.Color;
    if ( useTexture ) {
        // テクスチャ適用
        Color *= tex2D( DiffuseSamp, IN.Tex );
    }
    if ( useSphereMap ) {
        // スフィアマップ適用
        if(spadd) Color += tex2D(ObjSphareSampler,IN.SpTex);
        else      Color *= tex2D(ObjSphareSampler,IN.SpTex);
    }
    
    if ( useToon ) {
        // トゥーン適用
        float LightNormal = dot( IN.Normal, -LightDirection );
        Color.rgb *= lerp(MaterialToon, float3(1,1,1), saturate(LightNormal * 16 + 0.5));
    }
    
    // スペキュラ適用
    Color.rgb += Specular;
    
    return Color;
}
// ピクセルシェーダ　ノーマルマップ追加
float4 Basic_PS_NormalMap(VS_OUTPUT IN, uniform bool useTexture, uniform bool useSphereMap, uniform bool useToon
, uniform sampler DiffuseSamp
, uniform sampler normalSamp) : COLOR0
{
	float3x3 tangentFrame = compute_tangent_frame(IN.Normal, IN.Eye, IN.Tex);
	float3 normal = normalize(mul(2.0f * tex2D(normalSamp, IN.Tex) - 1.0f, tangentFrame));
	
    IN.Color.rgb = AmbientColor;
    if ( !useToon ) {
        IN.Color.rgb += max(0,dot( normal, -LightDirection )) * DiffuseColor.rgb;
    }
    IN.Color.a = DiffuseColor.a;
    IN.Color = saturate( IN.Color );
	
	float2 NormalWV = mul( normal, (float3x3)ViewMatrix );
	IN.SpTex.x = NormalWV.x * 0.5f + 0.5f;
	IN.SpTex.y = NormalWV.y * -0.5f + 0.5f;
	
    // スペキュラ色計算
    float3 HalfVector = normalize( normalize(IN.Eye) + -LightDirection );
    float3 Specular = pow( max(0,dot( HalfVector, normalize(normal) )), SpecularPower ) * SpecularColor;
    
    float4 Color = IN.Color;
    if ( useTexture ) {
        // テクスチャ適用
        Color *= tex2D( DiffuseSamp, IN.Tex );
    }
    if ( useSphereMap ) {
        // スフィアマップ適用
        if(spadd) Color += tex2D(ObjSphareSampler,IN.SpTex);
        else      Color *= tex2D(ObjSphareSampler,IN.SpTex);
    }
    
    if ( useToon ) {
        // トゥーン適用
        float LightNormal = dot( normal, -LightDirection );
        Color.rgb *= lerp(MaterialToon, float3(1,1,1), saturate(LightNormal * 16 + 0.5));
    }
    
    // スペキュラ適用
    Color.rgb += Specular;
    
    return Color;
}

//-----------------------------------------------------------------------------------------------------
#define MAKE_TECH_NORMAL_MAP_SET(SUBSET,NORMAL_MAP_NUM) \
TECH_NORMAL_MAP_BASE(0,0,0,SUBSET,NORMAL_MAP_NUM) \
TECH_NORMAL_MAP_BASE(1,0,0,SUBSET,NORMAL_MAP_NUM) \
TECH_NORMAL_MAP_BASE(0,1,0,SUBSET,NORMAL_MAP_NUM) \
TECH_NORMAL_MAP_BASE(1,1,0,SUBSET,NORMAL_MAP_NUM) \
TECH_NORMAL_MAP_BASE(0,0,1,SUBSET,NORMAL_MAP_NUM) \
TECH_NORMAL_MAP_BASE(1,0,1,SUBSET,NORMAL_MAP_NUM) \
TECH_NORMAL_MAP_BASE(0,1,1,SUBSET,NORMAL_MAP_NUM) \
TECH_NORMAL_MAP_BASE(1,1,1,SUBSET,NORMAL_MAP_NUM)

#undef TECH_NORMAL_MAP_BASE
#define TECH_NORMAL_MAP_BASE(TEX,SPH,TOON,SUBSET,NORMAL_MAP_NUM) \
technique NormalMap##NORMAL_MAP_NUM##_MainTec##TEX##SPH##TOON < \
		string Subset = SUBSET; \
		string MMDPass = "object"; bool UseTexture = TEX; bool UseSphereMap = SPH; bool UseToon = TOON; > { \
	pass DrawObject { \
		VertexShader = compile vs_3_0 Basic_VS(TEX, SPH, TOON); \
		PixelShader  = compile ps_3_0 Basic_PS_NormalMap(TEX, SPH, TOON, \
		ObjTexSampler, NormalMapTexSampler##NORMAL_MAP_NUM ); \
    } \
}

// ノーマルマップ対応版 オブジェクト描画用テクニック生成
#ifdef USE_NORMAL_MAP_MATERIALS0
MAKE_TECH_NORMAL_MAP_SET(USE_NORMAL_MAP_MATERIALS0,0)
#endif

//-----------------------------------------------------------------------------------------------------
// 標準エミュレート
// オブジェクト描画用テクニック（アクセサリ用）
// 不要なものは削除可
technique MainTec0 < string MMDPass = "object"; bool UseTexture = false; bool UseSphereMap = false; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 Basic_VS(false, false, false);
        PixelShader  = compile ps_3_0 Basic_PS(false, false, false,
		ObjTexSampler);
    }
}

technique MainTec1 < string MMDPass = "object"; bool UseTexture = true; bool UseSphereMap = false; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 Basic_VS(true, false, false);
        PixelShader  = compile ps_3_0 Basic_PS(true, false, false,
		ObjTexSampler);
    }
}

technique MainTec2 < string MMDPass = "object"; bool UseTexture = false; bool UseSphereMap = true; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 Basic_VS(false, true, false);
        PixelShader  = compile ps_3_0 Basic_PS(false, true, false,
		ObjTexSampler);
    }
}

technique MainTec3 < string MMDPass = "object"; bool UseTexture = true; bool UseSphereMap = true; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 Basic_VS(true, true, false);
        PixelShader  = compile ps_3_0 Basic_PS(true, true, false,
		ObjTexSampler);
    }
}

// オブジェクト描画用テクニック（PMDモデル用）
technique MainTec4 < string MMDPass = "object"; bool UseTexture = false; bool UseSphereMap = false; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 Basic_VS(false, false, true);
        PixelShader  = compile ps_3_0 Basic_PS(false, false, true,
		ObjTexSampler);
    }
}

technique MainTec5 < string MMDPass = "object"; bool UseTexture = true; bool UseSphereMap = false; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 Basic_VS(true, false, true);
        PixelShader  = compile ps_3_0 Basic_PS(true, false, true,
		ObjTexSampler);
    }
}

technique MainTec6 < string MMDPass = "object"; bool UseTexture = false; bool UseSphereMap = true; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 Basic_VS(false, true, true);
        PixelShader  = compile ps_3_0 Basic_PS(false, true, true,
		ObjTexSampler);
    }
}

technique MainTec7 < string MMDPass = "object"; bool UseTexture = true; bool UseSphereMap = true; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 Basic_VS(true, true, true);
        PixelShader  = compile ps_3_0 Basic_PS(true, true, true,
		ObjTexSampler);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
// セルフシャドウ用Z値プロット

struct VS_ZValuePlot_OUTPUT {
    float4 Pos : POSITION;              // 射影変換座標
    float4 ShadowMapTex : TEXCOORD0;    // Zバッファテクスチャ
};

// 頂点シェーダ
VS_ZValuePlot_OUTPUT ZValuePlot_VS( float4 Pos : POSITION )
{
    VS_ZValuePlot_OUTPUT Out = (VS_ZValuePlot_OUTPUT)0;

    // ライトの目線によるワールドビュー射影変換をする
    Out.Pos = mul( Pos, LightWorldViewProjMatrix );

    // テクスチャ座標を頂点に合わせる
    Out.ShadowMapTex = Out.Pos;

    return Out;
}

// ピクセルシェーダ
float4 ZValuePlot_PS( float4 ShadowMapTex : TEXCOORD0 ) : COLOR
{
    // R色成分にZ値を記録する
    return float4(ShadowMapTex.z/ShadowMapTex.w,0,0,1);
}

// Z値プロット用テクニック
technique ZplotTec < string MMDPass = "zplot"; > {
    pass ZValuePlot {
        AlphaBlendEnable = FALSE;
        VertexShader = compile vs_3_0 ZValuePlot_VS();
        PixelShader  = compile ps_3_0 ZValuePlot_PS();
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////
// オブジェクト描画（セルフシャドウON）

// シャドウバッファのサンプラ。"register(s0)"なのはMMDがs0を使っているから
sampler DefSampler : register(s0);

struct BufferShadow_OUTPUT {
    float4 Pos      : POSITION;     // 射影変換座標
    float4 ZCalcTex : TEXCOORD0;    // Z値
    float2 Tex      : TEXCOORD1;    // テクスチャ
    float3 Normal   : TEXCOORD2;    // 法線
    float3 Eye      : TEXCOORD3;    // カメラとの相対位置
    float2 SpTex    : TEXCOORD4;	 // スフィアマップテクスチャ座標
    float4 Color    : COLOR0;       // ディフューズ色
};

// 頂点シェーダ
BufferShadow_OUTPUT BufferShadow_VS(float4 Pos : POSITION, float4 Normal : NORMAL, float2 Tex : TEXCOORD0, uniform bool useTexture, uniform bool useSphereMap, uniform bool useToon)
{
    BufferShadow_OUTPUT Out = (BufferShadow_OUTPUT)0;

    // カメラ視点のワールドビュー射影変換
    Out.Pos = mul( Pos, WorldViewProjMatrix );
    
    // カメラとの相対位置
    Out.Eye = CameraPosition - mul( Pos, WorldMatrix );
    // 頂点法線
    Out.Normal = normalize( mul( Normal, (float3x3)WorldMatrix ) );
	// ライト視点によるワールドビュー射影変換
    Out.ZCalcTex = mul( Pos, LightWorldViewProjMatrix );
    
    // ディフューズ色＋アンビエント色 計算
    Out.Color.rgb = AmbientColor;
    if ( !useToon ) {
        Out.Color.rgb += max(0,dot( Out.Normal, -LightDirection )) * DiffuseColor.rgb;
    }
    Out.Color.a = DiffuseColor.a;
    Out.Color = saturate( Out.Color );
    
    // テクスチャ座標
    Out.Tex = Tex;
    
    if ( useSphereMap ) {
        // スフィアマップテクスチャ座標
        float2 NormalWV = mul( Out.Normal, (float3x3)ViewMatrix );
        Out.SpTex.x = NormalWV.x * 0.5f + 0.5f;
        Out.SpTex.y = NormalWV.y * -0.5f + 0.5f;
    }
    
    return Out;
}

// ピクセルシェーダ
float4 BufferShadow_PS(BufferShadow_OUTPUT IN, uniform bool useTexture, uniform bool useSphereMap, uniform bool useToon
, uniform sampler DiffuseSamp
) : COLOR
{
    // ディフューズ色＋アンビエント色 計算
    IN.Color.rgb = AmbientColor;
    if ( !useToon ) {
        IN.Color.rgb += max(0,dot( IN.Normal, -LightDirection )) * DiffuseColor.rgb;
    }
    IN.Color.a = DiffuseColor.a;
    IN.Color = saturate( IN.Color );
    
    // スペキュラ色計算
    float3 HalfVector = normalize( normalize(IN.Eye) + -LightDirection );
    float3 Specular = pow( max(0,dot( HalfVector, normalize(IN.Normal) )), SpecularPower ) * SpecularColor;

    float4 Color = IN.Color;
    float4 ShadowColor = float4(AmbientColor, Color.a);  // 影の色
    if ( useTexture ) {
        // テクスチャ適用
        float4 TexColor = tex2D( DiffuseSamp, IN.Tex );
        Color *= TexColor;
        ShadowColor *= TexColor;
    }
    if ( useSphereMap ) {
        // スフィアマップ適用
        float4 TexColor = tex2D(ObjSphareSampler,IN.SpTex);
        if(spadd) {
            Color += TexColor;
            ShadowColor += TexColor;
        } else {
            Color *= TexColor;
            ShadowColor *= TexColor;
        }
    }
    // スペキュラ適用
    Color.rgb += Specular;
    
    // テクスチャ座標に変換
    IN.ZCalcTex /= IN.ZCalcTex.w;
    float2 TransTexCoord;
    TransTexCoord.x = (1.0f + IN.ZCalcTex.x)*0.5f;
    TransTexCoord.y = (1.0f - IN.ZCalcTex.y)*0.5f;
    
    if( any( saturate(TransTexCoord) != TransTexCoord ) ) {
        // シャドウバッファ外
        return Color;
    } else {
        float comp;
        if(parthf) {
            // セルフシャドウ mode2
            comp=1-saturate(max(IN.ZCalcTex.z-tex2D(DefSampler,TransTexCoord).r , 0.0f)*SKII2*TransTexCoord.y-0.3f);
        } else {
            // セルフシャドウ mode1
            comp=1-saturate(max(IN.ZCalcTex.z-tex2D(DefSampler,TransTexCoord).r , 0.0f)*SKII1-0.3f);
        }
        if ( useToon ) {
            // トゥーン適用
            comp = min(saturate(dot(IN.Normal,-LightDirection)*Toon),comp);
            ShadowColor.rgb *= MaterialToon;
        }
        
        float4 ans = lerp(ShadowColor, Color, comp);
		if( transp ) ans.a = 0.5f;
        return ans;
    }
}

// ピクセルシェーダ ノーマルマップ追加
float4 BufferShadow_PS_NormalMap(BufferShadow_OUTPUT IN, uniform bool useTexture, uniform bool useSphereMap, uniform bool useToon
, uniform sampler DiffuseSamp
, uniform sampler normalSamp) : COLOR0
{
	float3x3 tangentFrame = compute_tangent_frame(IN.Normal, IN.Eye, IN.Tex);
	float3 normal = normalize(mul(2.0f * tex2D(normalSamp, IN.Tex) - 1.0f, tangentFrame));
	
    IN.Color.rgb = AmbientColor;
    if ( !useToon ) {
        IN.Color.rgb += max(0,dot( normal, -LightDirection )) * DiffuseColor.rgb;
    }
    IN.Color.a = DiffuseColor.a;
    IN.Color = saturate( IN.Color );
    
	float2 NormalWV = mul( normal, (float3x3)ViewMatrix );
	IN.SpTex.x = NormalWV.x * 0.5f + 0.5f;
	IN.SpTex.y = NormalWV.y * -0.5f + 0.5f;
	
    // スペキュラ色計算
    float3 HalfVector = normalize( normalize(IN.Eye) + -LightDirection );
    float3 Specular = pow( max(0,dot( HalfVector, normalize(normal) )), SpecularPower ) * SpecularColor;

    float4 Color = IN.Color;
    float4 ShadowColor = float4(AmbientColor, Color.a);  // 影の色
    if ( useTexture ) {
        // テクスチャ適用
        float4 TexColor = tex2D( DiffuseSamp, IN.Tex );
        Color *= TexColor;
        ShadowColor *= TexColor;
    }
    if ( useSphereMap ) {
        // スフィアマップ適用
        float4 TexColor = tex2D(ObjSphareSampler,IN.SpTex);
        if(spadd) {
            Color += TexColor;
            ShadowColor += TexColor;
        } else {
            Color *= TexColor;
            ShadowColor *= TexColor;
        }
    }
    // スペキュラ適用
    Color.rgb += Specular;
    
    // テクスチャ座標に変換
    IN.ZCalcTex /= IN.ZCalcTex.w;
    float2 TransTexCoord;
    TransTexCoord.x = (1.0f + IN.ZCalcTex.x)*0.5f;
    TransTexCoord.y = (1.0f - IN.ZCalcTex.y)*0.5f;
    
    if( any( saturate(TransTexCoord) != TransTexCoord ) ) {
        // シャドウバッファ外
        return Color;
    } else {
        float comp;
        if(parthf) {
            // セルフシャドウ mode2
            comp=1-saturate(max(IN.ZCalcTex.z-tex2D(DefSampler,TransTexCoord).r , 0.0f)*SKII2*TransTexCoord.y-0.3f);
        } else {
            // セルフシャドウ mode1
            comp=1-saturate(max(IN.ZCalcTex.z-tex2D(DefSampler,TransTexCoord).r , 0.0f)*SKII1-0.3f);
        }
        if ( useToon ) {
            // トゥーン適用
            comp = min(saturate(dot(normal,-LightDirection)*Toon),comp);
            ShadowColor.rgb *= MaterialToon;
        }
        
        float4 ans = lerp(ShadowColor, Color, comp);
		if( transp ) ans.a = 0.5f;
        return ans;
    }
}


//-----------------------------------------------------------------------------------------------------
#define MAKE_TECH_BS_NORMAL_MAP_SET(SUBSET,NORMAL_MAP_NUM) \
TECH_BS_NORMAL_MAP_BASE(0,0,0,SUBSET,NORMAL_MAP_NUM) \
TECH_BS_NORMAL_MAP_BASE(1,0,0,SUBSET,NORMAL_MAP_NUM) \
TECH_BS_NORMAL_MAP_BASE(0,1,0,SUBSET,NORMAL_MAP_NUM) \
TECH_BS_NORMAL_MAP_BASE(1,1,0,SUBSET,NORMAL_MAP_NUM) \
TECH_BS_NORMAL_MAP_BASE(0,0,1,SUBSET,NORMAL_MAP_NUM) \
TECH_BS_NORMAL_MAP_BASE(1,0,1,SUBSET,NORMAL_MAP_NUM) \
TECH_BS_NORMAL_MAP_BASE(0,1,1,SUBSET,NORMAL_MAP_NUM) \
TECH_BS_NORMAL_MAP_BASE(1,1,1,SUBSET,NORMAL_MAP_NUM)

#undef TECH_BS_NORMAL_MAP_BASE
#define TECH_BS_NORMAL_MAP_BASE(TEX,SPH,TOON,SUBSET,NORMAL_MAP_NUM) \
technique NormalMap##NORMAL_MAP_NUM##_MainTecBS##TEX##SPH##TOON < \
		string Subset = SUBSET; \
		string MMDPass = "object_ss"; bool UseTexture = TEX; bool UseSphereMap = SPH; bool UseToon = TOON; > { \
	pass DrawObject { \
		VertexShader = compile vs_3_0 BufferShadow_VS(TEX, SPH, TOON); \
		PixelShader  = compile ps_3_0 BufferShadow_PS_NormalMap(TEX, SPH, TOON, \
		ObjTexSampler, NormalMapTexSampler##NORMAL_MAP_NUM ); \
    } \
}

// ノーマルマップ対応版 オブジェクト描画用テクニック生成
#ifdef USE_NORMAL_MAP_MATERIALS0
MAKE_TECH_BS_NORMAL_MAP_SET(USE_NORMAL_MAP_MATERIALS0,0)
#endif

//-----------------------------------------------------------------------------------------------------
// 標準エミュレート
// オブジェクト描画用テクニック（アクセサリ用）
technique MainTecBS0  < string MMDPass = "object_ss"; bool UseTexture = false; bool UseSphereMap = false; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 BufferShadow_VS(false, false, false);
        PixelShader  = compile ps_3_0 BufferShadow_PS(false, false, false,
		ObjTexSampler);
    }
}

technique MainTecBS1  < string MMDPass = "object_ss"; bool UseTexture = true; bool UseSphereMap = false; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 BufferShadow_VS(true, false, false);
        PixelShader  = compile ps_3_0 BufferShadow_PS(true, false, false,
		ObjTexSampler);
    }
}

technique MainTecBS2  < string MMDPass = "object_ss"; bool UseTexture = false; bool UseSphereMap = true; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 BufferShadow_VS(false, true, false);
        PixelShader  = compile ps_3_0 BufferShadow_PS(false, true, false,
		ObjTexSampler);
    }
}

technique MainTecBS3  < string MMDPass = "object_ss"; bool UseTexture = true; bool UseSphereMap = true; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 BufferShadow_VS(true, true, false);
        PixelShader  = compile ps_3_0 BufferShadow_PS(true, true, false,
		ObjTexSampler);
    }
}

// オブジェクト描画用テクニック（PMDモデル用）
technique MainTecBS4  < string MMDPass = "object_ss"; bool UseTexture = false; bool UseSphereMap = false; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 BufferShadow_VS(false, false, true);
        PixelShader  = compile ps_3_0 BufferShadow_PS(false, false, true,
		ObjTexSampler);
    }
}

technique MainTecBS5  < string MMDPass = "object_ss"; bool UseTexture = true; bool UseSphereMap = false; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 BufferShadow_VS(true, false, true);
        PixelShader  = compile ps_3_0 BufferShadow_PS(true, false, true,
		ObjTexSampler);
    }
}

technique MainTecBS6  < string MMDPass = "object_ss"; bool UseTexture = false; bool UseSphereMap = true; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 BufferShadow_VS(false, true, true);
        PixelShader  = compile ps_3_0 BufferShadow_PS(false, true, true,
		ObjTexSampler);
    }
}

technique MainTecBS7  < string MMDPass = "object_ss"; bool UseTexture = true; bool UseSphereMap = true; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 BufferShadow_VS(true, true, true);
        PixelShader  = compile ps_3_0 BufferShadow_PS(true, true, true,
		ObjTexSampler);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
