#include "FbxImport.h"
using namespace std;

int main()
{
	FbxImport fbx;
	fbx.initializeImporter("Models/cubeTest.fbx");
	
	fbx.WriteToBinary("testBox.mole");
	fbx.readFromBinary();

	cin.get();
	return 0;
}
