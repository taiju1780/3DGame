#pragma once
#include <vector>

struct PMDHeader {
	char magic[3]; //"pmd"
	float version;
	char model_name[20];
	char comment[256];
};

#pragma pack(1)
struct PMDvertex {
	float pos[3];				// x, y, z // 座標 
	float normal_vec[3];		// nx, ny, nz // 法線ベクトル 
	float uv[2];				// u, v // UV座標 // MMDは頂点UV 
	unsigned short bone_num[2]; // ボーン番号1、番号2 // モデル変形(頂点移動)時に影響
	unsigned char weight;
	unsigned char edge;
};
#pragma pack()

struct PMDMaterial
{
	float diffuse_color[3];		//rgb
	float alpha;
	float specular;
	float specular_color[3];	//rgb
	float mirror_color[3];		//rgb
	unsigned char toon_index;	//toon.bmp
	unsigned char edge_flag;	//輪郭、影
	//ここまで４６バイト
	unsigned int face_vert_count;	//面積点数
	char texture_file_name[20];		//テクスチャファイル名

};

class PMDModel
{
private:
	std::vector<PMDvertex> _verticesData;
	std::vector<unsigned short> _indexData;

	void InitModel(const char * filepath);

public:
	PMDModel(const char * filepath);
	~PMDModel();

	std::vector<PMDvertex> GetverticesData();
	std::vector<unsigned short> GetindexData();
};

