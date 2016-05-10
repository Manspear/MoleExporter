#include "FbxImport.h"
using namespace std;

int main()
{
	FbxImport fbx;
	fbx.initializeImporter("Models/animTestCylinder.fbx", 24);
	
	fbx.WriteToBinary("animTestCylinder.mole");

	cin.get();
	return 0;
}
