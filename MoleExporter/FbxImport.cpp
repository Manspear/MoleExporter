#include "FbxImport.h"
#include "HeaderData.h"

FbxImport::~FbxImport()
{
}

FbxImport::FbxImport()
{
	meshCounter = 1;
	materialCounter = 1;
	textureCounter = 0;
	cameraCounter = 1;
	lightCounter = 1;

	firstProcess = true;
}


void FbxImport::initializeImporter()
{
	/*Initialize memory allocator.*/
	pmManager = FbxManager::Create();

	/*Initialize settings for the import.*/
	pmSettings = FbxIOSettings::Create(pmManager, IOSROOT);

	/*Settings the options manually, by default it is true.*/
	pmSettings->SetBoolProp(IMP_FBX_MATERIAL, true);
	pmSettings->SetBoolProp(IMP_FBX_TEXTURE, true);
	pmSettings->SetBoolProp(IMP_FBX_LINK, true);
	pmSettings->SetBoolProp(IMP_FBX_SHAPE, true);
	pmSettings->SetBoolProp(IMP_FBX_GOBO, true);
	pmSettings->SetBoolProp(IMP_FBX_ANIMATION, true);
	pmSettings->SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);

	/*Setting the importing settings.*/
	pmManager->SetIOSettings(pmSettings);

	FbxImporter* pImporter = FbxImporter::Create(pmManager, "");

	bool importStatus = pImporter->Initialize("Models/test.fbx", -1, pmManager->GetIOSettings());
	/*Abort if the importer can't be intitialized.*/
	if (importStatus == false) {
		FBXSDK_printf("Error: Can't initialize the importer. Aborting...\n");
		exit(1);
	}

	FbxScene* pScene = FbxScene::Create(pmManager, "MyScene");

	importStatus = pImporter->Import(pScene);
	/*Abort if the scene can't be imported.*/
	if (importStatus == false) {
		FBXSDK_printf("Error: Can't import the scene. Aborting...\n");
		exit(1);
	}
	pImporter->Destroy();

	/*Get the handle to all of the objects in the scene.*/
	pmRootNode = pScene->GetRootNode();

	for (int childIndex = 0; childIndex < pmRootNode->GetChildCount(); childIndex++)
	{
		/*Getting the child nodes in the scene and their node attribute types.*/
		FbxNode* childNode = pmRootNode->GetChild(childIndex);
		FbxNodeAttribute::EType attributeType = childNode->GetNodeAttribute()->GetAttributeType();

		/*No attributes, then skip!*/
		if (childNode->GetNodeAttribute() == NULL)
			continue;

		/*To make sure we only want to import meshes, lights and cameras.*/
		if (attributeType != FbxNodeAttribute::eMesh && attributeType != FbxNodeAttribute::eLight &&
			attributeType != FbxNodeAttribute::eCamera)
			continue;

		if (attributeType == FbxNodeAttribute::eMesh)
		{
			cout << "\n" << "Object nr: " << meshCounter << " Name: " << childNode->GetName() << "\n";

			processMesh((FbxMesh*)childNode->GetNodeAttribute());
			meshCounter += 1;

			/*headerData.meshCount = mMeshList.size(); */
		}

		if (attributeType == FbxNodeAttribute::eLight)
		{
			processLight((FbxLight*)childNode->GetNodeAttribute());

			/*headerData.lightCount = mDirPointList.size() + mSpotList.size();*/
		}

		if (attributeType == FbxNodeAttribute::eCamera)
		{
			processCamera((FbxCamera*)childNode->GetNodeAttribute());

			/*headerData.cameraCount = mCameraList.size();*/
		}
	}

	assignToHeaderData();

}

void FbxImport::processMesh(FbxMesh * inputMesh)
{
	importMeshData = sImportMeshData();

	processVertices(inputMesh);

	processNormals(inputMesh);

	processTangents(inputMesh);

	processBiTangents(inputMesh);

	processUVs(inputMesh);

	processMaterials(inputMesh);

	processTextures(inputMesh);

	processTransformations(inputMesh);

	/*meshData.vertexCount = meshTempData.mVertexList.size();

	headerData.materialCount = mMaterialList.size();*/

	mTempMeshList.push_back(importMeshData);
}

void FbxImport::processVertices(FbxMesh * inputMesh)
{
	/*Array of the control points of mesh.*/
	FbxVector4* vertices = inputMesh->GetControlPoints();

	for (int i = 0; i < inputMesh->GetPolygonCount(); i++)
	{
		/*Getting vertices of a polygon in the mesh.*/
		int numPolygonVertices = inputMesh->GetPolygonSize(i);

		/*If the mesh is not triangulated, meaning that there are quads in the mesh,
		then the program should abort, terminating the process.*/
		assert(numPolygonVertices == 3);

		for (int j = 0; j < numPolygonVertices; j++)
		{
			/*Getting the index to a control point "vertex".*/
			int polygonVertex = inputMesh->GetPolygonVertex(i, j);

			vertexData.vertexPos[0] = (float)vertices[polygonVertex].mData[0];
			vertexData.vertexPos[1] = (float)vertices[polygonVertex].mData[1];
			vertexData.vertexPos[2] = (float)vertices[polygonVertex].mData[2];

			cout << "\n" << "Position: " << (float)vertices[polygonVertex].mData[0] << " " <<
				(float)vertices[polygonVertex].mData[1] << " " <<
				(float)vertices[polygonVertex].mData[1] << "\n";

			importMeshData.mVertexList.push_back(vertexData);
		}
	}
}

