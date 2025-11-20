#pragma once

#include <string>
#include <memory>
#include "Scene.h"

//シーン生成関数の型定義
using SceneCreatorFunc = Scene * (*)();

class SceneFactory
{
public:
	//シーンクラス名からインスタンスを生成
	static Scene* CreateScene(const std::string& className);

	//シーンクラスと生成関数を登録
	static void RegisterScene(const std::string& className, SceneCreatorFunc func);

private:
	static std::map<std::string, SceneCreatorFunc> s_creators;
};

// ヘルパーマクロ
#define REGISTER_SCENE(ClassName) \
    namespace { \
        Scene* Create##ClassName() { return new ClassName(); } \
        struct ClassName##Register { \
            ClassName##Register() { SceneFactory::RegisterScene(#ClassName, Create##ClassName); } \
        }; \
        static ClassName##Register register_##ClassName; \
    }
