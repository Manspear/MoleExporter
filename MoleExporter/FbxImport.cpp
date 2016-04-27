#include "FbxImport.h"
#include "HeaderData.h"
#include "ReadHeaderData.h"

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
		currJoint.bBoxID = -1337;
		currJoint.bboxMeshName = nullptr;
		//Adding bbox-children to the joint
		for(int c = 0; c < inNode->GetChildCount(); c++)
		{
			FbxNodeAttribute::EType attributeType = inNode->GetChild(c)->GetNodeAttribute()->GetAttributeType();
			if (attributeType == FbxNodeAttribute::eMesh) 
			{
				char* bBoxName;
				bBoxName = const_cast<char*>(inNode->GetChild(c)->GetName());
				currJoint.bboxMeshName = bBoxName;
			}
		}

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
		printf("Error in FbxImport::findJointIndexByName(const char* jointName): cannot find matching joint name\n");
	}
}

void FbxImport::findBBoxByName(const char * bBoxName, int meshIndex, int jointIndex)
{
	try
	{
		for (unsigned int i = 0; i < mTempMeshList.size(); ++i)
		{
			//Note: * before pointer to get object
			const char* compareName = mTempMeshList[i].meshName;
			int compareValue = std::strcmp(bBoxName, compareName);
			if (compareValue == 0) {
				//asdf
				mTempMeshList[i].isBoundingBox = true;
				mTempMeshList[meshIndex].jointList[jointIndex].bBoxID = mTempMeshList[i].meshID;
			}
		}
	}

	catch (const std::exception&)
	{
		printf("Error in FbxImport::findBBoxByName(const char* bBoxName): cannot find matching mesh name\n");
	}
}

FbxImport::FbxImport()
{
	meshCounter = 0;
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

			importMeshData = sImportMeshData();
			importMeshData.meshName = childNode->GetName();

			strncpy(importMeshData.storeName, childNode->GetName(), 256);

			importMeshData.meshID = meshCounter;
			importMeshData.isBoundingBox = false; //is by default false.

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

	processBoundingBoxes();

	assignToHeaderData();

}

