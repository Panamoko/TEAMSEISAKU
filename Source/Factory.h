#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <functional>

#include "GameObject.h"

class Factory
{
public:
	using CreateFunc = std::function<std::unique_ptr<GameObject>()>;

	//ƒNƒ‰ƒX–¼¨¶¬ŠÖ”‚Ìƒ}ƒbƒv
	static std::unordered_map<std::string, CreateFunc>& Registry();

	//“o˜^ŠÖ”
	static void Register(const std::string& className, CreateFunc func);

	//¶¬ŠÖ”
	static std::unique_ptr<GameObject> Create(const std::string& className);

	//“o˜^Ï‚İƒNƒ‰ƒXˆêŠ‡æ“¾
	static std::vector<std::string> GetRegisteredClassNames();
};

//©“®“o˜^ƒ}ƒNƒ
#define REGISTER_GAMEOBJECT(ClassType) \
namespace { \
    struct ClassType##Register { \
        ClassType##Register() { \
            Factory::Register(#ClassType, []() { return std::make_unique<ClassType>(); }); \
        } \
    }; \
    static ClassType##Register global_##ClassType##Register; \
}