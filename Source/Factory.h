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

	//クラス名→生成関数のマップ
	static std::unordered_map<std::string, CreateFunc>& Registry();

	//登録関数
	static void Register(const std::string& className, CreateFunc func);

	//生成関数
	static std::unique_ptr<GameObject> Create(const std::string& className);
};

//自動登録マクロ
#define REGISTER_GAMEOBJECT(ClassType) \
namespace { \
    struct ClassType##Register { \
        ClassType##Register() { \
            Factory::Register(#ClassType, []() { return std::make_unique<ClassType>(); }); \
        } \
    }; \
    static ClassType##Register global_##ClassType##Register; \
}