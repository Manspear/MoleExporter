#include "FbxImport.h"
using namespace std;

int main()
{
	FbxImport fbx;
	fbx.initializeImporter("Models/twoJointCube.fbx");
	
	fbx.WriteToBinary();

	cin.get();
	return 0;
}
