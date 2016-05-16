#include "FbxImport.h"
using namespace std;

int main()
{
	FbxImport fbx;
	fbx.initializeImporter("Models/klara41.fbx", 24);

	fbx.WriteToBinary("klara42_demo.mole");

	cin.get();
	return 0;
}
