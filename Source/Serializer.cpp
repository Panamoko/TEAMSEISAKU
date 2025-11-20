#include "Serializer.h"
#include <fstream>
#include <iostream>

#include "Factory.h"

//JSONへ変換
void to_json(json& j, const GameObject& obj)
{
	j = json{
		{"id",obj.id},
		{"name",obj.name},
		{"class_name",obj.class_name},
		{"position",{obj.position.x,obj.position.y,obj.position.z}},
		{"rotation",{obj.angle.x,obj.angle.y,obj.angle.z}},
		{"scale",{obj.scale.x,obj.scale.y,obj.scale.z}},
		{"color",{obj.color.x,obj.color.y,obj.color.z,obj.color.w}},
		{"type",static_cast<int>(obj.type)},
		{"mesh_index",obj.mesh_index},
		{"model_path",obj.model_path}
	};
}

void to_json(json& j, const SpriteObject& sp)
{
	j = nlohmann::json{
		 {"name", sp.name},
		 {"texture", sp.texture},
		 {"position", {sp.position.x, sp.position.y}},
		 {"size", {sp.size.x, sp.size.y}},
		 {"rotation", sp.rotation},
		 {"color", {sp.color.x, sp.color.y, sp.color.z, sp.color.w}},
		 {"sprite_index", sp.sprite_index},
		 {"uv_min", {sp.uv_min.x, sp.uv_min.y}},
		 {"uv_max", {sp.uv_max.x, sp.uv_max.y}}
	};
}

//JSONから復元
void from_json(const json& j, GameObject& obj)
{
	obj.id = j.at("id").get<int>();
	obj.name = j.at("name").get<std::string>();
	obj.class_name = j.at("class_name").get<std::string>();
	auto pos = j.at("position");
	obj.position = { pos[0],pos[1],pos[2] };
	auto rot = j.at("rotation");
	obj.angle = { rot[0],rot[1],rot[2] };
	auto sca = j.at("scale");
	obj.scale = { sca[0],sca[1],sca[2] };
	auto col = j.value("color", std::vector<float>{1.0f, 1.0f, 1.0f, 1.0f});
	obj.color = { col[0],col[1],col[2],col[3] };
	obj.type = static_cast<GameObject::Type>(j.at("type").get<int>());
	obj.mesh_index = j.at("mesh_index").get<int>();
	obj.model_path = j.value("model_path", "");
	if (!obj.model_path.empty())
	{
		obj.model = ModelManager::Instance().Load(obj.model_path);
	}
	ApplyTransform(obj);
}

void from_json(const json& j, SpriteObject& sp)
{
	sp.name = j.at("name").get<std::string>();
	sp.texture = j.at("texture").get<std::string>();
	auto pos = j.at("position");
	sp.position = { pos[0], pos[1] };
	auto size = j.at("size");
	sp.size = { size[0], size[1] };
	sp.rotation = j.at("rotation").get<float>();
	auto col = j.at("color");
	sp.color = { col[0], col[1], col[2], col[3] };
	if (j.contains("sprite_index"))
		sp.sprite_index = j.at("sprite_index").get<int>();
	else
		sp.sprite_index = 0;

	if (j.contains("uv_min"))
	{
		auto uv = j.at("uv_min");
		sp.uv_min = { uv[0].get<float>(), uv[1].get<float>() };
	}
	else
		sp.uv_min = { 0.0f, 0.0f };

	if (j.contains("uv_max"))
	{
		auto uv = j.at("uv_max");
		sp.uv_max = { uv[0].get<float>(), uv[1].get<float>() };
	}
	else
		sp.uv_max = { 1.0f, 1.0f };
}

//データをファイルに保存
void Serializer::SaveScene(
	const std::vector<std::shared_ptr<GameObject>>& objects,
	const std::vector<std::unique_ptr<SpriteObject>>& sprites,
	const std::string& filename)
{
	json j;

	//GameObjectの保存
	for (const auto& obj : objects)
	{
		j["objects"].push_back(*obj);
	}

	//SpriteObjectの保存
	for (const auto& sp : sprites)
	{
		j["sprites"].push_back(*sp);
	}

	std::ofstream file(filename);
	if (file.is_open())
	{
		file << j.dump(4);
	}
	else
	{
		//エラー処理
		std::cerr << "Error: Could not open file for saving: " << filename << std::endl;
	}

}

//ファイルからデータを復元
bool Serializer::LoadScene(
	std::vector<std::shared_ptr<GameObject>>& objects,
	std::vector<std::unique_ptr<SpriteObject>>& sprites,
	const std::string& filename)
{
	std::ifstream file(filename);
	if (!file.is_open()) return false;

	json j;
	try
	{
		file >> j;
	}
	catch (const std::exception& e)
	{
		// JSONパスエラー処理
		std::cerr << "JSON Parse Error on loading " << filename << ": " << e.what() << std::endl;
		return false;
	}

	//既存のリストをクリア
	objects.clear();

	if (j.contains("objects") && j["objects"].is_array())
	{
		for (auto& item : j["objects"])
		{
			std::string className = item.value("class_name", "GameObject");

			std::shared_ptr<GameObject> obj = Factory::Create(className);

			//Factory が失敗した場合はデフォルトの GameObject を作成
			if (!obj)obj = std::make_shared<GameObject>();

			from_json(item, *obj);
			objects.push_back(std::move(obj));
		}
	}

	sprites.clear();
	//sprites (SpriteObject) の復元
	if (j.contains("sprites") && j["sprites"].is_array())
		for (auto& item : j["sprites"])
		{
			// unique_ptr を使用し、from_json でコピーを避けて復元
			auto sp = std::make_unique<SpriteObject>();
			from_json(item, *sp);
			sprites.push_back(std::move(sp));
		}

	//復元後にIDの重複を修正
	FixDuplicateIDs(objects);

	return true;
}
