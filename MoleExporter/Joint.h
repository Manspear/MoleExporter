#pragma once

#include <vector>
class Joint 
{
public:

	struct sKeyData
	{
		float keyTransform[16];
		float keyTime;
	};
	/**
	The keys implicitly know which joint they belong to.
	**/
	struct sJointData
	{
		const char* name;
		unsigned int parentJointIndex;
		unsigned int jointIndex;
		float globalBindPoseInverse[16];
		std::vector<sKeyData> setKeys;
	};

	sJointData mJointData;

	Joint();
	~Joint();
};