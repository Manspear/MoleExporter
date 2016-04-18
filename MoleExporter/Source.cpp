#include <iostream>
using namespace std;

#include "FbxImport.h"

int main()
{
	FbxImport fbx;
	//"C:/Users/Dator/Documents/MoleExporter/MoleExporter/Models/jointCube.fbx"
	fbx.initializeImporter("Models/jointCube.fbx");
	fbx.WriteToBinary();
	//int baby = 0; Have linked.
	cin.get();
	return 0;
}
