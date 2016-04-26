#pragma once
#include <vector>

struct sMainHeader
{
	unsigned int meshCount;
	unsigned int materialCount;
	unsigned int lightCount;
	unsigned int cameraCount;
};

struct sMesh
{
	unsigned int materialID;

	float translate[3];
	float rotation[3];
	float scale[3];

	bool isBoundingBox;
	bool isAnimated;

	unsigned int vertexCount;
	//unsigned int skelAnimVertexCount;
	//unsigned int jointCount;
};

static std::vector<sMesh> meshList;

struct sVertex
{
	float vertexPos[3];
	float vertexNormal[3];
	float vertexUV[2];
	float tangentNormal[3];
	float biTangentNormal[3];
};

struct m
{
	std::vector<sVertex> vList;
};

static std::vector<m> mList;

struct sSkelAnimVertex
{
	float vertexPos[3];
	float vertexNormal[3];
	float vertexUV[2];
	float tangentNormal[3];
	float biTangentNormal[3];

	/*Set to -1337 in FbxImport::processVertices(FbxMesh * inputMesh)
	to say "this is not a set value"*/
	int influences[4];
	float weights[4];
};

struct sJoint
{
	int jointID;
	int parentJointID;
	int bBoxID;

	float pos[3];
	float rot[3];
	float scale[3];

	float bindPoseInverse[16];
	float globalBindPoseInverse[16];
	int animationStateCount;
};

struct sAnimationState
{
	int keyFrames;
};

struct sKeyFrame
{
	float keyTime;
	float keyPos[3];
	float keyRotate[3];
	float keyScale[3];
	
};

struct sMaterial
{
	const char* materialName;

	float ambientColor[3];
	float diffuseColor[3];
	float specularColor[3];

	float shinyFactor;

	wchar_t* diffuseTexture;
	wchar_t* specularTexture;
	wchar_t* normalTexture;
};

static std::vector<sMaterial> materialList;

struct sLight
{
	int lightID; 

	float lightPos[3];
	float lightRot[3];
	float lightScale[3];

	float color[3];
	float intensity;
};

static std::vector<sLight> mLightList;

struct sCamera
{
	float camPos[3];
	float upVector[3];

	float fieldOfView;
	float nearPlane;
	float farPlane;
};

static std::vector<sCamera> mCameraList;

/*Struct objects*/
sMainHeader static mainHeader;
sMaterial static materialData;
sLight static lightData;
sCamera static camData;