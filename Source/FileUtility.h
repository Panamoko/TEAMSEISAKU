#pragma once

#include <vector>
#include <string>

class FileUtility
{
public:
	//.jsonƒtƒ@ƒCƒ‹–¼‚Ìˆê——‚ðŽæ“¾
	static std::vector<std::string> GetSceneFileNames(const std::string& directory_path);
private:
	FileUtility() = delete;
};

