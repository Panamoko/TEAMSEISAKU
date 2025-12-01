#pragma once

#include <wrl.h>
#include <d3d11.h>
#include <DirectXMath.h>

#include "RenderContext.h"
#include "SpriteResource.h"

// スプライト
class Sprite
{
public:
	Sprite();
	Sprite(const char* filename);
	void SetResource(std::shared_ptr<SpriteResource> resource)
	{
		this->sprite_resource = resource;
	}

	// 頂点データ
	struct Vertex
	{
		DirectX::XMFLOAT3	position;
		DirectX::XMFLOAT4	color;
		DirectX::XMFLOAT2	texcoord;
	};

	// 描画実行
	void Render(const RenderContext& rc,
		float dx, float dy,					// 左上位置
		float dz,							// 奥行
		float dw, float dh,					// 幅、高さ
		float sx, float sy,					// 画像切り抜き位置
		float sw, float sh,					// 画像切り抜きサイズ
		float angle,						// 角度
		float r, float g, float b, float a	// 色
	) const;

	// 描画実行（テクスチャ切り抜き指定なし）
	void Render(const RenderContext& rc,
		float dx, float dy,					// 左上位置
		float dz,							// 奥行
		float dw, float dh,					// 幅、高さ
		float angle,						// 角度
		float r, float g, float b, float a	// 色
	) const;
	// 外部テクスチャ(SRV)を指定して描画する関数
	void RenderTexture(const RenderContext& rc,
		ID3D11ShaderResourceView* texture,
		float dx, float dy,
		float dw, float dh,
		float angle = 0.0f,
		float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f
	) const;
private:
	Microsoft::WRL::ComPtr<ID3D11VertexShader>			vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>			pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>			inputLayout;

	Microsoft::WRL::ComPtr<ID3D11Buffer>				vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	shaderResourceView;

	float textureWidth = 0;
	float textureHeight = 0;

	std::shared_ptr<SpriteResource> sprite_resource;
};
