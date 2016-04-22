#include "FbxImport.h"
#include "HeaderData.h"

FbxImport::~FbxImport()
{
}

bool FbxImport::determineIfIndexed(FbxMesh * inputMesh)
{
	return false;

	bool isIndexed = false;
	FbxGeometryElementNormal* normalElement = inputMesh->GetElementNormal();
	if (normalElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
	{
		int normalCount = normalElement->GetDirectArray().GetCount();
		int normalIndexCount = normalElement->GetIndexArray().GetCount();

		int tangentCount = inputMesh->GetElementTangentCount();
		if (tangentCount == 0)
		{
			FbxStringList UVSetNameList;
			inputMesh->GetUVSetNames(UVSetNameList);
			for (int setIndex = 0; setIndex < UVSetNameList.GetCount(); setIndex++)
			{
				const char* UVSetName = UVSetNameList.GetStringAt(setIndex);
				const FbxGeometryElementUV* UVElement = inputMesh->GetElementUV(UVSetName);

				int cpCount = inputMesh->GetControlPointsCount();
				int UVCount = UVElement->GetDirectArray().GetCount();
				int UVIndexCount = UVElement->GetIndexArray().GetCount();

				if (cpCount == UVCount && UVIndexCount > UVCount)
				{
					isIndexed = true;
					return isIndexed;
				}

				/*if (UVElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
				{
					isIndexed = true;
				}*/
				break;
			}
		}

		for (int i = 0; i < tangentCount; i++)
		{
			FbxGeometryElementTangent* tangentElement = inputMesh->GetElementTangent(i);
			int cpCount = inputMesh->GetControlPointsCount();
			int tangentCount = tangentElement->GetDirectArray().GetCount();
			int tangentIndexCount = tangentElement->GetIndexArray().GetCount();

			if (tangentElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
			{
				int biTangentCount = inputMesh->GetElementBinormalCount();
				for (int biIndex = 0; biIndex < biTangentCount; biIndex++)
				{
					FbxGeometryElementBinormal* biElement = inputMesh->GetElementBinormal(biIndex);
					if (biElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
					{
						FbxStringList UVSetNameList;
						inputMesh->GetUVSetNames(UVSetNameList);
						for (int setIndex = 0; setIndex < UVSetNameList.GetCount(); setIndex++)
						{
							const char* UVSetName = UVSetNameList.GetStringAt(setIndex);
							const FbxGeometryElementUV* UVElement = inputMesh->GetElementUV(UVSetName);

							int cpCount = inputMesh->GetControlPointsCount();
							int UVCount = UVElement->GetDirectArray().GetCount();
							int UVIndexCount = UVElement->GetIndexArray().GetCount();

							if (cpCount == UVCount && UVIndexCount > UVCount)
							{
								isIndexed = true;
							}

							if (UVElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
							{
								isIndexed = true;
							}
							break;
						}
					}
					break;
				}
			}
			break;
		}
	}

	return isIndexed;
}

void FbxImport::processIndices(FbxMesh * inputMesh)
{
	for (unsigned int polyCounter = 0; polyCounter < inputMesh->GetPolygonCount(); polyCounter++)
	{
		const unsigned int polySize = inputMesh->GetPolygonSize(polyCounter);
		assert(polySize == 3 && "The size of polygon nr: %d is not 3.", polyCounter);
		for (unsigned int polyCorner = 0; polyCorner < 3; polyCorner++)
		{
			const unsigned int index = inputMesh->GetPolygonVertex(polyCounter, polyCorner);
			importMeshData.mIndexList.push_back(index);
		}
	}
}

void FbxImport::processJointHierarchy(FbxNode * inputRoot)
{
	for (int childIndex = 0; childIndex < inputRoot->GetChildCount(); ++childIndex) {
		FbxNode* currNode = inputRoot->GetChild(childIndex);
		recursiveJointHierarchyTraversal(currNode, 0, -1);
	}
}

void FbxImport::recursiveJointHierarchyTraversal(FbxNode * inNode, int currIndex, int inNodeParentIndex)
{
	if (inNode->GetNodeAttribute() && inNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		sImportJointData currJoint;
		currJoint.parentJointID = inNodeParentIndex;
		currJoint.name = inNode->GetName();
		currJoint.jointID = currIndex;
		pmSceneJoints.push_back(currJoint);
	}
	for (int i = 0; i < inNode->GetChildCount(); i++) {
		//currentIndex becomes the "old index". And the size of the joint-hierarchy-list "becomes" the currentIndex instead
		//We process each and every child of this node, we search for children of AttributeType eSkeleton to add to the list of joints.
		recursiveJointHierarchyTraversal(inNode->GetChild(i), pmSceneJoints.size(), currIndex);
	}
}

unsigned int FbxImport::findJointIndexByName(const char * jointName)
{
	try
	{
		for (unsigned int i = 0; i < pmSceneJoints.size(); ++i)
		{
			//Note: * before pointer to get object
			int compareValue = std::strcmp(jointName, pmSceneJoints[i].name);
			if (compareValue == 0) { //Apparently no matching name can be found...
				return pmSceneJoints[i].jointID; //parentIndex + 1 gets the index of this joint.
			}
		}
	}

	catch (const std::exception&)
	{
		printf("Error in FbxDawg::findJointIndexByName(const char* jointName): cannot find matching joint name\n");
	}
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


void FbxImport::initializeImporter(const char* filePath)
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

	bool importStatus = pImporter->Initialize(filePath, -1, pmManager->GetIOSettings());
	/*Abort if the importer can't be intitialized.*/
	if (importStatus == false) {
		FBXSDK_printf("Error: Can't initialize the importer. Aborting...\n");
		exit(1);
	}

	pmScene = FbxScene::Create(pmManager, "MyScene");

	importStatus = pImporter->Import(pmScene);
	/*Abort if the scene can't be imported.*/
	if (importStatus == false) {
		FBXSDK_printf("Error: Can't import the scene. Aborting...\n");
		exit(1);
	}
	pImporter->Destroy();

	/*Get the handle to all of the objects in the scene.*/
	pmRootNode = pmScene->GetRootNode();

	//Fill the "scene-joint-graph" with basic "parenting" values
	processJointHierarchy(pmRootNode);

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
			std::cout << "\n" << "Object nr: " << meshCounter << " Name: " << childNode->GetName() << "\n";

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

	processJoints(inputMesh);

	/*meshData.vertexCount = meshTempData.mVertexList.size();

	headerData.materialCount = mMaterialList.size();*/

	mTempMeshList.push_back(importMeshData);
}

void FbxImport::processVertices(FbxMesh * inputMesh)
{
	//Calls a function to see if indexation is worthwhile
	//For the moment it is not.
	importMeshData.isIndexed = determineIfIndexed(inputMesh);

	/*Array of the control points of mesh.*/
	FbxVector4* vertices = inputMesh->GetControlPoints();

	if (importMeshData.isIndexed)
	{
		unsigned int deformerCount = inputMesh->GetDeformerCount(FbxDeformer::eSkin);
		if (deformerCount > 0)
		{
			importMeshData.isAnimated = true;
			//First get the control-point-vertices
			const unsigned int controlPointCount = inputMesh->GetControlPointsCount();
			for (unsigned int cpCounter = 0; cpCounter < controlPointCount; cpCounter++)
			{
				sSkelAnimVertex animVertex;
				
				animVertex.vertexPos[0] = vertices[cpCounter].mData[0];
				animVertex.vertexPos[1] = vertices[cpCounter].mData[1];
				animVertex.vertexPos[2] = vertices[cpCounter].mData[2];

				importMeshData.mSkelVertexList.push_back(animVertex);
			}
			//Then get the indices
			processIndices(inputMesh);
		}
		else
		{
			importMeshData.isAnimated = false;
			//First get the control-point-vertices
			const unsigned int controlPointCount = inputMesh->GetControlPointsCount();
			for (unsigned int cpCounter = 0; cpCounter < controlPointCount; cpCounter++)
			{
				sVertex vertex;

				vertex.vertexPos[0] = vertices[cpCounter].mData[0];
				vertex.vertexPos[1] = vertices[cpCounter].mData[1];
				vertex.vertexPos[2] = vertices[cpCounter].mData[2];

				importMeshData.mVertexList.push_back(vertex);
			}
			//Then get the indices
			processIndices(inputMesh);
		}
	}

	if (!importMeshData.isIndexed)
	{

		unsigned int deformerCount = inputMesh->GetDeformerCount(FbxDeformer::eSkin);
		if (deformerCount > 0)
		{
			importMeshData.isAnimated = true;
			for (int i = 0; i < inputMesh->GetPolygonCount(); i++)
			{
				/*Getting vertices of a polygon in the mesh.*/
				int numPolygonVertices = inputMesh->GetPolygonSize(i);

				/*If the mesh is not triangulated, meaning that there are quads in the mesh,
				then the program should abort, terminating the process.*/
				assert(numPolygonVertices == 3);

				for (int j = 0; j < numPolygonVertices; j++)
				{
					sSkelAnimVertex animVertex;

					/*Getting the index to a control point "vertex".*/
					int polygonVertex = inputMesh->GetPolygonVertex(i, j);

					//Set influences to -1337 so that we know which index ain't set yet.
					for (int c = 0; c < 4; c++) {
						animVertex.influences[c] = -1337;
						animVertex.weights[c] = 0.0;
					}

					animVertex.vertexPos[0] = (float)vertices[polygonVertex].mData[0];
					animVertex.vertexPos[1] = (float)vertices[polygonVertex].mData[1];
					animVertex.vertexPos[2] = (float)vertices[polygonVertex].mData[2];

					std::cout << "\n" << "Position: " << (float)vertices[polygonVertex].mData[0] << " " <<
						(float)vertices[polygonVertex].mData[1] << " " <<
						(float)vertices[polygonVertex].mData[1] << "\n";

					importMeshData.mSkelVertexList.push_back(animVertex);
				}
			}
		}
		else
		{
			importMeshData.isAnimated = false;
			for (int i = 0; i < inputMesh->GetPolygonCount(); i++)
			{
				/*Getting vertices of a polygon in the mesh.*/
				int numPolygonVertices = inputMesh->GetPolygonSize(i);

				/*If the mesh is not triangulated, meaning that there are quads in the mesh,
				then the program should abort, terminating the process.*/
				assert(numPolygonVertices == 3);

				for (int j = 0; j < numPolygonVertices; j++)
				{
					sVertex vertex;
					/*Getting the index to a control point "vertex".*/
					int polygonVertex = inputMesh->GetPolygonVertex(i, j);

					vertex.vertexPos[0] = (float)vertices[polygonVertex].mData[0];
					vertex.vertexPos[1] = (float)vertices[polygonVertex].mData[1];
					vertex.vertexPos[2] = (float)vertices[polygonVertex].mData[2];

					std::cout << "\n" << "Position: " << (float)vertices[polygonVertex].mData[0] << " " <<
						(float)vertices[polygonVertex].mData[1] << " " <<
						(float)vertices[polygonVertex].mData[1] << "\n";

					importMeshData.mVertexList.push_back(vertex);
				}
			}
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

				std::cout << "\n" << "Normal: " << normals.mData[0] << " " << normals.mData[1] << " " << normals.mData[2] << "\n";

				if (importMeshData.isAnimated)
				{
					importMeshData.mSkelVertexList.at(vertexIndex).vertexNormal[0] = normals.mData[0];
					importMeshData.mSkelVertexList.at(vertexIndex).vertexNormal[1] = normals.mData[1];
					importMeshData.mSkelVertexList.at(vertexIndex).vertexNormal[2] = normals.mData[2];
				}
				else
				{
					importMeshData.mVertexList.at(vertexIndex).vertexNormal[0] = normals.mData[0];
					importMeshData.mVertexList.at(vertexIndex).vertexNormal[1] = normals.mData[1];
					importMeshData.mVertexList.at(vertexIndex).vertexNormal[2] = normals.mData[2];
				}
				
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
					/*Reference mode is index-to-direct, which means getting normals by index-to-direct.*/
					if (normalElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
					{
						normalIndex = normalElement->GetIndexArray().GetAt(indexPolygonVertex);
					}

					FbxVector4 normals = normalElement->GetDirectArray().GetAt(normalIndex); //Obtain normals of each polygon-vertex

					std::cout << "\n" << "Normal: " << normals.mData[0] << " " << normals.mData[1] << " " << normals.mData[2] << "\n";

					if (importMeshData.isAnimated)
					{
						importMeshData.mSkelVertexList.at(indexPolygonVertex).vertexNormal[0] = normals.mData[0];
						importMeshData.mSkelVertexList.at(indexPolygonVertex).vertexNormal[1] = normals.mData[1];
						importMeshData.mSkelVertexList.at(indexPolygonVertex).vertexNormal[2] = normals.mData[2];
					}
					else
					{
						importMeshData.mVertexList.at(indexPolygonVertex).vertexNormal[0] = normals.mData[0];
						importMeshData.mVertexList.at(indexPolygonVertex).vertexNormal[1] = normals.mData[1];
						importMeshData.mVertexList.at(indexPolygonVertex).vertexNormal[2] = normals.mData[2];
					}

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

					std::cout << "\n" << "Tangent Normals: " << tangents.mData[0] << " " << tangents.mData[1] << " " << tangents.mData[2] << "\n";

					if (importMeshData.isAnimated)
					{
						importMeshData.mSkelVertexList.at(vertexIndex).tangentNormal[0] = tangents.mData[0];
						importMeshData.mSkelVertexList.at(vertexIndex).tangentNormal[1] = tangents.mData[1];
						importMeshData.mSkelVertexList.at(vertexIndex).tangentNormal[2] = tangents.mData[2];
					}
					else
					{
						importMeshData.mVertexList.at(vertexIndex).tangentNormal[0] = tangents.mData[0];
						importMeshData.mVertexList.at(vertexIndex).tangentNormal[1] = tangents.mData[1];
						importMeshData.mVertexList.at(vertexIndex).tangentNormal[2] = tangents.mData[2];
					}
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

						std::cout << "\n" << "Tangent Normals: " << tangents.mData[0] << " " << tangents.mData[1] << " " << tangents.mData[2] << "\n";

						if (importMeshData.isAnimated)
						{
							importMeshData.mSkelVertexList.at(indexPolygonVertex).tangentNormal[0] = tangents.mData[0];
							importMeshData.mSkelVertexList.at(indexPolygonVertex).tangentNormal[1] = tangents.mData[1];
							importMeshData.mSkelVertexList.at(indexPolygonVertex).tangentNormal[2] = tangents.mData[2];
						}
						else
						{
							importMeshData.mVertexList.at(indexPolygonVertex).tangentNormal[0] = tangents.mData[0];
							importMeshData.mVertexList.at(indexPolygonVertex).tangentNormal[1] = tangents.mData[1];
							importMeshData.mVertexList.at(indexPolygonVertex).tangentNormal[2] = tangents.mData[2];
						}

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

					std::cout << "\n" << "BiTangent normals: " << biTangents.mData[0] << " " << biTangents.mData[1] << " " << biTangents.mData[2] << "\n";

					if(importMeshData.isAnimated)
					{
						importMeshData.mSkelVertexList.at(vertexIndex).biTangentNormal[0] = biTangents.mData[0];
						importMeshData.mSkelVertexList.at(vertexIndex).biTangentNormal[1] = biTangents.mData[1];
						importMeshData.mSkelVertexList.at(vertexIndex).biTangentNormal[2] = biTangents.mData[2];
					}
					else
					{
						importMeshData.mVertexList.at(vertexIndex).biTangentNormal[0] = biTangents.mData[0];
						importMeshData.mVertexList.at(vertexIndex).biTangentNormal[1] = biTangents.mData[1];
						importMeshData.mVertexList.at(vertexIndex).biTangentNormal[2] = biTangents.mData[2];
					}


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

						std::cout << "\n" << "BiTangent Normals: " << biTangents.mData[0] << " " << biTangents.mData[1] << " " << biTangents.mData[2] << "\n";

						if (importMeshData.isAnimated)
						{
							importMeshData.mSkelVertexList.at(indexPolygonVertex).biTangentNormal[0] = biTangents.mData[0];
							importMeshData.mSkelVertexList.at(indexPolygonVertex).biTangentNormal[1] = biTangents.mData[1];
							importMeshData.mSkelVertexList.at(indexPolygonVertex).biTangentNormal[2] = biTangents.mData[2];
						}
						else
						{
							importMeshData.mVertexList.at(indexPolygonVertex).biTangentNormal[0] = biTangents.mData[0];
							importMeshData.mVertexList.at(indexPolygonVertex).biTangentNormal[1] = biTangents.mData[1];
							importMeshData.mVertexList.at(indexPolygonVertex).biTangentNormal[2] = biTangents.mData[2];
						}

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

		int wololo = UVElement->GetDirectArray().GetCount();
		int kekek = UVElement->GetIndexArray().GetCount();
		int koala = 5;
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

					std::cout << "\n" << "UV: " << UVs.mData[0] << " " << UVs.mData[1] << "\n";

					if(importMeshData.isAnimated)
					{
						importMeshData.mSkelVertexList.at(vertexIndex).vertexUV[0] = UVs.mData[0];
						importMeshData.mSkelVertexList.at(vertexIndex).vertexUV[1] = UVs.mData[1];
					}
					else
					{
						importMeshData.mVertexList.at(vertexIndex).vertexUV[0] = UVs.mData[0];
						importMeshData.mVertexList.at(vertexIndex).vertexUV[1] = UVs.mData[1];
					}
					
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

					std::cout << "\n" << "UV: " << UVs.mData[0] << " " << UVs.mData[1] << "\n";

					if (importMeshData.isAnimated)
					{
						importMeshData.mSkelVertexList.at(polyIndexCount).vertexUV[0] = UVs.mData[0];
						importMeshData.mSkelVertexList.at(polyIndexCount).vertexUV[1] = UVs.mData[1];
					}
					else
					{
						importMeshData.mVertexList.at(polyIndexCount).vertexUV[0] = UVs.mData[0];
						importMeshData.mVertexList.at(polyIndexCount).vertexUV[1] = UVs.mData[1];
					}
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
						std::cout << "\n" << "Material Number " << materialCounter << ": " << materialName << "\n";

						ambientColor = ((FbxSurfacePhong *)material)->Ambient;
						diffuseColor = ((FbxSurfacePhong *)material)->Diffuse;
						specularColor = ((FbxSurfacePhong *)material)->Specular;

						float shininess = ((FbxSurfacePhong*)material)->Shininess;

						materialList[importMeshData.materialID].ambientColor[0] = ambientColor.mData[0];
						materialList[importMeshData.materialID].ambientColor[1] = ambientColor.mData[1];
						materialList[importMeshData.materialID].ambientColor[2] = ambientColor.mData[2];

						std::cout << "\n" << "Ambient color: " << ambientColor.mData[0] << " " << ambientColor.mData[1] <<
							" " << ambientColor.mData[2] << "\n";

						materialList[importMeshData.materialID].diffuseColor[0] = diffuseColor.mData[0];
						materialList[importMeshData.materialID].diffuseColor[1] = diffuseColor.mData[1];
						materialList[importMeshData.materialID].diffuseColor[2] = diffuseColor.mData[2];

						std::cout << "\n" << "Diffuse color: " << diffuseColor.mData[0] << " " << diffuseColor.mData[1] <<
							" " << diffuseColor.mData[2] << "\n";

						materialList[importMeshData.materialID].specularColor[0] = specularColor.mData[0];
						materialList[importMeshData.materialID].specularColor[0] = specularColor.mData[0];
						materialList[importMeshData.materialID].specularColor[0] = specularColor.mData[0];

						std::cout << "\n" << "Specular color: " << specularColor.mData[0] << " " << specularColor.mData[1] <<
							" " << specularColor.mData[2] << "\n";

						materialList[importMeshData.materialID].shinyFactor = shininess;

						std::cout << "\n" << "Shininess factor: " << shininess << "\n";

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
						std::cout << "\n" << "Material Number " << materialCounter << ": " << materialName << "\n";

						ambientColor = ((FbxSurfaceLambert *)material)->Ambient;
						diffuseColor = ((FbxSurfaceLambert *)material)->Diffuse;

						materialList[importMeshData.materialID].ambientColor[0] = ambientColor.mData[0];
						materialList[importMeshData.materialID].ambientColor[1] = ambientColor.mData[1];
						materialList[importMeshData.materialID].ambientColor[2] = ambientColor.mData[2];

						std::cout << "\n" << "Ambient color: " << ambientColor.mData[0] << " " << ambientColor.mData[1] <<
							" " << ambientColor.mData[2] << "\n";

						materialList[importMeshData.materialID].diffuseColor[0] = diffuseColor.mData[0];
						materialList[importMeshData.materialID].diffuseColor[1] = diffuseColor.mData[1];
						materialList[importMeshData.materialID].diffuseColor[2] = diffuseColor.mData[2];

						std::cout << "\n" << "Diffuse color: " << diffuseColor.mData[0] << " " << diffuseColor.mData[1] <<
							" " << diffuseColor.mData[2] << "\n";

						/*No specular attributes and shininess for lambert material, so set the values to 0.*/
						materialList[importMeshData.materialID].specularColor[0] = 0;
						materialList[importMeshData.materialID].specularColor[1] = 0;
						materialList[importMeshData.materialID].specularColor[2] = 0;

						materialList[importMeshData.materialID].shinyFactor = 0;
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
	if (deformerCount > 0)
	{
		
	}

	for (unsigned int deformerCounter = 0; deformerCounter < deformerCount; ++deformerCounter)
	{
		FbxSkin* currSkin = reinterpret_cast<FbxSkin*>(inputMesh->GetDeformer(0, FbxDeformer::eSkin));
		if (!currSkin)
			continue;

		const unsigned int clusterCount = currSkin->GetClusterCount();
		for (unsigned int clusterCounter = 0; clusterCounter < clusterCount; ++clusterCounter)
		{
			FbxCluster* currCluster = currSkin->GetCluster(clusterCounter);
			FbxNode* currJoint = currCluster->GetLink();
			FbxAnimEvaluator* animationEvaluator = currJoint->GetAnimationEvaluator();

			FbxAMatrix bindposeTransform = currJoint->EvaluateGlobalTransform();

			int currJointIndex = findJointIndexByName(currJoint->GetName());
			//pmSceneJoints[currJointIndex];

			FbxAMatrix tempBindMatrix;
			FbxAMatrix tempParentBindMatrix;

			currCluster->GetTransformMatrix(tempParentBindMatrix);
			currCluster->GetTransformLinkMatrix(tempBindMatrix);
			


			FbxAMatrix tempGlobalBindPoseInverse;
			tempGlobalBindPoseInverse = tempBindMatrix * tempParentBindMatrix;
			tempGlobalBindPoseInverse = tempGlobalBindPoseInverse.Inverse();
			
			FbxAMatrix tempBindPoseInverse;
			tempBindPoseInverse = tempBindMatrix.Inverse();

			float invBindMatrix[16];
			float invGBindMatrix[16];

			convertFbxMatrixToFloatArray(tempGlobalBindPoseInverse, invGBindMatrix);
			convertFbxMatrixToFloatArray(tempParentBindMatrix, invBindMatrix);
			//convertFbxMatrixToFloatArray(tempInvParentBindMatrix, invParentBindMatrix);

			//push_back the matrices now onto the joint
			for (unsigned int c = 0; c < 16; c++)
			{
				pmSceneJoints[currJointIndex].bindPoseInverse[c] = invBindMatrix[c];
				pmSceneJoints[currJointIndex].globalBindPoseInverse[c] = invGBindMatrix[c];
			}

			//Start processing vertices, add weight and influence to those vertices
			const unsigned int controlPointIndicesCount = currCluster->GetControlPointIndicesCount();
			vector <FbxImport::sBlendData> bdList;

			for (unsigned int i = 0; i < controlPointIndicesCount; ++i)
			{
				FbxImport::sBlendData temp;
				//The index of this joint serves as it's ID
				int jointID = clusterCounter;
				//The control point that this joint affects
				int controlPointIndex = currCluster->GetControlPointIndices()[i];
				//The weight from this joint that the control point get
				float blendingWeight = currCluster->GetControlPointWeights()[i];

				temp.jointID = jointID;
				temp.controlPointIndex = controlPointIndex; //Use controlPointIndex to "find" the joints that affect the control point.
				temp.blendingWeight = blendingWeight;

				bdList.push_back(temp);
			}
			
			const unsigned int polyCount = inputMesh->GetPolygonCount();
			unsigned int indexCounter = 0;
			for (unsigned int polyCounter = 0; polyCounter < polyCount; polyCounter++)
			{
				const unsigned int polySize = inputMesh->GetPolygonSize(polyCounter);

				for (unsigned int polyCorner = 0; polyCorner < polySize; polyCorner++)
				{
					const unsigned index = inputMesh->GetPolygonVertex(polyCounter, polyCorner);
					sBlendData* currBlendData = findBlendDataForControlPoint(bdList, index);
					if (currBlendData->blendingWeight < 0.0001)
					{
						//If the weight is 0, this joint is not an influence of the current vertex. 
						indexCounter++;
						continue;
					}
					//Add the weights and influences to the animated vertex
					//got to make sure that I don't replace shit thats already assignesd
					for (int i = 0; i < 4; i++)
					{
						//If we've already added this joint as an influence to the current vertex, continue to the next vertex.
						if (importMeshData.mSkelVertexList[indexCounter].influences[i] == currBlendData->jointID)
							break;
						if(importMeshData.mSkelVertexList[indexCounter].influences[i] == -1337)
						{
							importMeshData.mSkelVertexList[indexCounter].influences[i] = currBlendData->jointID;
							importMeshData.mSkelVertexList[indexCounter].weights[i] = currBlendData->blendingWeight;
							break;
						}
					}
					indexCounter++;
				}
			}

			//Start processing stacks holding animation layers.
			const unsigned int stackCount = pmScene->GetSrcObjectCount<FbxAnimStack>();
			for (unsigned int stackCounter = 0; stackCounter < stackCount; ++stackCounter)
			{
				FbxAnimStack* currStack = pmScene->GetSrcObject<FbxAnimStack>(stackCounter);

				int layerCount = currStack->GetMemberCount<FbxAnimLayer>();

				//Start processing layers, each working as an "key-data source" 
				for (unsigned int layerCounter = 0; layerCounter < layerCount; layerCounter++)
				{
					FbxAnimLayer* currLayer = currStack->GetMember<FbxAnimLayer>();

					FbxString stackName = currStack->GetName();

					FbxAnimCurve* translationCurveX = currJoint->LclTranslation.GetCurve(currLayer, FBXSDK_CURVENODE_COMPONENT_X);

					

					if (translationCurveX == nullptr)
						continue; 

					const unsigned int keyCount = translationCurveX->KeyGetCount();
					
					sImportAnimationState currAnimation;
					for (unsigned int keyCounter = 0; keyCounter < keyCount; keyCounter++)
					{
						FbxAnimCurveKey currKey = translationCurveX->KeyGet(keyCounter);

						FbxVector4 tempTranslation = animationEvaluator->GetNodeLocalTranslation(currJoint, currKey.GetTime());
						FbxVector4 tempRotation = animationEvaluator->GetNodeLocalRotation(currJoint, currKey.GetTime());
						FbxVector4 tempScale = animationEvaluator->GetNodeLocalRotation(currJoint, currKey.GetTime());

						float keyTime = currKey.GetTime().GetSecondDouble();
						float translation[3] = { tempTranslation[0],  tempTranslation[1], tempTranslation[2] };
						float rotation[3] = { tempRotation[0], tempRotation[1], tempRotation[2] };
						float scale[3] = { tempScale[0], tempScale[1], tempScale[2] };

						//add these values to a sKey-struct, then append it to the keyFrame vector.
						sImportKeyFrame tempKey;
						tempKey.keyTime = keyTime;

						for (unsigned int k = 0; k < 3; k++)
						{
							tempKey.keyPos[k] = translation[k];
							tempKey.keyRotate[k] = rotation[k];
							tempKey.keyScale[k] = scale[k];
						}
						currAnimation.keyList.push_back(tempKey);
					}
					pmSceneJoints[currJointIndex].animationState.push_back(currAnimation);
					//importMeshData.jointList.push_back
				}
			}

			importMeshData.jointList.push_back(pmSceneJoints[currJointIndex]);
			importMeshData.jointList;
			importMeshData.mSkelVertexList;
			importMeshData.mVertexList;
			int momongo = 5;
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

		std::cout << "\n" << "Texturename Nr " << textureCounter + 1 << ": " << fileTextureName << "\n";

		wchar_t* textureToWchar;
		FbxUTF8ToWC(fileTextureName.Buffer(), textureToWchar, NULL);

		materialList[importMeshData.materialID].diffuseTexture = textureToWchar;

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

		std::cout << "\n" << "Texturename Nr " << textureCounter + 1 << ": " << fileTextureName << "\n";

		wchar_t* textureToWchar;
		FbxUTF8ToWC(fileTextureName.Buffer(), textureToWchar, NULL);

		materialList[importMeshData.materialID].specularTexture = textureToWchar;

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

		std::cout << "\n" << "Texturename Nr " << textureCounter + 1 << ": " << fileTextureName << "\n";

		wchar_t* textureToWchar;
		FbxUTF8ToWC(fileTextureName.Buffer(), textureToWchar, NULL);

		materialList[importMeshData.materialID].normalTexture = textureToWchar;

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

	std::cout << "\n" << "Position: " << trans.mData[0] << " " << trans.mData[1] << " " << trans.mData[2] << "\n";

	importMeshData.translate[0] = trans.mData[0];
	importMeshData.translate[1] = trans.mData[1];
	importMeshData.translate[2] = trans.mData[2];

	std::cout << "\n" << "Scale: " << scale.mData[0] << " " << scale.mData[1] << " " << scale.mData[2] << "\n";

	importMeshData.scale[0] = scale.mData[0];
	importMeshData.scale[1] = scale.mData[1];
	importMeshData.scale[2] = scale.mData[2];

	std::cout << "\n" << "Rotation: " << rotat.mData[0] << " " << rotat.mData[1] << " " << rotat.mData[2] << "\n\n";

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
		std::cout << "\n" << "Light nr " << lightCounter++ << ": " << lightName;

		FbxVector4 lightColor = inputLight->Color.Get();

		dirPointData.color[0] = lightColor.mData[0];
		dirPointData.color[1] = lightColor.mData[1];
		dirPointData.color[2] = lightColor.mData[2];

		std::cout << "\n" << "Light color: " << lightColor.mData[0] << " "
			<< lightColor.mData[1] << " " << lightColor.mData[2];

		float lightIntensity = inputLight->Intensity.Get();

		dirPointData.intensity = lightIntensity;

		std::cout << "\n" << "Light intensity: " << lightIntensity;

		FbxAMatrix lightPosition = inputLight->GetNode()->EvaluateGlobalTransform();

		FbxVector4 trans, rot, scal;

		trans = lightPosition.GetT();
		rot = lightPosition.GetR();
		scal = lightPosition.GetS();

		dirPointData.lightPos[0] = trans.mData[0];
		dirPointData.lightPos[1] = trans.mData[1];
		dirPointData.lightPos[2] = trans.mData[2];

		std::cout << "\n" << "Light position: " << trans.mData[0] << " " << trans.mData[1] << " " << trans.mData[2];

		dirPointData.lightScale[0] = rot.mData[0];
		dirPointData.lightScale[1] = rot.mData[1];
		dirPointData.lightScale[2] = rot.mData[2];

		std::cout << "\n" << "Light orientation: " << rot.mData[0] << " " << rot.mData[1] << " " << rot.mData[2];

		dirPointData.lightRot[0] = scal.mData[0];
		dirPointData.lightRot[0] = scal.mData[1];
		dirPointData.lightRot[0] = scal.mData[2];

		std::cout << "\n" << "Light scale: " << scal.mData[0] << " " << scal.mData[1] << " " << scal.mData[2] << "\n\n";

		mDirPointList.push_back(dirPointData);

		/*lightData.countDirectionalPoint = mDirPointList.size();*/
	}

	if (lightType == 2) /*If the light type is a Spotlight.*/
	{
		std::cout << "\n" << "Light nr " << lightCounter++ << ": " << lightName;

		FbxVector4 lightColor = inputLight->Color.Get();

		dirPointData.color[0] = lightColor.mData[0];
		dirPointData.color[1] = lightColor.mData[1];
		dirPointData.color[2] = lightColor.mData[2];

		std::cout << "\n" << "Light color: " << lightColor.mData[0] << " "
			<< lightColor.mData[1] << " " << lightColor.mData[2];

		float lightIntensity = inputLight->Intensity.Get();

		spotData.intensity = lightIntensity;

		std::cout << "\n" << "Light intensity: " << lightIntensity;

		float innerAngle = inputLight->InnerAngle.Get();
		float outerAngle = inputLight->OuterAngle.Get();

		spotData.innerAngle = innerAngle;
		spotData.outerAngle = outerAngle;

		std::cout << "\n" << "Inner angle: " << innerAngle << " " << "Outer angle: " << outerAngle;

		FbxAMatrix lightPosition = inputLight->GetNode()->EvaluateGlobalTransform();

		FbxVector4 trans, rot, scal;

		trans = lightPosition.GetT();
		rot = lightPosition.GetR();
		scal = lightPosition.GetS();

		spotData.lightPos[0] = trans.mData[0];
		spotData.lightPos[1] = trans.mData[1];
		spotData.lightPos[2] = trans.mData[2];

		std::cout << "\n" << "Light position: " << trans.mData[0] << " " << trans.mData[1] << " " << trans.mData[2];

		spotData.lightScale[0] = rot.mData[0];
		spotData.lightScale[1] = rot.mData[1];
		spotData.lightScale[2] = rot.mData[2];

		std::cout << "\n" << "Light orientation: " << rot.mData[0] << " " << rot.mData[1] << " " << rot.mData[2];

		spotData.lightRot[0] = scal.mData[0];
		spotData.lightRot[0] = scal.mData[1];
		spotData.lightRot[0] = scal.mData[2];

		std::cout << "\n" << "Light scale: " << scal.mData[0] << " " << scal.mData[1] << " " << scal.mData[2] << "\n\n";

		mSpotList.push_back(spotData);

		/*lightData.countSpotlight = mSpotList.size();*/
	}
}

void FbxImport::processCamera(FbxCamera * inputCamera)
{
	FbxString cameraName = inputCamera->GetName();

	std::cout << "\n\n" << "Camera nr " << cameraCounter++ << ": " << cameraName << "\n";

	FbxVector4 cameraPos = inputCamera->Position.Get();

	std::cout << "\n" << "Camera position: " << cameraPos.mData[0] << " " << cameraPos.mData[1]
		<< " " << cameraPos.mData[2];

	camData.camPos[0] = cameraPos.mData[0];
	camData.camPos[1] = cameraPos.mData[1];
	camData.camPos[2] = cameraPos.mData[2];

	FbxVector4 camUpVector = inputCamera->UpVector.Get();

	std::cout << "\n" << "Camera Up Vector: " << camUpVector.mData[0] << " " << camUpVector.mData[1]
		<< " " << camUpVector.mData[2];

	camData.upVector[0] = camUpVector.mData[0];
	camData.upVector[1] = camUpVector.mData[1];
	camData.upVector[2] = camUpVector.mData[2];

	float fov = inputCamera->FieldOfView.Get();

	std::cout << "\n" << "Field of view: " << fov << " degrees";

	camData.fieldOfView = fov;

	float nearPlane = inputCamera->NearPlane.Get();

	std::cout << "\n" << "Near plane: " << nearPlane;

	camData.nearPlane = nearPlane;

	float farPlane = inputCamera->FarPlane.Get();

	std::cout << "\n" << "Far plane: " << farPlane;

	camData.farPlane = farPlane;

	mCameraList.push_back(camData);
}

bool FbxImport::checkMaterialName(const char* materialName)
{
	for (int nameIndex = 0; nameIndex < materialList.size(); nameIndex++)
	{
		if (std::strcmp(materialList[nameIndex].materialName, materialName) == 0)
		{
			importMeshData.materialID = nameIndex;
			return false; /*The material names are identical.*/
		}
	}

	if (materialData.materialName == nullptr)
	{
		materialData.materialName = materialName;
		materialList.push_back(materialData);
		importMeshData.materialID = 0;
		firstProcess = false;
		return true; /*This is the first material name.*/
	}

	materialData.materialName = materialName;
	materialList.push_back(materialData);
	importMeshData.materialID = materialList.size() - 1;

	return true; /*The two material names are not identical.*/

}

FbxImport::sBlendData* FbxImport::findBlendDataForControlPoint(std::vector<FbxImport::sBlendData>& inputVector, unsigned int controlPointIndex)
{
	const unsigned int vectorSize = inputVector.size();
	for (unsigned int i = 0; i < vectorSize; i++)
	{
		if (inputVector[i].controlPointIndex == controlPointIndex)
		{
			return &inputVector[i];
		}
	}
	sBlendData noneInfluence;
	//Set 0 to say "This joint doesn't influence that vertex"
	noneInfluence.blendingWeight = 0.0;
	//The jointID of the joint that we're processing
	noneInfluence.jointID = inputVector[0].jointID;
	//The control point that isn't influenced.
	noneInfluence.controlPointIndex = controlPointIndex;
	
	return &noneInfluence;
}



void FbxImport::assignToHeaderData()
{
	mainHeader.meshCount = mTempMeshList.size();
	mainHeader.materialCount = materialList.size();
	mainHeader.lightCount = mDirPointList.size() + mSpotList.size();

	lightData.directionalPointCount = mDirPointList.size();
	lightData.spotlightCount = mSpotList.size();

	mainHeader.cameraCount = mCameraList.size();

	meshList.resize(mainHeader.meshCount);

	mList.resize(mTempMeshList.size());

	std::cout << meshList.size() << "\n";
	cout << "\n\n" << "/////////////Information to binary format/////////////" << "\n\n";

	cout << "Count of meshes: " << mainHeader.meshCount << "\n\n";
	cout << "Count of materials: " << mainHeader.materialCount << "\n\n";
	cout << "Count of lights: " << mainHeader.lightCount << "\n\n";
	cout << "Count of cameras: " << mainHeader.cameraCount << "\n\n";

	cout << "////////////////////////////////////////////////" << "\n\n";

	sVertex vertex;

	for (int sMesh = 0; sMesh < meshList.size(); sMesh++)
	{
		cout << "-------------------------------" << "\n\n";
		cout << "Mesh: " << sMesh + 1 << "\n\n";

		meshList[sMesh].materialID = mTempMeshList[sMesh].materialID;

		meshList[sMesh].translate[0] = mTempMeshList[sMesh].translate[0];
		meshList[sMesh].translate[1] = mTempMeshList[sMesh].translate[1];
		meshList[sMesh].translate[2] = mTempMeshList[sMesh].translate[2];

		meshList[sMesh].rotation[0] = mTempMeshList[sMesh].rotation[0];
		meshList[sMesh].rotation[1] = mTempMeshList[sMesh].rotation[1];
		meshList[sMesh].rotation[2] = mTempMeshList[sMesh].rotation[2];

		meshList[sMesh].scale[0] = mTempMeshList[sMesh].scale[0];
		meshList[sMesh].scale[1] = mTempMeshList[sMesh].scale[1];
		meshList[sMesh].scale[2] = mTempMeshList[sMesh].scale[2];

		meshList[sMesh].vertexCount = mTempMeshList[sMesh].mVertexList.size();

		std::cout << "Vertex count: " << meshList[sMesh].vertexCount << "\n";
		cout << "-------------------------------" << "\n\n";

		cout << "Vertex count: " << meshList[sMesh].vertexCount << "\n\n";

		cout << "Vertex information: " << "\n\n";

		for (int vertices = 0; vertices < mTempMeshList[sMesh].mVertexList.size(); vertices++)
		{
			cout << "Position: " << mTempMeshList[sMesh].mVertexList[vertices].vertexPos[0] << " "
				<< mTempMeshList[sMesh].mVertexList[vertices].vertexPos[1] << " "
				<< mTempMeshList[sMesh].mVertexList[vertices].vertexPos[2] << "\n\n";

			std::cout << "Normal: " << mTempMeshList[sMesh].mVertexList[vertices].vertexNormal[0] << " "
				<< mTempMeshList[sMesh].mVertexList[vertices].vertexNormal[1] << " "
				<< mTempMeshList[sMesh].mVertexList[vertices].vertexNormal[2] << "\n\n";

			std::cout << "UV: " << mTempMeshList[sMesh].mVertexList[vertices].vertexUV[0] << " "
				<< mTempMeshList[sMesh].mVertexList[vertices].vertexUV[1] << "\n\n";

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

void FbxImport::convertFbxMatrixToFloatArray(FbxAMatrix inputMatrix, float inputArray[16])
{
	//THis function assumes row-major matrices

	unsigned int localCounter = 0;
	for (unsigned int g = 0; g < 4; ++g)
	{
		for (unsigned int h = 0; h < 4; ++h)
		{
			inputArray[localCounter] = inputMatrix.Get(g, h);
			localCounter++;
		}
	}
}

void FbxImport::WriteToBinary()
{
	cout << ">>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<" << "\n" << "\n" << endl;
	cout << "Binary Writer" << endl;
	cout << "\n" << endl;
	

	std::ofstream outfile("testBin.bin", std::ofstream::binary);//				Öppnar en fil som är redo för binärt skriv
																//				write header
	outfile.write((const char*)&mainHeader, sizeof(sMainHeader));//				Information av hur många meshes som senare kommer att komma, och efter det hur många material osv, samt hur mycket minne den inten som berättar detta tar upp (reservation för vår header)
	cout << "______________________" << endl;
	cout << "Main Header" << endl;
	cout << "meshCount:" << mainHeader.meshCount << endl;
	cout << "materialCount:" << mainHeader.materialCount << endl;
	cout << "______________________" << endl;
	//cout << mainHeader.lightCount << endl;
	//cout << mainHeader.cameraCount << endl;


	for (int i = 0; i < mainHeader.meshCount; i++)
	{
		cout << "Mesh: " << i << endl;

		outfile.write((const char*)&meshList[i], sizeof(sMesh));//													Information av hur många vertices som senare kommer att komma, och efter det hur många skelAnim verticear som kommer komma osv, samt hur mycket minne den inten som berättar detta tar upp(reservation för vår header).En int kommer först, den har värdet 100.  Och den inten kommer ta upp 4 bytes.

		cout << "Mesh vector: " << endl;

		cout << "\t";
		cout << "xyz: ";
		cout << meshList[i].translate[0];
		cout << meshList[i].translate[1];
		cout << meshList[i].translate[2] << endl;

		cout << "\t";
		cout << "rot: ";
		cout << meshList[i].rotation[0];
		cout << meshList[i].rotation[1];
		cout << meshList[i].rotation[2] << endl;

		cout << "\t";
		cout << "scale: ";
		cout << meshList[i].scale[0];
		cout << meshList[i].scale[1];
		cout << meshList[i].scale[2] << endl;
		
		cout << "\t";
		cout << "Vertex Count: ";
		cout << meshList[i].vertexCount << endl;
		//cout << "SkelAnimVert Count: 0" << endl;
		//cout << "Joint Count: 0"  << endl;

		cout << "\t";
		cout << "Material ID: ";
		cout << meshList[i].materialID << endl;
		//												detta är storleken av innehållet i vList.data()
		
		cout << "\n";
		cout << "Vertex vector: " << endl; 

		cout << "\t";
		cout << mList[i].vList.data() << endl;

		cout << "\t";
		cout << "Allocated memory for " << meshList[i].vertexCount << " vertices" << endl;

		outfile.write((const char*)mList[i].vList.data(), sizeof(sVertex) * meshList[i].vertexCount);//				Skriver ut alla vertices i får vArray, pos, nor, rgba 100 gånger. Och minnet 100 Vertices tar upp.
		
		//cout << "SkelAnimVert vector: NULL" << endl;

		//cout << "Joint vector: NULL" << endl;

		cout << "______________________" << endl;

	}

	for (int i = 0; i < mainHeader.materialCount; i++)
	{
		cout << "Material: " << i << endl;
		
		cout << "Material vector: " << endl;

		cout << "\t";
		cout << &materialList[i] << endl;

		cout << "\t";
		cout << "Allocated memory for " << mainHeader.materialCount << " materials" << endl;

		outfile.write((const char*)&materialList[i], sizeof(sMaterial) * mainHeader.materialCount);//				Information av hur många material som senare kommer att komma, samt hur mycket minne den inten som berättar detta tar upp.
		
		cout << "______________________" << endl;
	}	
}



