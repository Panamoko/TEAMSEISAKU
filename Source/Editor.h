#pragma once

#include <vector>
#include <string>
#include <memory>
#include <DirectXMath.h>

#include <nlohmann/json.hpp>
#include <fstream>

#include "System/ModelRenderer.h"
#include "ModelManager.h"
#include "GameObject.h"
#include "Factory.h"
#include "Scene.h"
#include "Serializer.h"

using json = nlohmann::json;

struct SpriteObject
{
	std::string name;
	std::string texture;
	DirectX::XMFLOAT2 position;
	DirectX::XMFLOAT2 size;
	float rotation;
	DirectX::XMFLOAT4 color;

	int sprite_index;

	// 追加：UV座標（0～1範囲）
	DirectX::XMFLOAT2 uv_min;  // 左上
	DirectX::XMFLOAT2 uv_max;  // 右下

	SpriteObject();
};

//3D空間での最終変換を反映
void ApplyTransform(GameObject& obj);

class editor :public GameObject
{
public:
	editor();//初期化
	~editor();//終了処理
	//ImGui描画
	void render(
		std::vector<std::shared_ptr<GameObject>>& objects,
		std::vector<std::unique_ptr<SpriteObject>>& sprites,
		const std::vector<std::unique_ptr<Model>>& models,
		ModelRenderer* model);
	//オブジェクト追加
	void AddObject(
		std::vector<std::shared_ptr<GameObject>>& objects,
		const std::string& class_name,
		const std::string& baseName,
		int mesh_index = -1
		//const std::string& modelPath
	);

	void AddSprite(
		std::vector<std::unique_ptr<SpriteObject>>& sprites,
		const std::string& baseName,
		int texture_index);

	void Draw3DEditor(
		std::vector<std::shared_ptr<GameObject>>& objects,
		const std::vector<std::unique_ptr<Model>>& models,
		ModelRenderer* renderer                       // ← レンダラー
	);

	void Draw2DEditor(
		std::vector<std::unique_ptr<SpriteObject>>& sprites);

	void Delete3DModel(const std::shared_ptr<GameObject>& obj);

	void CreatNewScene(const std::string& scene_name);

private:

	enum class EditorModel { Model3D, Model2D };
	enum class GameMode { Edit, Play };
public:
	//モード切替
	void ToggleMode(
		std::vector<std::shared_ptr<GameObject>>& objects,
		std::vector<std::unique_ptr<SpriteObject>>& sprites);
	GameMode GetMode()const { return editor_mode; }
	bool PlayGame()const { return play; };
private:

	EditorModel mode = EditorModel::Model3D;
	GameMode editor_mode = GameMode::Edit;

	int select_index;//選択中インデックス
	int delete_index;//削除待ちインデックス

	int select_index2D;
	int delete_index2D;

	int type_index = 0;
	int static_index = 0;
	int skinned_index = 0;
	int sprite_index = 0;
	std::string select_class;
	bool play = true;

	Model* model = nullptr;

	// モデルリストを参照
	std::vector<const char*> names;

	//同じ名前が存在しないように、新しいオブジェクト名を自動生成
	std::string MakeUniqueName(const std::vector<std::shared_ptr<GameObject>>& objects, const std::string& base);
	std::string MakeUniqueSpriteName(const std::vector<std::unique_ptr<SpriteObject>>& sprites, const std::string& base);
};

