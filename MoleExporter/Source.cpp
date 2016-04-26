#include "FbxImport.h"
using namespace std;

int main()
{
	FbxImport fbx;
	fbx.initializeImporter("Models/twoJointCubes.fbx");
	
	fbx.WriteToBinary();
	fbx.readFromBinary();

	cin.get();
	return 0;
}
