#include "FbxImport.h"
using namespace std;

int main()
{
	FbxImport fbx;
	fbx.initializeImporter("Models/klara3_triangulated.fbx", 24);
	
	fbx.WriteToBinary("klara3.mole");

	cin.get();
	return 0;
}
