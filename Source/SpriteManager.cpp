#include "SpriteManager.h"
#include "System/Graphics.h" 
#include <filesystem>
#include <iostream>

//パスの正規化ヘルパー
std::string SpriteManager::GetNormaledName(const std::string& path)
{
    std::cerr << "Current Working Directory (CWD): "
        << std::filesystem::current_path().string()
        << std::endl;

    std::filesystem::path p(path);
    try {
        return std::filesystem::canonical(p).string();
    }
    catch (const std::filesystem::filesystem_error& e) {
        // デバッグ情報としてエラーの内容をログに出力）
        std::string debug_output = "[Filesystem Error] Normalization failed for path: " + path
            + ". Error: " + e.what() + "\n";        //           << " (Path: " << path << ")" << std::endl;

        OutputDebugStringA(debug_output.c_str());

        return "";
    }
    catch (...) {
        return "";
    }
}

//リソースの取得・キャッシュ
std::shared_ptr<SpriteResource> SpriteManager::GetResource(const std::string& path)
{
    try{

    std::string name = GetNormaledName(path);
    if (name.empty()) return nullptr;

    // キャッシュにあれば返す
    if (auto it = resource_map_.find(name); it != resource_map_.end())
    {
        return it->second;
    }

    // なければロードして登録
    auto resource = std::make_shared<SpriteResource>();

    // Resource::Load を呼び出す
    if (FAILED(resource->Load(Graphics::Instance().GetDevice(), path.c_str())))
    {
        return nullptr;
    }

    resource_map_[name] = resource;

    return resource;

    }
    catch (const std::exception& e) {
        // ロード中に発生した標準例外（filesystem_errorなど）をキャッチ
        // 開発者にエラー内容を通知する処理（ログ出力など）を追加すべきです。
        std::cerr << "Resource load error: " << e.what() << std::endl;

        return nullptr;
    }
    catch (...) {
        // その他の予期せぬ例外をキャッチ
        return nullptr;
    }

}

//共有リソースを使って新しい Sprite を作成
Sprite* SpriteManager::CreateNewInstance(const std::string& path)
{
    //リソースの取得
    auto resource = GetResource(path);
    if (!resource) return nullptr;

    //テクスチャロードを行わない Sprite::Sprite() を使用
    auto sprite_instance = std::make_unique<Sprite>();

    //リソースを Sprite インスタンスに設定
    sprite_instance->SetResource(resource);

    Sprite* ptr = sprite_instance.get();

    sprites_data.push_back(std::move(sprite_instance));

    return ptr;
}

const std::vector<std::string> SpriteManager::GetResourceNames() const
{
    std::vector<std::string> names;

    // resource_map_ のキー (正規化された絶対パス) を names に格納
    for (const auto& pair : resource_map_)
    {
        // 【修正】絶対パス (正規化されたキー) をそのまま names に追加します。
        // pair.first は GetNormaledName() が返した絶対パス文字列です。
        names.push_back(pair.first);
    }

    return names;

    //std::vector<std::string> names;

    //// resource_map_ のキーを names に格納
    //for (const auto& pair : resource_map_)
    //{
    //    //std::filesystem::path を使ってファイル名のみを抽出します。
    //    std::filesystem::path full_path(pair.first);

    //    // filename() はファイル名（拡張子含む）を返します。
    //    // string() は std::string に変換するために必要です。
    //    names.push_back(full_path.filename().string());
    //}

    //return names;
}

Sprite* SpriteManager::Load(const std::string& path)
{
    std::string name = GetNormaledName(path);
    if (name.empty()) return nullptr;

    // すでにロード済みなら再利用
    if (auto it = sprite_map_.find(name); it != sprite_map_.end())
        return it->second;

    // リソースを取得（ロードまたはキャッシュから）
    auto resource = GetResource(path);
    if (!resource) return nullptr;

    // 新規作成
    auto sprite = std::make_unique<Sprite>();
    sprite->SetResource(resource); // リソースを設定

    // パスを保存したい場合はここで実行: sprite->path = path;

    Sprite* ptr = sprite.get();

    // キャッシュに登録
    sprite_map_[name] = ptr;

    // models_ に追加して管理対象にする
    sprites_data.push_back(std::move(sprite));

    return ptr;
}

std::unique_ptr<Sprite> SpriteManager::CreateUniqueInstance(const std::string& path)
{
    // リソースを取得（キャッシュにあればそれを使う）
    auto resource = GetResource(path);
    if (!resource) return nullptr;

    // リソースを渡して新しい Sprite を作成
    auto sprite = std::make_unique<Sprite>();
    sprite->SetResource(resource); // リソースを設定

    return sprite;
}
