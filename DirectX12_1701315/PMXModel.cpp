#include "PMXModel.h"
#include <iostream>
#include <DirectXMath.h>
#include <DirectXTex.h>
#include <Windows.h>
#include "d3dx12.h"
#include <d3dcompiler.h>
#include <vector>
#include<Shlwapi.h> 

//リンク
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment (lib,"d3dcompiler.lib")
#pragma comment(lib,"DirectXTex.lib")
#pragma comment(lib,"shlwapi.lib")


void PMXModel::LoadModel(const char * filepath, ID3D12Device* _dev)
{
	FILE* fp;
	fopen_s(&fp, filepath, "rb");

	PMXHeader pmxH = {};
	fread(&pmxH.magic, sizeof(pmxH.magic), 1, fp);
	fread(&pmxH.version, sizeof(pmxH.version), 1, fp);
	fread(&pmxH.bitesize, sizeof(pmxH.bitesize), 1, fp);
	fread(&pmxH.data, sizeof(pmxH.data), 1, fp);

	ModelInfo modelI = {};
	fread(&modelI.ModelNamesize, sizeof(modelI.ModelNamesize), 1, fp);
	fseek(fp, modelI.ModelNamesize, SEEK_CUR);

	fread(&modelI.ModelNameEsize, sizeof(modelI.ModelNameEsize), 1, fp);
	fseek(fp, modelI.ModelNameEsize, SEEK_CUR);

	fread(&modelI.Commentsize, sizeof(modelI.Commentsize), 1, fp);
	fseek(fp, modelI.Commentsize, SEEK_CUR);

	fread(&modelI.CommentEsize, sizeof(modelI.CommentEsize), 1, fp);
	fseek(fp, modelI.CommentEsize, SEEK_CUR);

	unsigned int Vnum = 0;
	fread(&Vnum, sizeof(Vnum), 1, fp);

	_verticesData.resize(Vnum);

	//waitclassの値
	int adduvnum = pmxH.data[1];
	int vertidxsize = pmxH.data[2];
	int texidxsize = pmxH.data[3];
	int bornidxsize = pmxH.data[5];

	for (auto i = 0; i < Vnum; ++i) {
		//ポジション
		fread(&_verticesData[i].pos, sizeof(_verticesData[i].pos), 1, fp);

		//法線
		fread(&_verticesData[i].normal, sizeof(_verticesData[i].normal), 1, fp);

		//UV
		fread(&_verticesData[i].uv, sizeof(_verticesData[i].uv), 1, fp);

		//追加uv
		for (int u = 0; u < adduvnum; ++u) {
			fread(&_verticesData[i].addUv[u], sizeof(_verticesData[i].addUv[u]), 1, fp);
		}
		
		//waitclass
		fread(&_verticesData[i].waitclass, sizeof(_verticesData[i].waitclass), 1, fp);
		
		int waitnum = _verticesData[i].waitclass;

		//ボーン
		if (waitnum == 0) {
			fread(&_verticesData[i].bone[0], bornidxsize, 1, fp);
		}
		else if (waitnum == 1) {
			
			fread(&_verticesData[i].bone[0], bornidxsize, 1, fp);
			fread(&_verticesData[i].bone[1], bornidxsize, 1, fp);
			fread(&_verticesData[i].wait[0], sizeof(_verticesData[i].wait[0]), 1, fp);
		}
		else if (waitnum == 2) {
			for (int bw = 0; bw < 4; ++bw) {
				fread(&_verticesData[i].bone[bw], bornidxsize, 1, fp);
				fread(&_verticesData[i].wait[bw], sizeof(_verticesData[i].wait[bw]), 1, fp);
			}
		}
		else if (waitnum == 3) {
			fread(&_verticesData[i].bone[0], bornidxsize, 1, fp);
			fread(&_verticesData[i].bone[1], bornidxsize, 1, fp);
			fread(&_verticesData[i].wait[0], sizeof(_verticesData[i].wait[0]), 1, fp);
			for (int s = 0; s < 3; ++s) {
				fread(&_verticesData[i].sdefvec[s], sizeof(_verticesData[i].sdefvec[s]), 1, fp);
			}
		}
		fread(&_verticesData[i].edge, sizeof(_verticesData[i].edge), 1, fp);
	}

	unsigned int idxNum = 0;
	fread(&idxNum, sizeof(idxNum), 1, fp);

	_indexData.resize(idxNum);

	for (int i = 0; i < idxNum; ++i) {
		fread(&_indexData[i], vertidxsize, 1, fp);
	}

	unsigned int textureNum;
	fread(&textureNum, sizeof(textureNum), 1, fp);
	_texpath.resize(textureNum);

	for (int t = 0; t < textureNum; ++t) {

		unsigned int pathlength = 0;
		fread(&pathlength, sizeof(pathlength), 1, fp);

		std::string str;

		for (int i = 0; i < pathlength /2; ++i) {

			wchar_t c;
			fread(&c, sizeof(wchar_t), 1, fp);
			str += c;
		}
		_texpath[t] = str;
	}

	unsigned int MatNum = 0;
	fread(&MatNum, sizeof(MatNum), 1, fp);

	_MatData.resize(MatNum);

	for (auto &mat : _MatData) {
		unsigned int bytenum = 0;
		fread(&bytenum, sizeof(bytenum), 1, fp);
		fseek(fp, bytenum, SEEK_CUR);

		fread(&bytenum, sizeof(bytenum), 1, fp);
		fseek(fp, bytenum, SEEK_CUR);

		fread(&mat.diffuse, sizeof(mat.diffuse), 1, fp);
		fread(&mat.specular, sizeof(mat.specular), 1, fp);
		fread(&mat.specPower, sizeof(mat.specPower), 1, fp);
		fread(&mat.ambient, sizeof(mat.ambient), 1, fp);

		fread(&mat.bitFlag, sizeof(mat.bitFlag), 1, fp);
		fread(&mat.edgeColer, sizeof(mat.edgeColer), 1, fp);
		fread(&mat.edgesize, sizeof(mat.edgesize), 1, fp);

		fread(&mat.textureIndex, texidxsize, 1, fp);
		fread(&mat.sphIndex, texidxsize, 1, fp);

		fread(&mat.sphmode, sizeof(mat.sphmode), 1, fp);
		fread(&mat.toonflag, sizeof(mat.toonflag), 1, fp);

		if (mat.toonflag == 0) {
			fread(&mat.toonidx, texidxsize, 1, fp);
		}
		else {
			fread(&mat.toonidx, sizeof(unsigned char), 1, fp);
		}

		fread(&bytenum, sizeof(bytenum), 1, fp);
		fseek(fp, bytenum, SEEK_CUR);

		fread(&mat.face_vert_cnt, sizeof(mat.face_vert_cnt), 1, fp);
	}

	fclose(fp);
}

PMXModel::PMXModel(const char * filepath,ID3D12Device* _dev)
{
	LoadModel(filepath, _dev);
}


PMXModel::~PMXModel()
{
}
