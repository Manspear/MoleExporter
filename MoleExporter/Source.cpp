#include "FbxImport.h"
using namespace std;

int main()
{
	FbxImport fbx;
	fbx.initializeImporter("Models/klara_alkb4.fbx", 24);
	
	fbx.WriteToBinary("klara_alkb4.mole");

	cin.get();
	return 0;
}
