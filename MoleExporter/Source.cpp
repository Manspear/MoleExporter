#include "FbxImport.h"
using namespace std;

int main()
{
	FbxImport fbx;
	fbx.initializeImporter("Models/rotationCube3.fbx", 24);
	
	fbx.WriteToBinary("rotationCube3.mole");

	cin.get();
	return 0;
}
