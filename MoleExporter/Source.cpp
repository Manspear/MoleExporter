#include "FbxImport.h"
using namespace std;

int main()
{
	FbxImport fbx;
	fbx.initializeImporter("Models/Kurtj4ckB0x.fbx");
	
	fbx.WriteToBinary("Kurtj4ckB0x.mole");
	/*fbx.readFromBinary("testBox.mole");*/

	cin.get();
	return 0;
}
