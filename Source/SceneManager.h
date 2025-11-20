#pragma once

#include "Scene.h"

// シーンマネージャー
class SceneManager
{
private:
	SceneManager() {}
	~SceneManager() {}

public:
	// 唯一のインスタンス取得
	static SceneManager& Instance()
	{
		static SceneManager instance;
		return instance;
	}

	// 更新処理
	void Update(float elapsedTime);

	// 描画処理
	void Render();

	// GUI描画
	void DrawGUI();

	// シーンクリア
	//管理しているシーンの終了処理を行う関数
	void Clear();

	// シーン切り替え
	void ChangeScene(Scene* scene);

	Scene* GetCurrentScene()const { return currentScene; }

	//名前指定でのシーン切り替え
	void ChangeSceneByName(const std::string& scene_name);

private:
	Scene* currentScene = nullptr;
	Scene* nextScene = nullptr;
};
