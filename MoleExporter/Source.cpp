#include "FbxImport.h"
using namespace std;

int main()
{
	FbxImport fbx;
	fbx.initializeImporter("Models/Molebat.fbx");
	
	fbx.WriteToBinary("molebat.mole");

	cin.get();
	return 0;
}
