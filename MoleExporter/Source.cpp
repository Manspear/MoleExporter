#include "FbxImport.h"
using namespace std;

int main()
{
	FbxImport fbx;
	fbx.initializeImporter("Models/molerat.fbx");
	
	fbx.WriteToBinary("testBox2.mole");

	cin.get();
	return 0;
}
