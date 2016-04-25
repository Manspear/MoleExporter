#include "FbxImport.h"
using namespace std;

int main()
{
	FbxImport fbx;
	fbx.initializeImporter("Models/jointCube_With_BBox.fbx");
	
	fbx.WriteToBinary();

	cin.get();
	return 0;
}
