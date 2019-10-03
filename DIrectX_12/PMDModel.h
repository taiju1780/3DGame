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
	float pos[3];				// x, y, z // ���W 
	float normal_vec[3];		// nx, ny, nz // �@���x�N�g�� 
	float uv[2];				// u, v // UV���W // MMD�͒��_UV 
	unsigned short bone_num[2]; // �{�[���ԍ�1�A�ԍ�2 // ���f���ό`(���_�ړ�)���ɉe��
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
	unsigned char edge_flag;	//�֊s�A�e
	//�����܂łS�U�o�C�g
	unsigned int face_vert_count;	//�ʐϓ_��
	char texture_file_name[20];		//�e�N�X�`���t�@�C����

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

