#include "FbxImport.h"
using namespace std;

int main()
{
	FbxImport fbx;
	fbx.initializeImporter("Models/fbx_test.fbx");
	
	fbx.WriteToBinary("testBox.mole");
	fbx.readFromBinary("testBox.mole");

	cin.get();
	return 0;
}
