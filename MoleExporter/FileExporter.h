#pragma once
#include "FbxImport.h"

class FileExporter {

public:

	struct sMesh
	{
		unsigned int materialIndex; //So that several meshes may reuse the same material settings.
		unsigned int vertexCount;
		unsigned int jointCount;
		float worldPos[16];
	};

	struct sMaterialData
	{
		unsigned int materialIndex;
		float ambientColor[3];
		float diffuseColor[3];
		
		float specularColor[3];
		float cosinePower;

		char diffuseTexture[256];
		char specularTexture[256];
		char normalTexture[256];
	};

	struct sVertexData
	{
		float vertexPos[3];
		float vertexNormal[3];
		float tangentNormal[3];
		float biTangentNormal[3];
		float vertexUV[2];
		unsigned int boneInfluences[4];
		float boneWeights[4];
	};

	struct sJoint
	{
		unsigned int jointIndex;
		float bindPose[16];
		float inverseBindPose[16];
		unsigned int keyCount;
		unsigned int animationCount;
	};

	struct sAnimation
	{
		unsigned int startKeyIndex; //can calculate number of keys based on start and end key
		unsigned int endKeyIndex;
	};

	struct sKey
	{
		unsigned int keyIndex;
		float keyTransform[16];
		float keyTime;
	};



	

	FileExporter();
	~FileExporter();
private:


};