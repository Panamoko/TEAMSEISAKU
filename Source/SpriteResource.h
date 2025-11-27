#pragma once

#include <string>
#include <d3d11.h>
#include <wrl/client.h>
#include <memory>

class SpriteResource
{
public:
	SpriteResource() = default;
	~SpriteResource() = default;

	//リソースをファイルからロード
	HRESULT Load(ID3D11Device* device_ptr, const char* file_name);

public:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view; // シェーダーリソースビュー
	float texture_width = 0.0f;                                            // テクスチャ幅
	float texture_height = 0.0f;                                           // テクスチャ高さ
	std::string file_path = "";                                            // 元のファイルパス
};

