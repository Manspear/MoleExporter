#include "FbxImport.h"
using namespace std;

int main()
{
	FbxImport fbx;
	fbx.initializeImporter("Models/fbx_test.fbx");
	
	fbx.WriteToBinary();
	fbx.readFromBinary();

	cin.get();
	return 0;
}
