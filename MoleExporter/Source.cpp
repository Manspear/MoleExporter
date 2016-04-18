#include <iostream>
using namespace std;

#include "FbxImport.h"

int main()
{
	FbxImport fbx;

	fbx.initializeImporter();
	fbx.WriteToBinary();
	//int baby = 0; Have linked.
	cin.get();
	return 0;
}
