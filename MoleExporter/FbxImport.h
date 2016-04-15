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

	/*Structs*/

	struct sTempMesh
	{
		std::vector<sVertexData> mVertexList;
		std::vector<sTransformData> mTransformList;
	};

	/*Functions*/

	void initializeImporter(const char* filePath);
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

	void convertFbxMatrixToFloatArray(FbxAMatrix inputMatrix, float inputArray[16]);

	/*Lists*/
	std::vector<sTempMesh> mMeshList;

	std::vector<sMaterialData> mMaterialList;
	std::vector<sCamData> mCameraList;

	std::vector<sLightData> mLightList;
	std::vector<sDirectionalPoint> mDirPointList;
	std::vector<sSpotLight> mSpotList;

	/*Struct objects*/
	sMainHeader headerData;

	sMeshData meshData;
	sTempMesh meshTempData;

	sVertexData vertexData;
	sTransformData transformData;
	sMaterialData materialData;

	sLightData lightData;
	sDirectionalPoint dirPointData;
	sSpotLight spotData;

	sCamData camData;

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