void FbxImport::processNormals(FbxMesh * inputMesh)
{
	/*Get the normal element of the mesh.*/
	FbxGeometryElementNormal* normalElement = inputMesh->GetElementNormal();

	if (normalElement) /*If there is normal element then proceed.*/
	{
		/*Obtain normal of each vertex.*/
		if (normalElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
		{
			/*Obtain the normals of each vertex, because the mapping mode of the normal element is by control point.*/
			for (int vertexIndex = 0; vertexIndex < inputMesh->GetControlPointsCount(); vertexIndex++)
			{
				int normalIndex = 0;

				/*If reference mode is direct, it means that the normal index is the same as a vertex index.*/
				if (normalElement->GetReferenceMode() == FbxGeometryElement::eDirect)
				{
					normalIndex = vertexIndex;
				}

				/*If the reference mode is Index-to-Direct, it means that the normals are obtained by the Index-to-Direct.*/
				if (normalElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
				{
					normalIndex = normalElement->GetIndexArray().GetAt(normalIndex);
				}

				/*Normals of each vertex is obtained.*/
				FbxVector4 normals = normalElement->GetDirectArray().GetAt(normalIndex);

				cout << "\n" << "Normal: " << normals.mData[0] << " " << normals.mData[1] << " " << normals.mData[2] << "\n";

				importMeshData.mVertexList.at(vertexIndex).vertexNormal[0] = normals.mData[0];
				importMeshData.mVertexList.at(vertexIndex).vertexNormal[1] = normals.mData[1];
				importMeshData.mVertexList.at(vertexIndex).vertexNormal[2] = normals.mData[2];
			}
		}

		/*Get the normals by obtaining polygon-vertex.*/
		else if (normalElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
		{
			int indexPolygonVertex = 0;

			/*Obtain normals of each polygon, because the mapping mode of normal element is by Polygon-Vertex.*/
			for (int polygonIndex = 0; polygonIndex < inputMesh->GetPolygonCount(); polygonIndex++)
			{
				/*Get the polygon size, to know how many vertices in current polygon.*/
				int polygonSize = inputMesh->GetPolygonSize(polygonIndex);

				for (int i = 0; i < polygonSize; i++) //Obtain each vertex of the current polygon.
				{
					int normalIndex = 0;

					/*Reference mode is direct because the normal index is same as indexPolygonVertex.*/
					if (normalElement->GetReferenceMode() == FbxGeometryElement::eDirect)
					{
						normalIndex = indexPolygonVertex;
					}
					/*Reference mose is index-to-direct, which means getting normals by index-to-direct.*/
					if (normalElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
					{
						normalIndex = normalElement->GetIndexArray().GetAt(indexPolygonVertex);
					}

					FbxVector4 normals = normalElement->GetDirectArray().GetAt(normalIndex); //Obtain normals of each polygon-vertex

					cout << "\n" << "Normal: " << normals.mData[0] << " " << normals.mData[1] << " " << normals.mData[2] << "\n";

					importMeshData.mVertexList.at(indexPolygonVertex).vertexNormal[0] = normals.mData[0];
					importMeshData.mVertexList.at(indexPolygonVertex).vertexNormal[1] = normals.mData[1];
					importMeshData.mVertexList.at(indexPolygonVertex).vertexNormal[2] = normals.mData[2];

					indexPolygonVertex++;
				}
			}
		}
	}
}

void FbxImport::processTangents(FbxMesh * inputMesh)
{
	int tangentCount = inputMesh->GetElementTangentCount();

	for (int i = 0; i < tangentCount; i++)
	{
		FbxGeometryElementTangent* tangentElement = inputMesh->GetElementTangent(i);

		if (tangentElement)
		{
			if (tangentElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
			{
				for (int vertexIndex = 0; vertexIndex < inputMesh->GetControlPointsCount(); vertexIndex++)
				{
					int tangentIndex = 0;

					if (tangentElement->GetReferenceMode() == FbxGeometryElement::eDirect)
					{
						tangentIndex = vertexIndex;
					}

					if (tangentElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
					{
						tangentIndex = tangentElement->GetIndexArray().GetAt(tangentIndex);
					}

					FbxVector4 tangents = tangentElement->GetDirectArray().GetAt(tangentIndex);

					cout << "\n" << "Tangent Normals: " << tangents.mData[0] << " " << tangents.mData[1] << " " << tangents.mData[2] << "\n";

					importMeshData.mVertexList.at(vertexIndex).tangentNormal[0] = tangents.mData[0];
					importMeshData.mVertexList.at(vertexIndex).tangentNormal[1] = tangents.mData[1];
					importMeshData.mVertexList.at(vertexIndex).tangentNormal[2] = tangents.mData[2];
				}

			}

			else if (tangentElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
			{
				int indexPolygonVertex = 0;

				for (int polygonIndex = 0; polygonIndex < inputMesh->GetPolygonCount(); polygonIndex++)
				{
					int polygonSize = inputMesh->GetPolygonSize(polygonIndex);

					for (int i = 0; i < polygonSize; i++)
					{
						int tangentIndex = 0;

						if (tangentElement->GetReferenceMode() == FbxGeometryElement::eDirect)
						{
							tangentIndex = indexPolygonVertex;
						}

						if (tangentElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
						{
							tangentIndex = tangentElement->GetIndexArray().GetAt(indexPolygonVertex);
						}

						FbxVector4 tangents = tangentElement->GetDirectArray().GetAt(tangentIndex);

						cout << "\n" << "Tangent Normals: " << tangents.mData[0] << " " << tangents.mData[1] << " " << tangents.mData[2] << "\n";

						importMeshData.mVertexList.at(indexPolygonVertex).tangentNormal[0] = tangents.mData[0];
						importMeshData.mVertexList.at(indexPolygonVertex).tangentNormal[1] = tangents.mData[1];
						importMeshData.mVertexList.at(indexPolygonVertex).tangentNormal[2] = tangents.mData[2];

						indexPolygonVertex++;
					}
				}
			}
		}
	}
}

void FbxImport::processBiTangents(FbxMesh * inputMesh)
{
	int biTangentCount = inputMesh->GetElementBinormalCount();

	for (int biIndex = 0; biIndex < biTangentCount; biIndex++)
	{
		FbxGeometryElementBinormal* biElement = inputMesh->GetElementBinormal(biIndex);

		if (biElement)
		{
			if (biElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
			{
				for (int vertexIndex = 0; vertexIndex < inputMesh->GetControlPointsCount(); vertexIndex++)
				{
					int biTangentIndex = 0;

					if (biElement->GetReferenceMode() == FbxGeometryElement::eDirect)
					{
						biTangentIndex = vertexIndex;
					}

					if (biElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
					{
						biTangentIndex = biElement->GetIndexArray().GetAt(biTangentIndex);
					}

					FbxVector4 biTangents = biElement->GetDirectArray().GetAt(biTangentIndex);

					cout << "\n" << "BiTangent normals: " << biTangents.mData[0] << " " << biTangents.mData[1] << " " << biTangents.mData[2] << "\n";

					importMeshData.mVertexList.at(vertexIndex).biTangentNormal[0] = biTangents.mData[0];
					importMeshData.mVertexList.at(vertexIndex).biTangentNormal[1] = biTangents.mData[1];
					importMeshData.mVertexList.at(vertexIndex).biTangentNormal[2] = biTangents.mData[2];
				}
			}

			else if (biElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
			{
				int indexPolygonVertex = 0;

				for (int polygonIndex = 0; polygonIndex < inputMesh->GetPolygonCount(); polygonIndex++)
				{
					int polygonSize = inputMesh->GetPolygonSize(polygonIndex);

					for (int i = 0; i < polygonSize; i++)
					{
						int biTangentIndex = 0;

						if (biElement->GetReferenceMode() == FbxGeometryElement::eDirect)
						{
							biTangentIndex = indexPolygonVertex;
						}

						if (biElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
						{
							biTangentIndex = biElement->GetIndexArray().GetAt(indexPolygonVertex);
						}

						FbxVector4 biTangents = biElement->GetDirectArray().GetAt(biTangentIndex);

						cout << "\n" << "BiTangent Normals: " << biTangents.mData[0] << " " << biTangents.mData[1] << " " << biTangents.mData[2] << "\n";

						importMeshData.mVertexList.at(indexPolygonVertex).biTangentNormal[0] = biTangents.mData[0];
						importMeshData.mVertexList.at(indexPolygonVertex).biTangentNormal[1] = biTangents.mData[1];
						importMeshData.mVertexList.at(indexPolygonVertex).biTangentNormal[2] = biTangents.mData[2];

						indexPolygonVertex++;
					}
				}
			}
		}
	}
}

void FbxImport::processUVs(FbxMesh * inputMesh)
{
	/*A mesh can have different UV sets in Maya.*/
	FbxStringList UVSetNameList;
	inputMesh->GetUVSetNames(UVSetNameList);

	/*Looping through all the UV sets of the mesh in the scene.*/
	for (int setIndex = 0; setIndex < UVSetNameList.GetCount(); setIndex++)
	{
		/*Getting the name of each UV set so the name can be used to get
		the Element UV, which are needed to obtain the UV coordinates.*/
		const char* UVSetName = UVSetNameList.GetStringAt(setIndex);
		const FbxGeometryElementUV* UVElement = inputMesh->GetElementUV(UVSetName);

		if (!UVElement) //If the are no UV element then exit the loop. 
			continue;

		/*If the mapping modes for the UV element are not of these two modes,
		then return from the function.*/
		if (UVElement->GetMappingMode() != FbxGeometryElement::eByPolygonVertex &&
			UVElement->GetMappingMode() != FbxGeometryElement::eByControlPoint)
			return;

		/*The index array, which holds the index referenced to UV data.*/
		const bool useIndex = UVElement->GetReferenceMode() != FbxGeometryElement::eDirect &&
			UVElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect;
		const int indexCount = (useIndex) ? UVElement->GetIndexArray().GetCount() : 0;

		const int polyCount = inputMesh->GetPolygonCount(); //Get the polygon count of mesh.

															/*If the mapping mode is "eByControlPoint".*/
		if (UVElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
		{
			for (int polyIndex = 0; polyIndex < polyCount; ++polyIndex)
			{
				const int polySize = inputMesh->GetPolygonSize(polyIndex);

				for (int vertexIndex = 0; vertexIndex < polySize; ++vertexIndex)
				{
					FbxVector2 UVs;

					/*Obtain the index of a current vertex in the control points array.*/
					int polyVertexIndex = inputMesh->GetPolygonVertex(polyIndex, vertexIndex);

					/*What UV Index to be used depends on the reference mode.*/
					int UVIndex = useIndex ? UVElement->GetIndexArray().GetAt(polyVertexIndex) : polyVertexIndex;

					UVs = UVElement->GetDirectArray().GetAt(UVIndex); //Getting the UV coordinates.

					cout << "\n" << "UV: " << UVs.mData[0] << " " << UVs.mData[1] << "\n";

					importMeshData.mVertexList.at(vertexIndex).vertexUV[0] = UVs.mData[0];
					importMeshData.mVertexList.at(vertexIndex).vertexUV[1] = UVs.mData[1];
				}
			}
		}

		/*Otherwise the mapping mode should be "eByPolygonVertex.*/
		else if (UVElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
		{
			int polyIndexCount = 0;
			for (int polyIndex = 0; polyIndex < polyCount; ++polyIndex)
			{
				const int polySize = inputMesh->GetPolygonSize(polyIndex);
				for (int vertexIndex = 0; vertexIndex < polySize; ++vertexIndex)
				{
					FbxVector2 UVs;

					int UVIndex = useIndex ? UVElement->GetIndexArray().GetAt(polyIndexCount) : polyIndexCount;

					UVs = UVElement->GetDirectArray().GetAt(UVIndex);

					cout << "\n" << "UV: " << UVs.mData[0] << " " << UVs.mData[1] << "\n";

					importMeshData.mVertexList.at(polyIndexCount).vertexUV[0] = UVs.mData[0];
					importMeshData.mVertexList.at(polyIndexCount).vertexUV[1] = UVs.mData[1];

					polyIndexCount++;
				}
			}
		}
	}
}

void FbxImport::processMaterials(FbxMesh * inputMesh)
{
	int materialCount = 0;

	if (inputMesh)
	{
		materialCount = inputMesh->GetNode()->GetMaterialCount();

		if (materialCount > 0)
		{
			for (int materialIndex = 0; materialIndex < materialCount; materialIndex++)
			{
				FbxSurfaceMaterial* material = inputMesh->GetNode()->GetMaterial(materialIndex);

				FbxDouble3 ambientColor, diffuseColor, specularColor;

				if (material->GetClassId().Is(FbxSurfacePhong::ClassId))
				{
					const char* materialName = material->GetName();

					if (firstProcess != true)
					{
						materialData.materialName = materialName;
					}

					bool materialFlag = checkMaterialName(materialName);

				/*	mMaterialList[meshTempData.materialID];*/

					if (materialFlag = true)
					{
						cout << "\n" << "Material Number " << materialCounter << ": " << materialName << "\n";

						ambientColor = ((FbxSurfacePhong *)material)->Ambient;
						diffuseColor = ((FbxSurfacePhong *)material)->Diffuse;
						specularColor = ((FbxSurfacePhong *)material)->Specular;

						float shininess = ((FbxSurfacePhong*)material)->Shininess;

						mMaterialList[importMeshData.materialID].ambientColor[0] = ambientColor.mData[0];
						mMaterialList[importMeshData.materialID].ambientColor[1] = ambientColor.mData[1];
						mMaterialList[importMeshData.materialID].ambientColor[2] = ambientColor.mData[2];

						cout << "\n" << "Ambient color: " << ambientColor.mData[0] << " " << ambientColor.mData[1] <<
							" " << ambientColor.mData[2] << "\n";

						mMaterialList[importMeshData.materialID].diffuseColor[0] = diffuseColor.mData[0];
						mMaterialList[importMeshData.materialID].diffuseColor[1] = diffuseColor.mData[1];
						mMaterialList[importMeshData.materialID].diffuseColor[2] = diffuseColor.mData[2];

						cout << "\n" << "Diffuse color: " << diffuseColor.mData[0] << " " << diffuseColor.mData[1] <<
							" " << diffuseColor.mData[2] << "\n";

						mMaterialList[importMeshData.materialID].specularColor[0] = specularColor.mData[0];
						mMaterialList[importMeshData.materialID].specularColor[0] = specularColor.mData[0];
						mMaterialList[importMeshData.materialID].specularColor[0] = specularColor.mData[0];

						cout << "\n" << "Specular color: " << specularColor.mData[0] << " " << specularColor.mData[1] <<
							" " << specularColor.mData[2] << "\n";

						mMaterialList[importMeshData.materialID].shinyFactor = shininess;

						cout << "\n" << "Shininess factor: " << shininess << "\n";

					}
				}

				else if (material->GetClassId().Is(FbxSurfaceLambert::ClassId))
				{
					const char* materialName = material->GetName();

					if (firstProcess != true)
					{
						materialName = material->GetName();
					}

					bool materialFlag = checkMaterialName(materialName);

					if (materialFlag == true)
					{
						cout << "\n" << "Material Number " << materialCounter << ": " << materialName << "\n";

						ambientColor = ((FbxSurfaceLambert *)material)->Ambient;
						diffuseColor = ((FbxSurfaceLambert *)material)->Diffuse;

						mMaterialList[importMeshData.materialID].ambientColor[0] = ambientColor.mData[0];
						mMaterialList[importMeshData.materialID].ambientColor[1] = ambientColor.mData[1];
						mMaterialList[importMeshData.materialID].ambientColor[2] = ambientColor.mData[2];

						cout << "\n" << "Ambient color: " << ambientColor.mData[0] << " " << ambientColor.mData[1] <<
							" " << ambientColor.mData[2] << "\n";

						mMaterialList[importMeshData.materialID].diffuseColor[0] = diffuseColor.mData[0];
						mMaterialList[importMeshData.materialID].diffuseColor[1] = diffuseColor.mData[1];
						mMaterialList[importMeshData.materialID].diffuseColor[2] = diffuseColor.mData[2];

						cout << "\n" << "Diffuse color: " << diffuseColor.mData[0] << " " << diffuseColor.mData[1] <<
							" " << diffuseColor.mData[2] << "\n";

						/*No specular attributes and shininess for lambert material, so set the values to 0.*/
						mMaterialList[importMeshData.materialID].specularColor[0] = 0;
						mMaterialList[importMeshData.materialID].specularColor[1] = 0;
						mMaterialList[importMeshData.materialID].specularColor[2] = 0;

						mMaterialList[importMeshData.materialID].shinyFactor = 0;
					}
				}
			}
		}
	}
}

void FbxImport::processJoints(FbxMesh * inputMesh)
{
	unsigned int deformerCount = inputMesh->GetDeformerCount(FbxDeformer::eSkin);
	
	//If we have a skeleton attacked to this mesh
	if(deformerCount > 0)
	{
		//Do nothing here, for a while.
	}

	for (unsigned int deformerCounter = 0; deformerCounter < deformerCount; ++deformerCounter)
	{
		FbxSkin* currSkin = reinterpret_cast<FbxSkin*>(inputMesh->GetDeformer(0, FbxDeformer::eSkin));
		if (!currSkin)
			continue;
		unsigned int clusterCount = currSkin->GetClusterCount();
		for (unsigned int clusterCounter = 0; clusterCounter < clusterCount; ++clusterCounter)
		{
			FbxCluster* currCluster = currSkin->GetCluster(clusterCounter);


		}
	}
	

}

void FbxImport::processTextures(FbxMesh * inputMesh)
{
	FbxProperty propDiffus, propSpecular, propNormal;

	int materialCount = inputMesh->GetNode()->GetSrcObjectCount<FbxSurfaceMaterial>();

	for (int materialIndex = 0; materialIndex < materialCount; materialIndex++)
	{
		FbxSurfaceMaterial* material = inputMesh->GetNode()->GetSrcObject<FbxSurfaceMaterial>(materialIndex);

		if (material)
		{
			propDiffus = material->FindProperty(FbxSurfaceMaterial::sDiffuse);
			propSpecular = material->FindProperty(FbxSurfaceMaterial::sSpecular);
			propNormal = material->FindProperty(FbxSurfaceMaterial::sNormalMap);

			FbxTexture* texture;
			int textureCount;

			if (propDiffus.IsValid())
			{
				processDiffuseMaps(propDiffus);
			}

			if (propSpecular.IsValid())
			{
				processSpecularMaps(propSpecular);
			}

			if (propNormal.IsValid())
			{
				processNormalMaps(propNormal);
			}
		}
	}
}

void FbxImport::processDiffuseMaps(FbxProperty diffuseProp)
{
	int textureCount = diffuseProp.GetSrcObjectCount<FbxTexture>();

	for (int textureIndex = 0; textureIndex < textureCount; textureIndex++)
	{
		FbxTexture* texture = diffuseProp.GetSrcObject<FbxTexture>(textureIndex);
		FbxFileTexture* fileTexture = FbxCast<FbxFileTexture>(texture);
		FbxString fileTextureName = fileTexture->GetName();

		cout << "\n" << "Texturename Nr " << textureCounter + 1 << ": " << fileTextureName << "\n";

		wchar_t* textureToWchar;
		FbxUTF8ToWC(fileTextureName.Buffer(), textureToWchar, NULL);

		mMaterialList[importMeshData.materialID].diffuseTexture = textureToWchar;

		textureCounter++;
	}
}

void FbxImport::processSpecularMaps(FbxProperty propSpecular)
{
	int textureCount = propSpecular.GetSrcObjectCount<FbxTexture>();

	for (int textureIndex = 0; textureIndex < textureCount; textureIndex++)
	{
		FbxTexture* texture = propSpecular.GetSrcObject<FbxTexture>(textureIndex);
		FbxFileTexture* fileTexture = FbxCast<FbxFileTexture>(texture);
		FbxString fileTextureName = fileTexture->GetName();

		cout << "\n" << "Texturename Nr " << textureCounter + 1 << ": " << fileTextureName << "\n";

		wchar_t* textureToWchar;
		FbxUTF8ToWC(fileTextureName.Buffer(), textureToWchar, NULL);

		mMaterialList[importMeshData.materialID].specularTexture = textureToWchar;

		textureCounter++;
	}
}

void FbxImport::processNormalMaps(FbxProperty propNormal)
{
	int textureCount = propNormal.GetSrcObjectCount<FbxTexture>();

	for (int textureIndex = 0; textureIndex < textureCount; textureIndex++)
	{
		FbxTexture* texture = propNormal.GetSrcObject<FbxTexture>(textureIndex);
		FbxFileTexture* fileTexture = FbxCast<FbxFileTexture>(texture);
		FbxString fileTextureName = fileTexture->GetName();

		cout << "\n" << "Texturename Nr " << textureCounter + 1 << ": " << fileTextureName << "\n";

		wchar_t* textureToWchar;
		FbxUTF8ToWC(fileTextureName.Buffer(), textureToWchar, NULL);

		mMaterialList[importMeshData.materialID].normalTexture = textureToWchar;

		textureCounter++;
	}
}

void FbxImport::processTransformations(FbxMesh* inputMesh)
{
	FbxAMatrix transformMatrix = inputMesh->GetNode()->EvaluateGlobalTransform();

	FbxVector4 trans, scale, rotat;

	trans = transformMatrix.GetT();
	scale = transformMatrix.GetS();
	rotat = transformMatrix.GetR();

	cout << "\n" << "Position: " << trans.mData[0] << " " << trans.mData[1] << " " << trans.mData[2] << "\n";

	importMeshData.translate[0] = trans.mData[0];
	importMeshData.translate[1] = trans.mData[1];
	importMeshData.translate[2] = trans.mData[2];

	cout << "\n" << "Scale: " << scale.mData[0] << " " << scale.mData[1] << " " << scale.mData[2] << "\n";

	importMeshData.scale[0] = scale.mData[0];
	importMeshData.scale[1] = scale.mData[1];
	importMeshData.scale[2] = scale.mData[2];

	cout << "\n" << "Rotation: " << rotat.mData[0] << " " << rotat.mData[1] << " " << rotat.mData[2] << "\n\n";

	importMeshData.rotation[0] = rotat.mData[0];
	importMeshData.rotation[1] = rotat.mData[1];
	importMeshData.rotation[2] = rotat.mData[2];
}

void FbxImport::processLight(FbxLight * inputLight)
{
	FbxString lightName = inputLight->GetNode()->GetName();

	int lightType = inputLight->LightType.Get();

	if (lightType == 0 || lightType == 1) /*If the lighttype is a Pointlight or a DirectionalLight.*/
	{
		cout << "\n" << "Light nr " << lightCounter++ << ": " << lightName;

		FbxVector4 lightColor = inputLight->Color.Get();

		dirPointData.color[0] = lightColor.mData[0];
		dirPointData.color[1] = lightColor.mData[1];
		dirPointData.color[2] = lightColor.mData[2];

		cout << "\n" << "Light color: " << lightColor.mData[0] << " "
			<< lightColor.mData[1] << " " << lightColor.mData[2];

		float lightIntensity = inputLight->Intensity.Get();

		dirPointData.intensity = lightIntensity;

		cout << "\n" << "Light intensity: " << lightIntensity;

		FbxAMatrix lightPosition = inputLight->GetNode()->EvaluateGlobalTransform();

		FbxVector4 trans, rot, scal;

		trans = lightPosition.GetT();
		rot = lightPosition.GetR();
		scal = lightPosition.GetS();

		dirPointData.lightPos[0] = trans.mData[0];
		dirPointData.lightPos[1] = trans.mData[1];
		dirPointData.lightPos[2] = trans.mData[2];

		cout << "\n" << "Light position: " << trans.mData[0] << " " << trans.mData[1] << " " << trans.mData[2];

		dirPointData.lightScale[0] = rot.mData[0];
		dirPointData.lightScale[1] = rot.mData[1];
		dirPointData.lightScale[2] = rot.mData[2];

		cout << "\n" << "Light orientation: " << rot.mData[0] << " " << rot.mData[1] << " " << rot.mData[2];

		dirPointData.lightRot[0] = scal.mData[0];
		dirPointData.lightRot[0] = scal.mData[1];
		dirPointData.lightRot[0] = scal.mData[2];

		cout << "\n" << "Light scale: " << scal.mData[0] << " " << scal.mData[1] << " " << scal.mData[2] << "\n\n";

		mDirPointList.push_back(dirPointData);

		/*lightData.countDirectionalPoint = mDirPointList.size();*/
	}

	if (lightType == 2) /*If the light type is a Spotlight.*/
	{
		cout << "\n" << "Light nr " << lightCounter++ << ": " << lightName;

		FbxVector4 lightColor = inputLight->Color.Get();

		dirPointData.color[0] = lightColor.mData[0];
		dirPointData.color[1] = lightColor.mData[1];
		dirPointData.color[2] = lightColor.mData[2];

		cout << "\n" << "Light color: " << lightColor.mData[0] << " "
			<< lightColor.mData[1] << " " << lightColor.mData[2];

		float lightIntensity = inputLight->Intensity.Get();

		spotData.intensity = lightIntensity;

		cout << "\n" << "Light intensity: " << lightIntensity;

		float innerAngle = inputLight->InnerAngle.Get();
		float outerAngle = inputLight->OuterAngle.Get();

		spotData.innerAngle = innerAngle;
		spotData.outerAngle = outerAngle;

		cout << "\n" << "Inner angle: " << innerAngle << " " << "Outer angle: " << outerAngle;

		FbxAMatrix lightPosition = inputLight->GetNode()->EvaluateGlobalTransform();

		FbxVector4 trans, rot, scal;

		trans = lightPosition.GetT();
		rot = lightPosition.GetR();
		scal = lightPosition.GetS();

		spotData.lightPos[0] = trans.mData[0];
		spotData.lightPos[1] = trans.mData[1];
		spotData.lightPos[2] = trans.mData[2];

		cout << "\n" << "Light position: " << trans.mData[0] << " " << trans.mData[1] << " " << trans.mData[2];

		spotData.lightScale[0] = rot.mData[0];
		spotData.lightScale[1] = rot.mData[1];
		spotData.lightScale[2] = rot.mData[2];

		cout << "\n" << "Light orientation: " << rot.mData[0] << " " << rot.mData[1] << " " << rot.mData[2];

		spotData.lightRot[0] = scal.mData[0];
		spotData.lightRot[0] = scal.mData[1];
		spotData.lightRot[0] = scal.mData[2];

		cout << "\n" << "Light scale: " << scal.mData[0] << " " << scal.mData[1] << " " << scal.mData[2] << "\n\n";

		mSpotList.push_back(spotData);

		/*lightData.countSpotlight = mSpotList.size();*/
	}
}

void FbxImport::processCamera(FbxCamera * inputCamera)
{
	FbxString cameraName = inputCamera->GetName();

	cout << "\n\n" << "Camera nr " << cameraCounter++ << ": " << cameraName << "\n";

	FbxVector4 cameraPos = inputCamera->Position.Get();

	cout << "\n" << "Camera position: " << cameraPos.mData[0] << " " << cameraPos.mData[1]
		<< " " << cameraPos.mData[2];

	camData.camPos[0] = cameraPos.mData[0];
	camData.camPos[1] = cameraPos.mData[1];
	camData.camPos[2] = cameraPos.mData[2];

	FbxVector4 camUpVector = inputCamera->UpVector.Get();

	cout << "\n" << "Camera Up Vector: " << camUpVector.mData[0] << " " << camUpVector.mData[1]
		<< " " << camUpVector.mData[2];

	camData.upVector[0] = camUpVector.mData[0];
	camData.upVector[1] = camUpVector.mData[1];
	camData.upVector[2] = camUpVector.mData[2];

	float fov = inputCamera->FieldOfView.Get();

	cout << "\n" << "Field of view: " << fov << " degrees";

	camData.fieldOfView = fov;

	float nearPlane = inputCamera->NearPlane.Get();

	cout << "\n" << "Near plane: " << nearPlane;

	camData.nearPlane = nearPlane;

	float farPlane = inputCamera->FarPlane.Get();

	cout << "\n" << "Far plane: " << farPlane;

	camData.farPlane = farPlane;

	mCameraList.push_back(camData);
}

bool FbxImport::checkMaterialName(const char* materialName)
{
	for (int nameIndex = 0; nameIndex < mMaterialList.size(); nameIndex++)
	{
		if (std::strcmp(mMaterialList[nameIndex].materialName, materialName) == 0)
		{
			importMeshData.materialID = nameIndex;
			return false; /*The material names are identical.*/
		}
	}

	if (materialData.materialName == nullptr)
	{
		materialData.materialName = materialName;
		mMaterialList.push_back(materialData);
		importMeshData.materialID = 0;
		firstProcess = false;
		return true; /*This is the first material name.*/
	}

	materialData.materialName = materialName;
	mMaterialList.push_back(materialData);
	importMeshData.materialID = mMaterialList.size() - 1;

	return true; /*The two material names are not identical.*/

}

void FbxImport::assignToHeaderData()
{
	/*The following lines of code is a copy from the temporary mesh list
	to the one that will have the vertex count.*/

	/*for (int i = 0; i < headerData.meshCount; i++)
	{
		mMeshList[i].vertexCount = mTempMeshList[i].mVertexList.size();

		mMeshList[i].materialID = mTempMeshList[i].materialID;

		for (int j = 0; j < 3; j++)
		{
			mMeshList[i].translate[j] = mTempMeshList[i].translate[j];
			mMeshList[i].scale[j] = mTempMeshList[i].scale[j];
			mMeshList[i].rotation[j] = mTempMeshList[i].rotation[j];
		}
	}*/

	headerData.meshCount = mTempMeshList.size();

	mSMeshList.resize(headerData.meshCount);

	mList.resize(mTempMeshList.size());

	cout << mSMeshList.size() << "\n";

	sVertex vertex;

	for (int sMesh = 0; sMesh < mSMeshList.size(); sMesh++)
	{
		cout << "Mesh: " << sMesh + 1 << "\n";

		mSMeshList[sMesh].vertexCount = mTempMeshList[sMesh].mVertexList.size();

		cout << "Vertex count: " << mSMeshList[sMesh].vertexCount << "\n";

		for (int vertices = 0; vertices < mTempMeshList[sMesh].mVertexList.size(); vertices++)
		{
			/*cout << "Position: " << mTempMeshList[sMesh].mVertexList[vertices].vertexPos[0] << " "
				<< mTempMeshList[sMesh].mVertexList[vertices].vertexPos[1] << " "
				<< mTempMeshList[sMesh].mVertexList[vertices].vertexPos[2] << "\n";

			cout << "Normal: " << mTempMeshList[sMesh].mVertexList[vertices].vertexNormal[0] << " "
				<< mTempMeshList[sMesh].mVertexList[vertices].vertexNormal[1] << " "
				<< mTempMeshList[sMesh].mVertexList[vertices].vertexNormal[2] << "\n";

			cout << "UV: " << mTempMeshList[sMesh].mVertexList[vertices].vertexUV[0] << " "
				<< mTempMeshList[sMesh].mVertexList[vertices].vertexUV[1] << "\n";*/

			vertex.vertexPos[0] = mTempMeshList[sMesh].mVertexList[vertices].vertexPos[0];
			vertex.vertexPos[1] = mTempMeshList[sMesh].mVertexList[vertices].vertexPos[1];
			vertex.vertexPos[2] = mTempMeshList[sMesh].mVertexList[vertices].vertexPos[2];

			vertex.vertexNormal[0] = mTempMeshList[sMesh].mVertexList[vertices].vertexNormal[0];
			vertex.vertexNormal[1] = mTempMeshList[sMesh].mVertexList[vertices].vertexNormal[1];
			vertex.vertexNormal[2] = mTempMeshList[sMesh].mVertexList[vertices].vertexNormal[2];

			vertex.vertexUV[0] = mTempMeshList[sMesh].mVertexList[vertices].vertexUV[0];
			vertex.vertexUV[1] = mTempMeshList[sMesh].mVertexList[vertices].vertexUV[1];

			vertex.tangentNormal[0] = mTempMeshList[sMesh].mVertexList[vertices].tangentNormal[0];
			vertex.tangentNormal[1] = mTempMeshList[sMesh].mVertexList[vertices].tangentNormal[1];
			vertex.tangentNormal[2] = mTempMeshList[sMesh].mVertexList[vertices].tangentNormal[2];

			vertex.biTangentNormal[0] = mTempMeshList[sMesh].mVertexList[vertices].biTangentNormal[0];
			vertex.biTangentNormal[1] = mTempMeshList[sMesh].mVertexList[vertices].biTangentNormal[1];
			vertex.biTangentNormal[2] = mTempMeshList[sMesh].mVertexList[vertices].biTangentNormal[2];

			mList[sMesh].vList.push_back(vertex);
		}
	}
}

void FbxImport::WriteToBinary()
{

}