void FbxImport::processMesh(FbxMesh * inputMesh)
{
	/*Reading all the information for a mesh in FBX.*/
	processVertices(inputMesh);

	processNormals(inputMesh);

	processTangents(inputMesh);

	processBiTangents(inputMesh);

	processUVs(inputMesh);

	processMaterials(inputMesh);

	processTextures(inputMesh);

	processTransformations(inputMesh);

	processJoints(inputMesh);

	/*Appending the current mesh to the temporary mesh list.*/
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

				/*The material for the current node is a surface phong.*/
				if (material->GetClassId().Is(FbxSurfacePhong::ClassId))
				{
					const char* materialName = material->GetName();

					bool materialFlag = checkMaterialName(materialName);

					if (materialFlag == false)
					{
						std::cout << "\n" << "Material Number " << materialCounter << ": " << materialName << "\n";

						/*Getting the material attributes that are typical for a surface phong.*/
						ambientColor = ((FbxSurfacePhong *)material)->Ambient;
						diffuseColor = ((FbxSurfacePhong *)material)->Diffuse;
						specularColor = ((FbxSurfacePhong *)material)->Specular;

						float shininess = ((FbxSurfacePhong*)material)->Shininess;

						mMaterialList[importMeshData.materialID].ambientColor[0] = ambientColor.mData[0];
						mMaterialList[importMeshData.materialID].ambientColor[1] = ambientColor.mData[1];
						mMaterialList[importMeshData.materialID].ambientColor[2] = ambientColor.mData[2];

						std::cout << "\n" << "Ambient color: " << ambientColor.mData[0] << " " << ambientColor.mData[1] <<
							" " << ambientColor.mData[2] << "\n";

						mMaterialList[importMeshData.materialID].diffuseColor[0] = diffuseColor.mData[0];
						mMaterialList[importMeshData.materialID].diffuseColor[1] = diffuseColor.mData[1];
						mMaterialList[importMeshData.materialID].diffuseColor[2] = diffuseColor.mData[2];

						std::cout << "\n" << "Diffuse color: " << diffuseColor.mData[0] << " " << diffuseColor.mData[1] <<
							" " << diffuseColor.mData[2] << "\n";

						mMaterialList[importMeshData.materialID].specularColor[0] = specularColor.mData[0];
						mMaterialList[importMeshData.materialID].specularColor[0] = specularColor.mData[0];
						mMaterialList[importMeshData.materialID].specularColor[0] = specularColor.mData[0];

						std::cout << "\n" << "Specular color: " << specularColor.mData[0] << " " << specularColor.mData[1] <<
							" " << specularColor.mData[2] << "\n";

						mMaterialList[importMeshData.materialID].shinyFactor = shininess;

						std::cout << "\n" << "Shininess factor: " << shininess << "\n";

					}
				}

				/*The material for the current node is a surface lambert.*/
				else if (material->GetClassId().Is(FbxSurfaceLambert::ClassId))
				{
					const char* materialName = material->GetName();

					bool materialFlag = checkMaterialName(materialName);

					if (materialFlag == false)
					{
						std::cout << "\n" << "Material Number " << materialCounter << ": " << materialName << "\n";


						/*Getting the material attributes that are typical for a surface lambert.*/
						ambientColor = ((FbxSurfaceLambert *)material)->Ambient;
						diffuseColor = ((FbxSurfaceLambert *)material)->Diffuse;

						mMaterialList[importMeshData.materialID].ambientColor[0] = ambientColor.mData[0];
						mMaterialList[importMeshData.materialID].ambientColor[1] = ambientColor.mData[1];
						mMaterialList[importMeshData.materialID].ambientColor[2] = ambientColor.mData[2];

						std::cout << "\n" << "Ambient color: " << ambientColor.mData[0] << " " << ambientColor.mData[1] <<
							" " << ambientColor.mData[2] << "\n";

						mMaterialList[importMeshData.materialID].diffuseColor[0] = diffuseColor.mData[0];
						mMaterialList[importMeshData.materialID].diffuseColor[1] = diffuseColor.mData[1];
						mMaterialList[importMeshData.materialID].diffuseColor[2] = diffuseColor.mData[2];

						std::cout << "\n" << "Diffuse color: " << diffuseColor.mData[0] << " " << diffuseColor.mData[1] <<
							" " << diffuseColor.mData[2] << "\n";

						/*No specular attributes and shininess can't be found with a lambert material, 
						so we simply set the values for the attributes to 0.*/
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

/*This function checks the material name for each node of the meshes,
so that we know which material ID each mesh have. Several meshes can
have the same material and to save processing, we can use the same
material ID for many meshes with the same material.*/
bool FbxImport::checkMaterialName(const char* materialName)
{
	bool isMatching = false;
	for (int nameIndex = 0; nameIndex < mMaterialList.size(); nameIndex++)
	{
		/*Comparing the characters for all the materials that are in the list
		with the current material that is being processed. If a current material
		have the same name with any of the appended materials in the list, we want
		to give the same material ID.*/
		if (std::strcmp(mMaterialList[nameIndex].materialName, materialName) == 0)
		{
			importMeshData.materialID = nameIndex;
			isMatching = true;
			return isMatching; /*A duplicate material was found.*/
		}
	}

	/*If there are no materials appended to the material list, we can assume
	that this would be the first material and is unique because it will be 
	appended first.*/
	if (firstProcess == true)
	{
		strncpy(gMaterialData.materialName, materialName, 256);

		mMaterialList.push_back(gMaterialData);
		importMeshData.materialID = 0;

		/*Setting the flag to false so we can assume that the first
		material in the list have been appended before.*/
		firstProcess = false;

		return isMatching; /*No duplicate material was found.*/
	}

	/*The current material name is unique and is considered to be
	a new material, appending it to the material list.*/
	strncpy(gMaterialData.materialName, materialName, 256);

	mMaterialList.push_back(gMaterialData);
	importMeshData.materialID = mMaterialList.size() - 1;
	isMatching = false;
	return isMatching; /*No duplicate material was found.*/
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

			//The pos/rot/scale of the joint at bindtime
			FbxAMatrix bindposeTransform = currJoint->EvaluateGlobalTransform();

			int currJointIndex = findJointIndexByName(currJoint->GetName());
			
			for (unsigned int x = 0; x < 3; x++)
			{
				pmSceneJoints[currJointIndex].pos[x] = bindposeTransform.GetT()[x];
				pmSceneJoints[currJointIndex].rot[x] = bindposeTransform.GetR()[x];
				pmSceneJoints[currJointIndex].scale[x] = bindposeTransform.GetS()[x];
			}
			
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
		}
	}
}

void FbxImport::processBoundingBoxes()
{
	char* popo = "kkk";
	const char* ooo = popo;

	const unsigned int meshCount = mTempMeshList.size();
	for (unsigned int i = 0; i < meshCount; i++)
	{
		const unsigned int jointCount = mTempMeshList[i].jointList.size();
		for (unsigned int j = 0; j < jointCount; j++)
		{
			if(mTempMeshList[i].jointList[j].bboxMeshName != nullptr)
				findBBoxByName(mTempMeshList[i].jointList[j].bboxMeshName, i, j);	
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
			/*The game engine supports diffuse-, specular- and normal maps*/
			propDiffus = material->FindProperty(FbxSurfaceMaterial::sDiffuse);
			propSpecular = material->FindProperty(FbxSurfaceMaterial::sSpecular);
			propNormal = material->FindProperty(FbxSurfaceMaterial::sNormalMap);

			FbxTexture* texture;

			/*Checking if the properties for the maps are valid.*/
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
		FbxString fileTextureName = fileTexture->GetFileName();

		std::cout << "\n" << "Texturename Nr " << textureCounter + 1 << ": " << fileTextureName << "\n";

		char* textureToChar = fileTextureName.Buffer();

		char charObject[256];

		strncpy(charObject, textureToChar, 256);

		for (int i = 0; i < 256; i++)
		{
			mMaterialList[importMeshData.materialID].diffuseTexture[i] = charObject[i];
		}

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
		FbxString fileTextureName = fileTexture->GetFileName();

		std::cout << "\n" << "Texturename Nr " << textureCounter + 1 << ": " << fileTextureName << "\n";

		char* textureToChar = fileTextureName.Buffer();

		char charObject[256];

		strncpy(charObject, textureToChar, 256);

		for (int i = 0; i < 256; i++)
		{
			mMaterialList[importMeshData.materialID].specularTexture[i] = charObject[i];
		}

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
		FbxString fileTextureName = fileTexture->GetFileName();

		std::cout << "\n" << "Texturename Nr " << textureCounter + 1 << ": " << fileTextureName << "\n";

		char* textureToChar = fileTextureName.Buffer();

		char charObject[256];

		strncpy(charObject, textureToChar, 256);

		for (int i = 0; i < 256; i++)
		{
			mMaterialList[importMeshData.materialID].normalTexture[i] = charObject[i];
		}

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
		std::cout << "\n" << "Light nr " << lightCounter++ << ": " << lightName << "\n\n"; 

		gLightData.lightID = lightType;

		cout << "Light type: " << lightType << "\n\n";

		FbxVector4 lightColor = inputLight->Color.Get();

		gLightData.color[0] = lightColor.mData[0];
		gLightData.color[1] = lightColor.mData[1];
		gLightData.color[2] = lightColor.mData[2];

		std::cout << "\n" << "Light color: " << lightColor.mData[0] << " "
			<< lightColor.mData[1] << " " << lightColor.mData[2];

		float lightIntensity = inputLight->Intensity.Get();

		gLightData.intensity = lightIntensity;

		std::cout << "\n" << "Light intensity: " << lightIntensity;

		FbxAMatrix lightPosition = inputLight->GetNode()->EvaluateGlobalTransform();

		FbxVector4 trans, rot, scal;

		trans = lightPosition.GetT();
		rot = lightPosition.GetR();
		scal = lightPosition.GetS();

		gLightData.lightPos[0] = trans.mData[0];
		gLightData.lightPos[1] = trans.mData[1];
		gLightData.lightPos[2] = trans.mData[2];

		std::cout << "\n" << "Light position: " << trans.mData[0] << " " << trans.mData[1] << " " << trans.mData[2];

		gLightData.lightScale[0] = rot.mData[0];
		gLightData.lightScale[1] = rot.mData[1];
		gLightData.lightScale[2] = rot.mData[2];

		std::cout << "\n" << "Light orientation: " << rot.mData[0] << " " << rot.mData[1] << " " << rot.mData[2];

		gLightData.lightRot[0] = scal.mData[0];
		gLightData.lightRot[0] = scal.mData[1];
		gLightData.lightRot[0] = scal.mData[2];

		std::cout << "\n" << "Light scale: " << scal.mData[0] << " " << scal.mData[1] << " " << scal.mData[2] << "\n\n";

		mLightList.push_back(gLightData);
	}

	else
	{
		cout << "Light type is not supported." << "\n\n";
	}
}

void FbxImport::processCamera(FbxCamera * inputCamera)
{
	FbxString cameraName = inputCamera->GetName();

	std::cout << "\n\n" << "Camera nr " << cameraCounter++ << ": " << cameraName << "\n";

	FbxVector4 cameraPos = inputCamera->Position.Get();

	std::cout << "\n" << "Camera position: " << cameraPos.mData[0] << " " << cameraPos.mData[1]
		<< " " << cameraPos.mData[2];

	gCamData.camPos[0] = cameraPos.mData[0];
	gCamData.camPos[1] = cameraPos.mData[1];
	gCamData.camPos[2] = cameraPos.mData[2];

	FbxVector4 camUpVector = inputCamera->UpVector.Get();

	std::cout << "\n" << "Camera Up Vector: " << camUpVector.mData[0] << " " << camUpVector.mData[1]
		<< " " << camUpVector.mData[2];

	gCamData.upVector[0] = camUpVector.mData[0];
	gCamData.upVector[1] = camUpVector.mData[1];
	gCamData.upVector[2] = camUpVector.mData[2];

	float fov = inputCamera->FieldOfView.Get();

	std::cout << "\n" << "Field of view: " << fov << " degrees";

	gCamData.fieldOfView = fov;

	float nearPlane = inputCamera->NearPlane.Get();

	std::cout << "\n" << "Near plane: " << nearPlane;

	gCamData.nearPlane = nearPlane;

	float farPlane = inputCamera->FarPlane.Get();

	std::cout << "\n" << "Far plane: " << farPlane;

	gCamData.farPlane = farPlane;

	mCameraList.push_back(gCamData);
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
	/*Function that will assign all the sizes of the node types from the scene 
	and also transfer the mesh data from the temporary list to the real list for
	the binary file exporting.*/

	gMainHeader.meshCount = mTempMeshList.size();
	gMainHeader.materialCount = mMaterialList.size();
	gMainHeader.lightCount = mLightList.size();

	gMainHeader.cameraCount = mCameraList.size();

	meshList.resize(gMainHeader.meshCount);

	mList.resize(mTempMeshList.size());

	std::cout << meshList.size() << "\n";
	cout << "\n\n" << "/////////////Information to binary format/////////////" << "\n\n";

	cout << "Count of meshes: " << gMainHeader.meshCount << "\n\n";
	cout << "Count of materials: " << gMainHeader.materialCount << "\n\n";
	cout << "Count of lights: " << gMainHeader.lightCount << "\n\n";
	cout << "Count of cameras: " << gMainHeader.cameraCount << "\n\n";

	cout << "////////////////////////////////////////////////" << "\n\n";

	sVertex vertex;

	for (int sMesh = 0; sMesh < meshList.size(); sMesh++)
	{
		cout << "-------------------------------" << "\n\n";
		cout << "Mesh: " << sMesh + 1 << "\n\n";

		strncpy(meshList[sMesh].meshName, mTempMeshList[sMesh].storeName, 256);
		
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
	//This function assumes row-major matrices.

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
void FbxImport::WriteToBinary(const char* fileName)
{
	cout << ">>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<" << "\n" << "\n" << endl;
	cout << "Binary Writer" << endl;
	cout << "\n" << endl;

	/*Open a file that is ready for binary writing.*/
	std::ofstream outfile(fileName, std::ofstream::binary);			
	
	/*Writing the first block of memory that is the main header. This will write 
	information about how much of each node type we have from a imported scene and
	how memory they will take up in the binary file.*/
	outfile.write((const char*)&gMainHeader, sizeof(sMainHeader));

	cout << "______________________" << endl;
	cout << "Main Header" << endl;
	cout << "meshCount: " << gMainHeader.meshCount << endl;
	cout << "materialCount: " << gMainHeader.materialCount << endl;
	cout << "lightCount: " << gMainHeader.lightCount << endl;
	cout << "cameraCount: " << gMainHeader.cameraCount << endl;
	cout << "______________________" << endl;

	for (int i = 0; i < gMainHeader.meshCount; i++)
	{
		cout << "Mesh: " << i << endl;
		
		/*Writing the block of memory that is the meshes. The information from the meshes 
		will be written here, that includes for example vertex count for a normal mesh 
		and a skinned mesh. What we do is reserving memory for all the data that is in the
		struct. For example, Vertex count is a integer and will take up to 4 bytes in the 
		memory when writing.*/
		outfile.write((const char*)&meshList[i], sizeof(sMesh));

		cout << "Mesh vector: " << endl;

		cout << "\t";
		cout << "xyz: ";
		cout << meshList[i].translate[0] << " ";
		cout << meshList[i].translate[1] << " ";
		cout << meshList[i].translate[2] << " " << endl;

		cout << "\t";
		cout << "rot: ";
		cout << meshList[i].rotation[0] << " ";
		cout << meshList[i].rotation[1] << " ";
		cout << meshList[i].rotation[2] << " " << endl;

		cout << "\t";
		cout << "scale: ";
		cout << meshList[i].scale[0] << " ";
		cout << meshList[i].scale[1] << " ";
		cout << meshList[i].scale[2] << " " << endl;

		cout << "\t";
		cout << "Vertex Count: ";
		cout << meshList[i].vertexCount << endl;

		/*Do we need to have some kind of special treatment when writing
		the meshes that have skinning???*/

		/*cout << "SkelAnimVert Count: 0" << endl;
		cout << "Joint Count: 0"  << endl;*/

		cout << "\t";
		cout << "Material ID: ";
		cout << meshList[i].materialID << endl;						

		cout << "\n";
		cout << "Vertex vector: " << endl;

		cout << "\t";
		cout << mList[i].vList.data() << endl;

		cout << "\t";
		cout << "Allocated memory for " << meshList[i].vertexCount << " vertices" << endl;

		/*Writing all the vertex lists for each mesh. For example if a mesh have 200 vertices,
		we can multiply the count of vertices with the sizes in bytes that the sVertex struct have.
		This means that we will be writing the pos, nor, uv, tan, bitan 200 times.*/

		outfile.write((const char*)mList[i].vList.data(), sizeof(sVertex) * meshList[i].vertexCount);
																									
		cout << "______________________" << endl;
	}

	for (int i = 0; i < gMainHeader.materialCount; i++)
	{
		cout << "Material: " << i << endl;

		cout << "Material vector: " << endl;

		cout << "\t";
		cout << &mMaterialList[i] << endl;

		cout << "\t";
		cout << "Allocated memory for " << gMainHeader.materialCount << " materials" << endl;

		/*Writing all the materials from the list with the size in bytes in mind.*/
		outfile.write((const char*)&mMaterialList[i], sizeof(sMaterial)); 

		cout << "______________________" << endl;
	}

	for (int i = 0; i < gMainHeader.lightCount; i++)
	{
		cout << "Light: " << i << endl;

		cout << "Light vector: " << endl;

		cout << "\t";
		cout << &mLightList[i] << endl; 

		cout << "\t";
		cout << "Allocated memory for " << gMainHeader.lightCount << " lights" << endl;

		/*Writing all the lights from the list with the size in bytes in mind.*/
		outfile.write((const char*)&mLightList[i], sizeof(sLight));

		cout << "______________________" << endl;
	}

	for (int i = 0; i < gMainHeader.cameraCount; i++)
	{
		cout << "Camera: " << i << endl;

		cout << "Camera vector: " << endl;

		cout << "\t";
		cout << "Allocated memory for " << gMainHeader.cameraCount << " cameras" << endl;

		/*Writing all the cameras from the list with the size in bytes in mind.*/
		outfile.write((const char*)&mCameraList[i], sizeof(sCamera));

		cout << "______________________" << endl;
	}

	outfile.close();
}

void FbxImport::readFromBinary()
{
	/*Reading the binary file that we just have been written to.*/

	std::ifstream infile("testBin.bin", std::ifstream::binary);

	cout << ">>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<" << "\n" << "\n" << endl;
	cout << "Binary Reader" << endl;
	cout << "\n" << endl;

	/*Reading the first block of memory that is the main header. This will read
	information about how much of each node type we have from a imported scene and
	how memory they will take up in the binary file.*/

	infile.read((char*)&read_gMainHeader, sizeof(read_sMainHeader));

	cout << "______________________" << endl;
	cout << "Main Header" << endl;
	cout << "meshCount: " << read_gMainHeader.meshCount << endl;
	cout << "materialCount: " << read_gMainHeader.materialCount << endl;
	cout << "lightCount: " << read_gMainHeader.lightCount << endl;
	cout << "cameraCount: " << read_gMainHeader.cameraCount << endl;
	cout << "______________________" << endl;


	read_meshList.resize(read_gMainHeader.meshCount);

	for (int i = 0; i < read_gMainHeader.meshCount; i++)
	{
		cout << "Mesh: " << i << endl;

		/*Reading the block of memory that is the meshes. The information from the meshes
		will be read here, that includes for example vertex count for a normal mesh
		and a skinned mesh. What we do is reserving memory for all the data that is in the
		struct. For example, Vertex count is a integer and will take up to 4 bytes in the
		memory when reading.*/

		infile.read((char*)&read_meshList[i], sizeof(read_sMesh));									

		cout << "Mesh vector: " << endl;

		cout << "\t";
		cout << "xyz: ";
		cout << read_meshList[i].translate[0];
		cout << read_meshList[i].translate[1];
		cout << read_meshList[i].translate[2] << endl;

		cout << "\t";
		cout << "rot: ";
		cout << read_meshList[i].rotation[0];
		cout << read_meshList[i].rotation[1];
		cout << read_meshList[i].rotation[2] << endl;

		cout << "\t";
		cout << "scale: ";
		cout << read_meshList[i].scale[0];
		cout << read_meshList[i].scale[1];
		cout << read_meshList[i].scale[2] << endl;

		cout << "\t";
		cout << "Vertex Count: ";
		cout << read_meshList[i].vertexCount << endl;

		/*Do we need to have some kind of special treatment when writing
		the meshes that have skinning???*/

		/*cout << "SkelAnimVert Count: 0" << endl;
		cout << "Joint Count: 0"  << endl;*/

		cout << "\t";
		cout << "Material ID: ";
		cout << read_meshList[i].materialID << endl;								

		cout << "\n";
		cout << "Vertex vector: " << endl;

		read_mList.resize(read_gMainHeader.meshCount);
		cout << "mlist: " << endl;
		read_mList[i].vList.resize(read_meshList[i].vertexCount);
		cout << "\t";
		cout << read_mList[i].vList.data() << endl;

		cout << "\t";
		cout << "Allocated memory for " << read_meshList[i].vertexCount << " vertices" << endl;

		read_mList[i].vList.resize(read_meshList[i].vertexCount);

		/*Reading all the vertex lists for each mesh. For example if a mesh have 200 vertices,
		we can multiply the count of vertices with the sizes in bytes that the sVertex struct have.
		This means that we will be reading the pos, nor, uv, tan, bitan 200 times.*/
		infile.read((char*)read_mList[i].vList.data(), sizeof(read_sVertex) * read_meshList[i].vertexCount);

		cout << "______________________" << endl;
	}

	read_materialList.resize(read_gMainHeader.materialCount);

	for (int i = 0; i < read_gMainHeader.materialCount; i++)
	{
		cout << "Material: " << i << endl;

		cout << "Material vector: " << endl;

		cout << "\t";
		cout << &read_materialList[i] << endl;

		cout << "\t";
		cout << "Allocated memory for " << read_gMainHeader.materialCount << " materials" << endl;

		/*Reading all the materials from the list with the size in bytes in mind.*/
		infile.read((char*)&read_materialList[i], sizeof(read_sMaterial));

		cout << "______________________" << endl;
	}

	read_mLightList.resize(gMainHeader.lightCount);

	for (int i = 0; i < gMainHeader.lightCount; i++)
	{
		cout << "Light: " << i << endl;

		cout << "Light vector: " << endl;

		cout << "\t";
		cout << &read_mLightList[i] << endl;

		cout << "\t";
		cout << "Allocated memory for " << read_gMainHeader.lightCount << " lights" << endl;

		/*Reading all the lights from the list with the size in bytes in mind.*/
		infile.read((char*)&read_mLightList[i], sizeof(sLight));

		cout << "______________________" << endl;
	}

	read_mCameraList.resize(gMainHeader.cameraCount);

	for (int i = 0; i < gMainHeader.cameraCount; i++)
	{
		cout << "Camera: " << i << endl;

		cout << "Camera vector: " << endl;

		cout << "\t";
		cout << "Allocated memory for " << read_gMainHeader.cameraCount << " cameras" << endl;

		/*Reading all the cameras from the list with the size in bytes in mind.*/
		infile.read((char*)&read_mCameraList[i], sizeof(sCamera));

		cout << "______________________" << endl;
	}

	infile.close();
}