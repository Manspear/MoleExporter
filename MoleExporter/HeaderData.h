#pragma once

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

	unsigned int vertexCount;
	/*unsigned int skelAnimVertexCount;*/
	/*unsigned int jointCount;*/
};

struct sVertex
{
	float vertexPos[3];
	float vertexNormal[3];
	float vertexUV[2];
	float tangentNormal[3];
	float biTangentNormal[3];
	
};

struct sSkelAnimVertex
{
	float vertexPos[3];
	float vertexNormal[3];
	float vertexUV[2];
	float tangentNormal[3];
	float biTangentNormal[3];
	float influences[4];
	float weights[4];
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

struct sLight
{
	unsigned int directionalPointCount;
	unsigned int spotlightCount; 
};

struct sDirectionalPoint
{
	float lightPos[3];
	float lightRot[3];
	float lightScale[3];

	float color[3];
	float intensity;
};

struct sSpotLight
{
	float lightPos[3];
	float lightRot[3];
	float lightScale[3];

	float color[3];
	float intensity;

	float innerAngle;
	float outerAngle;
};

struct sCamera
{
	float camPos[3];
	float upVector[3];

	float fieldOfView;
	float nearPlane;
	float farPlane;
};
