#pragma once

#include <string>
#include <memory>
#include <vector>

#include <nlohmann/json.hpp>

#include "Editor.h"
#include "GameObject.h"

class GameObject;
struct SpriteObject;

using json = nlohmann::json;

//JSONへ変換
void to_json(json& j, const GameObject& obj);
void to_json(json& j, const SpriteObject& sp);
//JSONから復元
void from_json(const json& j, GameObject& obj);
void from_json(const json& j, SpriteObject& sp);


class Serializer
{
public:
	static void GlobalGameManagersClear();

	//データをファイルに保存
	static void SaveScene(
		const std::vector<std::shared_ptr<GameObject>>& objects,
		const std::vector<std::unique_ptr<SpriteObject>>& sprites,
		const std::string& filename
	);

	//ファイルからデータを復元
	static bool LoadScene(
		std::vector<std::shared_ptr<GameObject>>& objects,
		std::vector<std::unique_ptr<SpriteObject>>& sprites,
		const std::string& filename
	);

private:
	Serializer() = delete;
};

