#include "SceneManager.h"
#include "SceneFactory.h"
#include "SceneLoading.h"

// 更新処理
void SceneManager::Update(float elapsedTime)
{
	if (nextScene != nullptr)
	{
		// 古いシーンを終了処理
		Clear();

		// 新しいシーンを設定
		currentScene = nextScene;
		nextScene = nullptr;

		// シーン初期化処理
		//currentScene->Initialize();
		if (!currentScene->IsReady())	//ゲームシーンが準備済みなら
		{
			currentScene->Initialize();
		}
	}

	if (currentScene != nullptr)
	{
		currentScene->Update(elapsedTime);
	}
}

// 描画処理
void SceneManager::Render()
{
	if (currentScene != nullptr)
	{
		currentScene->Render();
	}
}

void SceneManager::DrawGUI()
{
	if (currentScene != nullptr)
	{
		currentScene->DrawGUI();
	}
}

// シーンクリア
void SceneManager::Clear()
{
	if (currentScene != nullptr)
	{
		currentScene->Finalize();
		delete currentScene;
		currentScene = nullptr;
	}
}

// シーン切り替え
void SceneManager::ChangeScene(Scene* scene)
{
	nextScene = scene;
}

//名前指定でのシーン切り替え
void SceneManager::ChangeSceneByName(const std::string& scene_name)
{
	//Factory を使って新しいシーンのインスタンスを生成
	Scene* newScene = SceneFactory::CreateScene(scene_name);

	if (newScene)
	{
		//既存の ChangeScene メソッドに生成したインスタンスを渡す
		ChangeScene(new SceneLoading(newScene));
	}
	else
	{
		//エラーログ
		std::cerr << "Error: ChangeSceneByName failed for scene: " << scene_name << std::endl;
	}
}
