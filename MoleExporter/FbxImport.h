#pragma once

#include <vector>
#include <assert.h>
#include <iostream>
#include <string>
#include "fbxsdk.h"

#include "HeaderData.h"

using namespace std;

class FbxImport
{
public:
	/*Functions*/

	void initializeImporter();
	void processMesh(FbxMesh* inputMesh);

	void processVertices(FbxMesh* inputMesh);
	void processNormals(FbxMesh* inputMesh);
	void processTangents(FbxMesh* inputMesh);
	void processBiTangents(FbxMesh* inputMesh);
	void processUVs(FbxMesh* inputMesh);
	void processMaterials(FbxMesh* inputMesh);

	void processTextures(FbxMesh* inputMesh);
	void processDiffuseMaps(FbxProperty inputProp);
	void processSpecularMaps(FbxProperty inputProp);
	void processNormalMaps(FbxProperty inputProp);

	void processTransformations(FbxMesh* inputMesh);

	void processLight(FbxLight* inputLight);
	void processCamera(FbxCamera* inputCamera);

	bool checkMaterialName(const char* materialName);

	std::vector<sMeshData> mMeshList;
	std::vector<sLightData> mLightList;
	std::vector<sCamData> mCameraList;
	std::vector<sMaterialData> mMaterialList;

	sMainHeader headerData;

	sVertexData vertexData;
	sTransformData transformData;
	sMaterialData materialData;
	sMeshData meshData;
	sLightData lightData;
	sCamData camData;

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



