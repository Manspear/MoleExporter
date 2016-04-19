#pragma once

#include <vector>
#include <assert.h>
#include <iostream>
#include <string>
#include "fbxsdk.h"
#include <fstream>

#include "HeaderData.h"

using namespace std;

#define DELTA 0.0001
#define EQUAL(A,B) (abs((A)-(B)) < DELTA) ? true:false

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

	struct sBlendData
	{
		int jointID, controlPointIndex;
		float blendingWeight;
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

	sBlendData* findBlendDataForControlPoint(std::vector<sBlendData>& inputVector, unsigned int controlPointIndex);

	void assignToHeaderData();

	void WriteToBinary();

	void convertFbxMatrixToFloatArray(FbxAMatrix inputMatrix, float inputArray[16]);

	/*Lists*/
	std::vector<sImportMeshData> mTempMeshList;
	
	//**
	sImportMeshData importMeshData;


	/*Struct objects*/
	sMainHeader mainHeader;
	sVertex vertex;
	sMaterial materialData;
	sLight lightData;
	sDirectionalPoint dirPointData;
	sSpotLight spotData;
	sCamera camData;

	/*Constructor and destructor*/
	FbxImport();
	~FbxImport();

private:

	FbxNode* pmRootNode;
	fbxsdk::FbxManager* pmManager;
	FbxIOSettings* pmSettings;
	FbxScene* pmScene;

	int meshCounter;
	int materialCounter;
	int textureCounter;
	int cameraCounter;
	int lightCounter;

	bool firstProcess;
};



