#include "Editor.h"


#include "imgui.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <unordered_set>

using namespace DirectX;

static float DegToRed(float d) { return d * XM_PI / 180.0f; }

//JSONへ変換
void to_json(json& j, const GameObject& obj)
{
	j = json{
		{"name",obj.name},
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
	obj.name = j.at("name").get<std::string>();
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

//現在の情報を保存
void SaveScene(
	const std::vector<std::unique_ptr<GameObject>>& objects,
	const std::vector<std::unique_ptr<SpriteObject>>& sprites,
	const std::string& filename)
{
	json j;
	for (auto& obj : objects)
	{
		j["objects"].push_back(*obj);
	}
	for (auto& sp : sprites)
	{
		j["sprites"].push_back(*sp);
	}
	std::ofstream file(filename);
	file << j.dump(4);
}


//保存した情報を復元
void LoadScene(
	std::vector<std::unique_ptr<GameObject>>& objects,
	std::vector<std::unique_ptr<SpriteObject>>& sprites,
	const std::string& filename)
{
	std::ifstream file(filename);
	if (!file.is_open())return;

	json j;
	file >> j;

	objects.clear();
	if (j.contains("objects"))
		for (auto& item : j["objects"])
		{
			auto obj = std::make_unique<GameObject>();
			*obj = item.get<GameObject>();
			objects.push_back(std::move(obj));
		}

	sprites.clear();
	if (j.contains("sprites"))
		for (auto& item : j["sprites"])
		{
			auto sp = std::make_unique<SpriteObject>();
			*sp = item.get<SpriteObject>();
			sprites.push_back(std::move(sp));
		}
}

void ApplyTransform(GameObject& obj)
{
	/*XMFLOAT3をDirectXMath が使う高速ベクトル型 XMVECTOR に変換*/
	XMVECTOR pos = XMLoadFloat3(&obj.position);
	XMVECTOR rot = XMVectorSet(
		XMConvertToRadians(obj.angle.x),
		XMConvertToRadians(obj.angle.y),
		XMConvertToRadians(obj.angle.z),
		0.0f
	);	XMVECTOR scl = XMLoadFloat3(&obj.scale);

	XMMATRIX T = XMMatrixTranslationFromVector(pos);//位置ベクトルから平行移動行列を作成
	XMMATRIX R = XMMatrixRotationRollPitchYawFromVector(rot);//回転ベクトルから回転行列を作成
	XMMATRIX S = XMMatrixScalingFromVector(scl);//スケール（拡大縮小）行列を作成

	//scale -> rotate -> translate
	XMMATRIX world = S * R * T;//最終的なワールド変換行列を計算
	XMStoreFloat4x4(&obj.transform, world);//結果を obj.world に格納
}

editor::editor()
	: select_index(-1)
	, delete_index(-1)
	, select_index2D(-1)
	, delete_index2D(-1)
	, type_index(0)
	, static_index(0)
	, skinned_index(0) {
};

//a
editor::~editor()
{
}

void editor::render(
	std::vector<std::unique_ptr<GameObject>>& objects,
	std::vector<std::unique_ptr<SpriteObject>>& sprites,
	const std::vector<std::unique_ptr<Model>>& models,
	ModelRenderer* renderer)
{
	ImGui::Begin("Editor");

	if (ImGui::Button("Save Scene"))
	{
		SaveScene(objects, sprites, "scene.json");
	}

	ImGui::SameLine();

	if (ImGui::Button("Load Scene"))
	{
		LoadScene(objects, sprites, "scene.json");
	}

	ImGui::Separator();

	if (ImGui::Button("3D Mode"))mode = EditorModel::Model3D;

	ImGui::SameLine();

	if (ImGui::Button("2D Mode"))mode = EditorModel::Model2D;

	ImGui::Separator();

	if (mode == EditorModel::Model3D)
	{
		ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "[3D Object Mode]");
		Draw3DEditor(objects, models, renderer);
	}
	else
	{
		ImGui::TextColored(ImVec4(0.5f, 0.7f, 1.0f, 1.0f), "[2D Sprite Mode]");
		Draw2DEditor(sprites);
	}

	//Sceneウィンドウの終了
	ImGui::End();

}

void editor::AddObject(
	std::vector<std::unique_ptr<GameObject>>& objects,
	const std::string& baseName,
	int mesh_index
	 )
{
	std::string name = MakeUniqueName(objects, baseName);//objects中に重複しない名前を生成
	
	//各クラスと紐づけ
	std::unique_ptr<GameObject> obj = Factory::Create(baseName);
	
	//auto obj = std::make_unique<GameObject>();
	obj->name = name;//新しいオブジェクトの name メンバに代入
	obj->mesh_index = mesh_index;

	//モデルを紐づけ
	auto& modelList = ModelManager::Instance().GetModels();
	if (mesh_index >= 0 && mesh_index < (int)modelList.size())
	{
		obj->model = modelList[mesh_index].get();

		obj->model_path = modelList[mesh_index]->path;
	}

	ApplyTransform(*obj);//world 行列を計算して設定
	objects.push_back(std::move(obj));
	select_index = static_cast<int>(objects.size() - 1);//追加したオブジェクトを選択状態にする
}

void editor::AddSprite(
	std::vector<std::unique_ptr<SpriteObject>>& sprites,
	const std::string& baseName,
	int texture_index)
{
	std::string name = MakeUniqueSpriteName(sprites, baseName);
	auto sp = std::make_unique<SpriteObject>();
	sp->name = name;
	sp->texture = baseName;
	sp->sprite_index = texture_index;
	sprites.push_back(std::move(sp));
	select_index2D = static_cast<int>(sprites.size() - 1);
}

void editor::Draw3DEditor(
	std::vector<std::unique_ptr<GameObject>>& objects,
	const std::vector<std::unique_ptr<Model>>& models,
	ModelRenderer* renderer                       // ← レンダラー
)
{
	ImGui::Text("Add New Object");

	auto& model = ModelManager::Instance().GetModels();

	names.clear();
	std::unordered_set<std::string> addedNames;

	for (auto& m : models)
	{
		if (m && addedNames.insert(m->name).second)
			names.push_back(m->name.c_str());
	}

	if (!names.empty())
		ImGui::Combo("Model", &static_index, names.data(), (int)names.size());
	else
		ImGui::TextDisabled("No models loaded.");


	if (ImGui::Button("Add"))
	{
		if (!models.empty())
		{
			int modelIndex = static_index;
			AddObject(
				objects,
				models[modelIndex]->name,
				modelIndex);
		}
	}

	//子ウィンドウ作成
	ImGui::BeginChild("Object_list", ImVec2(250, 0), true);
	ImGui::Text("Object_list");
	ImGui::Separator();

	//選択可能なリストアイテムを表示
	for (size_t i = 0; i < objects.size(); i++)
	{
		GameObject* obj = objects[i].get();
		if (ImGui::Selectable(obj->name.c_str(), select_index == (int)i))
		{
			select_index = (int)i;
		}
	}

	ImGui::EndChild();//子ウィンドウを閉じる

	ImGui::SameLine();//次のUIを同じ行に続けて描くよう指示

	//幅自動の子ウィンドウを作成し、見出しと区切りを描画
	ImGui::BeginChild("Inspector", ImVec2(0, 0), true);
	ImGui::Text("Inspector");
	ImGui::Separator();

	//選択中のインデックスが有効かをチェック
	if (select_index >= 0 && select_index < (int)objects.size())
	{
		GameObject* sel = objects[select_index].get();

		//名前編集用の入力欄
		char buf[128];
		strncpy_s(buf, sizeof(buf), sel->name.c_str(), _TRUNCATE);
		if (ImGui::InputText("Name", buf, sizeof(buf)))
		{
			sel->name = std::string(buf);
		}

		//位置を編集可能にする
		if (ImGui::DragFloat3("Position", &sel->position.x, 0.1f))
		{
			ApplyTransform(*sel);
			sel->dirty = true;

		}

		//回転角度を編集可能にする
		if (ImGui::DragFloat3("Rotation", &sel->angle.x, 1.0f))
		{
			ApplyTransform(*sel);
		}

		//拡大・縮小を編集可能にする
		if (ImGui::DragFloat3("Scale", &sel->scale.x, 0.05f))
		{
			//最小値にクランプ
			sel->scale.x = (std::max)(sel->scale.x, 0.001f);
			sel->scale.y = (std::max)(sel->scale.y, 0.001f);
			sel->scale.z = (std::max)(sel->scale.z, 0.001f);
			ApplyTransform(*sel);
		}

		ImGui::ColorEdit4("Color", &sel->color.x);


		ImGui::Separator();//区切り線

		//Deleteボタン
		if (ImGui::Button("Delete"))
		{
			delete_index = select_index;
		}

		ImGui::SameLine();

		//Resetボタン(初期化する)
		if (ImGui::Button("Reset Transform"))
		{
			sel->position = XMFLOAT3(0.0f, 0.0f, 0.0f);
			sel->angle = XMFLOAT3(0.0f, 0.0f, 0.0f);
			sel->scale = XMFLOAT3(1.0f, 1.0f, 1.0f);
			ApplyTransform(*sel);
		}

		//デバッグ用にワールド行列を折りたたみヘッダで表示
		ImGui::Separator();
		if (select_index >= 0 && select_index < (int)objects.size())
		{
			if (ImGui::CollapsingHeader("World matrix(debug)"))
			{
				for (int r = 0; r < 4; r++)
				{
					ImGui::Text("%.3f %.3f %.3f %.3f",
						sel->transform.m[r][0], sel->transform.m[r][1], sel->transform.m[r][2], sel->transform.m[r][3]);
				}
			}
		}
		else
		{
			ImGui::TextDisabled("No object selected");
		}
	}

	ImGui::EndChild();

	//削除処理
	if (delete_index >= 0)
	{
		if (delete_index < static_cast<int>(objects.size()))
		{
			objects.erase(objects.begin() + delete_index);
			if (!objects.empty())
			{
				select_index = (std::min)(delete_index, static_cast<int>(objects.size() - 1));
			}
			else
			{
				select_index = -1;
			}
		}
		delete_index = -1;
	}
}

void editor::Draw2DEditor(
	std::vector<std::unique_ptr<SpriteObject>>& sprites)
{
	ImGui::Text("Add New 2DSprite");

	std::vector<const char*> names;
	//names.reserve(sprites_batchs.size());

	//for (auto& m : sprites_batchs) names.push_back(m->name.c_str());

	if (!names.empty())
		ImGui::Combo("Sprite", &sprite_index, names.data(), (int)names.size());
	else
		ImGui::TextDisabled("No sprites loaded.");


	if (ImGui::Button("Add"))
	{
		std::string baseName = "Empty";
		int mesh_index = -1;

		mesh_index = sprite_index;
		//baseName = sprites_batchs[mesh_index]->name;

		AddSprite(sprites, baseName, mesh_index);
	}

	ImGui::BeginChild("", ImVec2(250, 0), true);
	ImGui::Text("2DSprite_List");
	ImGui::Separator();
	for (size_t i = 0; i < sprites.size(); i++)
	{
		SpriteObject* sp = sprites[i].get();
		if (ImGui::Selectable(sp->name.c_str(), select_index2D == (int)i))
		{
			select_index2D = (int)i;
		}
	}
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("SpriteInspector", ImVec2(0, 0), true);
	ImGui::Text("Sprite Inspector");
	ImGui::Separator();

	if (select_index2D >= 0 && select_index2D < (int)sprites.size())
	{
		SpriteObject* sp = sprites[select_index2D].get();
		char buf[128];
		strncpy_s(buf, sizeof(buf), sp->name.c_str(), _TRUNCATE);
		if (ImGui::InputText("Name", buf, sizeof(buf)))
			sp->name = buf;

		// sprite_batch 選択コンボ
		std::vector<const char*> spriteNames;
		//for (auto& sb : sprites_batchs)
			//spriteNames.push_back(sb->name.c_str());

		if (!spriteNames.empty())
			ImGui::Combo(
				"Texture",
				&sp->sprite_index,
				spriteNames.data(),
				(int)spriteNames.size());

		if (ImGui::DragFloat2("Position", &sp->position.x, 1.0f))
		{
			// update sprite transform if needed
		}
		ImGui::DragFloat2("Size", &sp->size.x, 1.0f, 1, 2048);
		ImGui::DragFloat("Rotation", &sp->rotation, 1.0f);
		ImGui::ColorEdit4("Color", &sp->color.x);

		ImGui::DragFloat2("UV Min", &sp->uv_min.x, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat2("UV Max", &sp->uv_max.x, 0.01f, 0.0f, 1.0f);

		if (ImGui::Button("Delete Sprite"))
		{
			delete_index2D = select_index2D;
		}
	}
	ImGui::EndChild();

	if (delete_index2D >= 0)
	{
		if (delete_index2D < (int)sprites.size())
			sprites.erase(sprites.begin() + delete_index2D);
		delete_index2D = -1;
		select_index2D = -1;
	}
}

std::string editor::MakeUniqueName(const std::vector<std::unique_ptr<GameObject>>& objects, const std::string& base)
{
	int i = 1;
	std::string cand = base;
	auto exists = [&](const std::string& nm) {
		for (auto& o : objects)if (o->name == nm)return true;
		return false;
		};
	while (exists(cand)) { ++i; cand = base + std::to_string(i); }
	return cand;
}

std::string editor::MakeUniqueSpriteName(const std::vector<std::unique_ptr<SpriteObject>>& sprites, const std::string& base)
{
	int i = 1;
	std::string cand = base;
	auto exists = [&](const std::string& nm) {
		for (auto& s : sprites) if (s->name == nm) return true;
		return false;
		};
	while (exists(cand)) { ++i; cand = base + std::to_string(i); }
	return cand;
}

SpriteObject::SpriteObject()
	: name("NewSprite"),
	texture(""),
	position{ 0, 0 },
	size{ 100, 100 },
	rotation(0.0f),
	color{ 1,1,1,1 },
	sprite_index(0),
	uv_min{ 0,0 },
	uv_max{ 1.0f,1.0f } {}
