#include "PMDModel.h"



PMDModel::PMDModel(const char * filepath)
{
	InitModel(filepath);
}


PMDModel::~PMDModel()
{
}

std::vector<PMDvertex> PMDModel::GetverticesData()
{
	return _verticesData;
}

std::vector<unsigned short> PMDModel::GetindexData()
{
	return _indexData;
}

void PMDModel::InitModel(const char * filepath)
{
	FILE *fp;

	fopen_s(&fp, filepath, "rb");

	//ヘッダ読み込み
	PMDHeader pmdheader = {};
	fread(&pmdheader.magic, sizeof(pmdheader.magic), 1, fp);
	fread(&pmdheader.version, sizeof(pmdheader) - sizeof(pmdheader.magic) - 1, 1, fp);

	//頂点
	unsigned int Vnum = 0;
	fread(&Vnum, sizeof(Vnum), 1, fp);

	_verticesData.resize(Vnum);

	for (auto i = 0; i < Vnum; ++i) {
		fread(&_verticesData[i], sizeof(PMDvertex), 1, fp);
	}

	//インデックス
	unsigned int IdxNum = 0;
	fread(&IdxNum, sizeof(IdxNum), 1, fp);

	_indexData.resize(IdxNum);
	for (auto i = 0; i < IdxNum; ++i) {
		fread(&_indexData[i], sizeof(unsigned short), 1, fp);
	}
}
