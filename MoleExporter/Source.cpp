#include "FbxImport.h"
using namespace std;

int main()
{
	FbxImport fbx;
	fbx.initializeImporter("Models/smoothDisabled.fbx");
	
	fbx.WriteToBinary();

	cin.get();
	return 0;
}
