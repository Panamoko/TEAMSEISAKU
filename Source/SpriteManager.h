#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include "System/Sprite.h"
#include "SpriteResource.h"

class SpriteManager
{
private:
	SpriteManager() = default;
	~SpriteManager() = default;

	//パスの正規化ヘルパー
	static std::string GetNormaledName(const std::string& path);

public:

	static SpriteManager& Instance()
	{
		static SpriteManager instance;
		return instance;
	}

	//リソースをキャッシュから取得またはロード
	std::shared_ptr<SpriteResource>GetResource(const std::string& path);

	//新しい Sprite インスタンスを作成して管理対象にする
	Sprite* CreateNewInstance(const std::string& path);

	//すでにロード済みの Sprite インスタンスのリストを取得
	const std::vector<std::unique_ptr<Sprite>>& GetSprite()const { return sprites_data; }

	const std::vector<std::string> GetResourceNames() const;

	//モデルロード
	Sprite* Load(const std::string& path);

	//SpriteManagerで管理せず、呼び出し元に所有権を渡す
	std::unique_ptr<Sprite> CreateUniqueInstance(const std::string& path);

private:
	// ロードされた SpriteResource のキャッシュ
	std::unordered_map<std::string, std::shared_ptr<SpriteResource>> resource_map_;

	// 生成され、管理対象になっている Sprite インスタンスのリスト
	std::vector<std::unique_ptr<Sprite>> sprites_data; 

	//Load で生成された Sprite オブジェクトをキャッシュするためのマップ
	std::unordered_map<std::string, Sprite*> sprite_map_;

};