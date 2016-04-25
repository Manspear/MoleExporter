#include "FbxImport.h"
using namespace std;

int main()
{
	FbxImport fbx;
	fbx.initializeImporter("Models/jesper_test.fbx");
	
	fbx.WriteToBinary();
	fbx.readFromBinary();

	cin.get();
	return 0;
}
