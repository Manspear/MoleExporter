#pragma once

struct sMainHeader
{
	unsigned int meshCount;
	unsigned int lightCount;
	unsigned int cameraCount;
};

struct sVertexData
{
	float vertexPos[3];
	float vertexNormal[3];
	float tangentNormal[3];
	float biTangentNormal[3];
	float vertexUV[2];
};

struct sTransformData
{
	float translate[3];
	float rotation[3];
	float scale[3];

};

struct sMeshData
{
	unsigned int materialID;

	unsigned int vertexCount;
	unsigned int materialCount;
};

struct sVertexAnimData
{
	float vertexPos[3];
	float vertexNormal[3];
	float tangentNormal[3];
	float biTangentNormal[3];
	float vertexUV[2];
	float influences[4];
	float weights[4];
};

struct sMaterialData
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

struct sLightData
{
	unsigned int countDirectionalPoint;
	unsigned int countSpotlight; 
};

struct sDirectionalPoint
{
	float color[3];
	float intensity;

	float lightPos[3];
	float lightRot[3];
	float lightScale[3];
};

struct sSpotLight
{
	float color[3];
	float intensity;
	float innerAngle;
	float outerAngle;

	float lightPos[3];
	float lightRot[3];
	float lightScale[3];
};


struct sCamData
{
	float camPos[3];
	float upVector[3];

	float fov;
	float nearPlane;
	float farPlane;
};
