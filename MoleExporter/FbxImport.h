#pragma once

#include <vector>
#include <assert.h>
#include <iostream>
#include <string>
#include "fbxsdk.h"
#include <fstream>

#include "HeaderData.h"

using namespace std;

class FbxImport
{
public:

	/*Structs*/

	struct sImportMeshData
	{
		unsigned int materialID;

		float translate[3];
		float rotation[3];
		float scale[3];

		vector<sVertex> mVertexList;
	};

	struct sImportAnimMeshData
	{
		unsigned int materialID;

		float translate[3];
		float rotation[3];
		float scale[3];

		vector<sSkelAnimVertex> mVertexList;
	};

	/*Functions*/

	void initializeImporter();
	void processMesh(FbxMesh* inputMesh);

	void processVertices(FbxMesh* inputMesh);
	void processNormals(FbxMesh* inputMesh);
	void processTangents(FbxMesh* inputMesh);
	void processBiTangents(FbxMesh* inputMesh);
	void processUVs(FbxMesh* inputMesh);
	void processMaterials(FbxMesh* inputMesh);
	void processJoints(FbxMesh* inputMesh);

	void processTextures(FbxMesh* inputMesh);
	void processDiffuseMaps(FbxProperty inputProp);
	void processSpecularMaps(FbxProperty inputProp);
	void processNormalMaps(FbxProperty inputProp);

	void processTransformations(FbxMesh* inputMesh);

	void processLight(FbxLight* inputLight);
	void processCamera(FbxCamera* inputCamera);

	bool checkMaterialName(const char* materialName);

	void assignToHeaderData();

	void WriteToBinary();

	/*Lists*/
	std::vector<sImportMeshData> mTempMeshList;
	std::vector<sMesh> mSMeshList;
	std::vector<sMaterial> mMaterialList;
	std::vector<sDirectionalPoint> mDirPointList;
	std::vector<sSpotLight> mSpotList;
	std::vector<sCamera> mCameraList;

	/*Struct objects*/
	sMainHeader headerData;
	sImportMeshData importMeshData;
	sVertex vertexData;
	sMaterial materialData;
	sLight lightData;
	sDirectionalPoint dirPointData;
	sSpotLight spotData;
	sCamera camData;

	/*Pointer to memory arrays where the vector store its owned elements*/
	sVertex *vertexArray;
	sMaterial *materialArray;
	sDirectionalPoint *dirPointArray;
	sSpotLight *spotlightArray;
	sCamera *cameraArray;

	/*Constructor and destructor*/
	FbxImport();
	~FbxImport();

private:

	FbxNode* pmRootNode;
	FbxManager* pmManager;
	FbxIOSettings* pmSettings;
	FbxScene* pmScene;

	int meshCounter;
	int materialCounter;
	int textureCounter;
	int cameraCounter;
	int lightCounter;

	bool firstProcess;
};



