#include "FbxImport.h"
using namespace std;

int main()
{
	FbxImport fbx;
	fbx.initializeImporter("Models/jointTwoCubeHierarchy.fbx");
	
	fbx.WriteToBinary("testBox2.mole");
	fbx.readFromBinary("testBox.mole");

	cin.get();
	return 0;
}
