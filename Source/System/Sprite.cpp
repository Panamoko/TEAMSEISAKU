#include <fstream>
#include "System/Graphics.h"
#include "Sprite.h"
#include "Misc.h"
#include "GpuResourceUtils.h"

Microsoft::WRL::ComPtr<ID3D11VertexShader> Sprite::commonVertexShader;
Microsoft::WRL::ComPtr<ID3D11PixelShader>  Sprite::commonPixelShader;
Microsoft::WRL::ComPtr<ID3D11InputLayout>  Sprite::commonInputLayout;

// コンストラクタ
Sprite::Sprite()
	: Sprite(nullptr)
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	HRESULT hr = S_OK;

	// 頂点バッファの生成
	{
		D3D11_BUFFER_DESC buffer_desc = {};
		buffer_desc.ByteWidth = sizeof(Vertex) * 4;
		buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
		buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		hr = device->CreateBuffer(&buffer_desc, nullptr, vertexBuffer.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}

	// 頂点シェーダー
	{
		D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR",0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		hr = GpuResourceUtils::LoadVertexShader(
			device,
			"Data/Shader/SpriteVS.cso",
			inputElementDesc,
			ARRAYSIZE(inputElementDesc),
			inputLayout.GetAddressOf(),
			vertexShader.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}

	// ピクセルシェーダー
	{
		hr = GpuResourceUtils::LoadPixelShader(
			device,
			"Data/Shader/SpritePS.cso",
			pixelShader.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}
}

// コンストラクタ
Sprite::Sprite(const char* filename)
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	HRESULT hr = S_OK;

	// 頂点バッファの生成 (これは個別に必要なのでそのまま)
	{
		D3D11_BUFFER_DESC buffer_desc = {};
		buffer_desc.ByteWidth = sizeof(Vertex) * 4;
		buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
		buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = 0;
		hr = device->CreateBuffer(&buffer_desc, nullptr, vertexBuffer.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}

	// ★変更: 頂点シェーダーと入力レイアウトの共有化
	// まだロードされていない場合のみロード処理を行う
	if (!commonVertexShader)
	{
		D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		// common... のアドレスにロードする
		hr = GpuResourceUtils::LoadVertexShader(
			device,
			"Data/Shader/SpriteVS.cso",
			inputElementDesc,
			ARRAYSIZE(inputElementDesc),
			commonInputLayout.GetAddressOf(),
			commonVertexShader.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}

	// ★変更: ピクセルシェーダーの共有化
	if (!commonPixelShader)
	{
		// common... のアドレスにロードする
		hr = GpuResourceUtils::LoadPixelShader(
			device,
			"Data/Shader/SpritePS.cso",
			commonPixelShader.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}

	// ★重要: 共有リソースをこのインスタンスのメンバ変数にセットする
	// ComPtrなので、代入するだけで参照カウントが増え、安全に共有される
	vertexShader = commonVertexShader;
	pixelShader = commonPixelShader;
	inputLayout = commonInputLayout;

	if (filename != nullptr)
	{
		D3D11_TEXTURE2D_DESC desc;
		hr = GpuResourceUtils::LoadTexture(device, filename, shaderResourceView.GetAddressOf(), &desc);
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		textureWidth = static_cast<float>(desc.Width);
		textureHeight = static_cast<float>(desc.Height);
	}
	else
	{
		D3D11_TEXTURE2D_DESC desc;
		hr = GpuResourceUtils::CreateDummyTexture(device, 0xFFFFFFFF, shaderResourceView.GetAddressOf(), &desc);
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		textureWidth = static_cast<float>(desc.Width);
		textureHeight = static_cast<float>(desc.Height);
	}
}

// Source/System/Sprite.cpp

// 描画実行
void Sprite::Render(const RenderContext& rc,
	float dx, float dy,					// 左上位置
	float dz,							// 奥行
	float dw, float dh,					// 幅、高さ
	float sx, float sy,					// 画像切り抜き位置
	float sw, float sh,					// 画像切り抜きサイズ
	float angle,						// 角度
	float r, float g, float b, float a	// 色
) const
{
	// ★変更: 描画に使うテクスチャとサイズを決定する
	ID3D11ShaderResourceView* srv = nullptr;
	float resource_width = 0.0f;
	float resource_height = 0.0f;

	if (sprite_resource)
	{
		// 共有リソースがある場合
		srv = sprite_resource->shader_resource_view.Get();
		resource_width = sprite_resource->texture_width;
		resource_height = sprite_resource->texture_height;
	}
	else if (shaderResourceView)
	{
		// 共有リソースがない場合、自分自身のローカルリソース（白画像など）を使う
		srv = shaderResourceView.Get();
		resource_width = textureWidth;
		resource_height = textureHeight;
	}
	else
	{
		// どちらもなければ描画しない
		return;
	}

	ID3D11DeviceContext* dc = rc.deviceContext;

	// 頂点座標
	DirectX::XMFLOAT2 positions[] = {
		DirectX::XMFLOAT2(dx,      dy),			// 左上
		DirectX::XMFLOAT2(dx + dw, dy),			// 右上
		DirectX::XMFLOAT2(dx,      dy + dh),	// 左下
		DirectX::XMFLOAT2(dx + dw, dy + dh),	// 右下
	};

	// テクスチャ座標
	DirectX::XMFLOAT2 texcoords[] = {
		DirectX::XMFLOAT2(sx,      sy),			// 左上
		DirectX::XMFLOAT2(sx + sw, sy),			// 右上
		DirectX::XMFLOAT2(sx,      sy + sh),	// 左下
		DirectX::XMFLOAT2(sx + sw, sy + sh),	// 右下
	};

	// スプライトの中心で回転させるために４頂点の中心位置が
	// 原点(0, 0)になるように一旦頂点を移動させる。
	float mx = dx + dw * 0.5f;
	float my = dy + dh * 0.5f;
	for (auto& p : positions)
	{
		p.x -= mx;
		p.y -= my;
	}

	// 頂点を回転させる
	float theta = DirectX::XMConvertToRadians(angle);
	float c = cosf(theta);
	float s = sinf(theta);
	for (auto& p : positions)
	{
		DirectX::XMFLOAT2 r = p;
		p.x = c * r.x + -s * r.y;
		p.y = s * r.x + c * r.y;
	}

	// 回転のために移動させた頂点を元の位置に戻す
	for (auto& p : positions)
	{
		p.x += mx;
		p.y += my;
	}

	// 現在設定されているビューポートからスクリーンサイズを取得する。
	D3D11_VIEWPORT viewport;
	UINT numViewports = 1;
	dc->RSGetViewports(&numViewports, &viewport);
	float screenWidth = viewport.Width;
	float screenHeight = viewport.Height;

	// スクリーン座標系からNDC座標系へ変換する。
	for (DirectX::XMFLOAT2& p : positions)
	{
		p.x = 2.0f * p.x / screenWidth - 1.0f;
		p.y = 1.0f - 2.0f * p.y / screenHeight;
	}

	// 頂点バッファの内容の編集を開始する。
	D3D11_MAPPED_SUBRESOURCE mappedSubresource;
	HRESULT hr = dc->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

	// 頂点バッファの内容を編集
	Vertex* v = static_cast<Vertex*>(mappedSubresource.pData);
	for (int i = 0; i < 4; ++i)
	{
		v[i].position.x = positions[i].x;
		v[i].position.y = positions[i].y;
		v[i].position.z = dz;

		v[i].color.x = r;
		v[i].color.y = g;
		v[i].color.z = b;
		v[i].color.w = a;

		// ★変更: resource_width / resource_height を使用
		v[i].texcoord.x = texcoords[i].x / resource_width;
		v[i].texcoord.y = texcoords[i].y / resource_height;
	}

	// 頂点バッファの内容の編集を終了する。
	dc->Unmap(vertexBuffer.Get(), 0);

	// GPUに描画するためのデータを渡す
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	dc->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
	dc->IASetInputLayout(inputLayout.Get());
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	dc->VSSetShader(vertexShader.Get(), nullptr, 0);
	dc->PSSetShader(pixelShader.Get(), nullptr, 0);

	// ★変更: srv をセット
	dc->PSSetShaderResources(0, 1, &srv);

	// レンダーステート設定
	dc->OMSetDepthStencilState(rc.renderState->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
	dc->RSSetState(rc.renderState->GetRasterizerState(RasterizerState::SolidCullNone));
	dc->OMSetBlendState(rc.renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);

	// 描画
	dc->Draw(4, 0);
}

// 描画実行（テクスチャ切り抜き指定なし）
void Sprite::Render(const RenderContext& rc,
	float dx, float dy,					// 左上位置
	float dz,							// 奥行
	float dw, float dh,					// 幅、高さ
	float angle,						// 角度
	float r, float g, float b, float a	// 色
) const
{
	float resource_width = sprite_resource ? sprite_resource->texture_width : 0.0f;
	float resource_height = sprite_resource ? sprite_resource->texture_height : 0.0f;

	Render(rc, dx, dy, dz, dw, dh, 0, 0, resource_width, resource_height, angle, r, g, b, a);
}

// 外部テクスチャ(SRV)を指定して描画
void Sprite::RenderTexture(const RenderContext& rc,
	ID3D11ShaderResourceView* texture,
	float dx, float dy,
	float dw, float dh,
	float angle,
	float r, float g, float b, float a
) const
{
	if (!texture) return;

	// テクスチャサイズを取得（UV計算のため）
	Microsoft::WRL::ComPtr<ID3D11Resource> resource;
	texture->GetResource(resource.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11Texture2D> tex2D;
	resource.As(&tex2D);
	D3D11_TEXTURE2D_DESC desc;
	tex2D->GetDesc(&desc);
	float w = static_cast<float>(desc.Width);
	float h = static_cast<float>(desc.Height);

	// 既存のRender関数（詳細指定版）を呼び出す処理をコピーまたは共通化したいところですが、
	// ここでは実装を埋め込みます（Renderのロジックと同じです）

	// ★Sprite::Renderの内部ロジックを流用（SRVだけ差し替え）
	ID3D11DeviceContext* dc = rc.deviceContext;

	// 頂点座標
	DirectX::XMFLOAT2 positions[] = {
		DirectX::XMFLOAT2(dx,      dy),
		DirectX::XMFLOAT2(dx + dw, dy),
		DirectX::XMFLOAT2(dx,      dy + dh),
		DirectX::XMFLOAT2(dx + dw, dy + dh),
	};

	// テクスチャ座標 (0.0~1.0 全体)
	DirectX::XMFLOAT2 texcoords[] = {
		DirectX::XMFLOAT2(0.0f, 0.0f),
		DirectX::XMFLOAT2(1.0f, 0.0f),
		DirectX::XMFLOAT2(0.0f, 1.0f),
		DirectX::XMFLOAT2(1.0f, 1.0f),
	};

	// --- 回転処理 (既存Renderと同じ) ---
	float mx = dx + dw * 0.5f;
	float my = dy + dh * 0.5f;
	for (auto& p : positions) { p.x -= mx; p.y -= my; }
	float theta = DirectX::XMConvertToRadians(angle);
	float c = cosf(theta);
	float s = sinf(theta);
	for (auto& p : positions) {
		DirectX::XMFLOAT2 rot = p;
		p.x = c * rot.x + -s * rot.y;
		p.y = s * rot.x + c * rot.y;
	}
	for (auto& p : positions) { p.x += mx; p.y += my; }

	// --- NDC変換 (既存Renderと同じ) ---
	D3D11_VIEWPORT viewport;
	UINT numViewports = 1;
	dc->RSGetViewports(&numViewports, &viewport);
	float screenWidth = viewport.Width;
	float screenHeight = viewport.Height;
	for (DirectX::XMFLOAT2& p : positions) {
		p.x = 2.0f * p.x / screenWidth - 1.0f;
		p.y = 1.0f - 2.0f * p.y / screenHeight;
	}

	// --- 定数バッファ更新 ---
	D3D11_MAPPED_SUBRESOURCE mapped;
	dc->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	Vertex* v = static_cast<Vertex*>(mapped.pData);
	for (int i = 0; i < 4; ++i) {
		v[i].position.x = positions[i].x;
		v[i].position.y = positions[i].y;
		v[i].position.z = 0.0f;
		v[i].color = { r, g, b, a };
		v[i].texcoord = texcoords[i];
	}
	dc->Unmap(vertexBuffer.Get(), 0);

	// --- 描画設定 ---
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	dc->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
	dc->IASetInputLayout(inputLayout.Get());
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	dc->VSSetShader(vertexShader.Get(), nullptr, 0);
	dc->PSSetShader(pixelShader.Get(), nullptr, 0);

	// ★ここで引数の texture をセット
	dc->PSSetShaderResources(0, 1, &texture);

	dc->OMSetDepthStencilState(rc.renderState->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
	dc->RSSetState(rc.renderState->GetRasterizerState(RasterizerState::SolidCullNone));
	dc->OMSetBlendState(rc.renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);

	dc->Draw(4, 0);
}
