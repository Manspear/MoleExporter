#include "FbxImport.h"
using namespace std;

int main()
{
	FbxImport fbx;
	fbx.initializeImporter("Models/twoJointCube_Animated60fps.fbx");
	
	fbx.WriteToBinary();

	cin.get();
	return 0;
}
