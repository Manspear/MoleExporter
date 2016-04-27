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
	char meshName[256];

	unsigned int materialID;

	float translate[3];
	float rotation[3];
	float scale[3];

	bool isBoundingBox;
	bool isAnimated;

	unsigned int vertexCount = 0;
	unsigned int skelAnimVertexCount = 0;
	unsigned int jointCount = 0;
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

struct mk
{
	std::vector<sSkelAnimVertex> vskList;
};

static std::vector<mk> mkList;

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

static std::vector<sJoint> jointList;

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

static std::vector<sKeyFrame> keyframeList;

struct sMaterial
{
	char materialName[256];

	float ambientColor[3];
	float diffuseColor[3];
	float specularColor[3];

	float shinyFactor;

	char diffuseTexture[256];
	char specularTexture[256];
	char normalTexture[256];
};

static std::vector<sMaterial> mMaterialList;

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
sMainHeader static gMainHeader;
sMaterial static gMaterialData;
sLight static gLightData;
sCamera static gCamData;
