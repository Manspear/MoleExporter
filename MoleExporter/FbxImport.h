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

	struct sImportKeyFrame
	{
		float keyTime;
		float keyPos[3];
		float keyRotate[3];
		float keyScale[3];
	};

	struct sImportAnimationState
	{
		std::vector<sImportKeyFrame> keyList;
	};

	struct sImportJointData
	{
		//Name helps with debugging
		const char* name;

		int jointID;
		int parentJointID;

		float pos[3];
		float rot[3];
		float scale[3];

		float bindPoseInverse[16];
		float globalBindPoseInverse[16];
		std::vector<sImportAnimationState> animationState;
	};

	struct sImportMeshData
	{
		unsigned int materialID;

		float translate[3];
		float rotation[3];
		float scale[3];

		bool isAnimated;
		vector<sVertex> mVertexList;
		vector<sSkelAnimVertex> mSkelVertexList;
		std::vector<sImportJointData> jointList;
	};

	/*struct sImportAnimMeshData
	{
		unsigned int materialID;

		float translate[3];
		float rotation[3];
		float scale[3];

		vector<sSkelAnimVertex> mVertexList;
	};*/

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

	void readFromBinary();

	void convertFbxMatrixToFloatArray(FbxAMatrix inputMatrix, float inputArray[16]);

	/*Lists*/
	std::vector<sImportMeshData> mTempMeshList;
	//want to have mTempMeshList 
	
	//**
	sImportMeshData importMeshData;




	/*Constructor and destructor*/
	FbxImport();
	~FbxImport();

private:

	//Initially filled inside the initlalizeImporter function.
	//You fill this with the processJointHierarchy-function. You fill this some more in
	//processJoints. In there you find the "current joint" by using findJointIndexByName()
	std::vector<sImportJointData> pmSceneJoints;

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

	void processJointHierarchy(FbxNode* inputRoot);
	void recursiveJointHierarchyTraversal(FbxNode* inNode, int currIndex, int inNodeParentIndex);
	unsigned int findJointIndexByName(const char* jointName);
};


