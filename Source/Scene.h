#pragma once
#include <vector>
#include <string>
#include <memory>
#include <Editor.h>
#include "GameObject.h"

// シーン
class Scene
{
public:
	Scene() {}
	virtual ~Scene() {}

	// 初期化
	virtual void Initialize() = 0;

	// 終了化
	virtual void Finalize() = 0;

	// 更新処理
	virtual void Update(float elapsedTime) = 0;

	// 描画処理
	virtual void Render() = 0;

	// GUI描画
	virtual void DrawGUI() = 0;

	// 準備完了しているか
	bool IsReady() const { return ready; }

	// 準備完了設定
	void SetReady() { ready = true;	}

	std::string GetSceneName() { return scene_name; }

	std::string SetSceneName(std::string name) { return scene_name = name; }

private:
	bool	ready = false;

protected:
	std::string scene_name;

	//各シーン専用のオブジェクト
	std::vector<std::shared_ptr<GameObject>> objects;
	std::vector<std::shared_ptr<GameObject>> sprites;
	Scene* scene;
};
