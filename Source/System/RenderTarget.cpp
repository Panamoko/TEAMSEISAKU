#include "RenderTarget.h"
#include "Graphics.h"
#include "Misc.h"

RenderTarget::RenderTarget(UINT width, UINT height) : width(width), height(height)
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	// 1. テクスチャ作成
	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	HRESULT hr = device->CreateTexture2D(&textureDesc, nullptr, renderTargetTexture.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

	// 2. レンダーターゲットビュー作成
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = textureDesc.Format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	hr = device->CreateRenderTargetView(renderTargetTexture.Get(), &rtvDesc, renderTargetView.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

	// 3. シェーダーリソースビュー作成（スプライトとして描画するため）
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	hr = device->CreateShaderResourceView(renderTargetTexture.Get(), &srvDesc, shaderResourceView.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

	// 4. 深度バッファ作成（3D描画に必要）
	D3D11_TEXTURE2D_DESC depthDesc = textureDesc;
	depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	hr = device->CreateTexture2D(&depthDesc, nullptr, depthStencilTexture.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

	hr = device->CreateDepthStencilView(depthStencilTexture.Get(), nullptr, depthStencilView.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

	// ビューポート設定
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
}

void RenderTarget::Activate(ID3D11DeviceContext* dc)
{
	// 現在の状態を保存
	dc->OMGetRenderTargets(1, cachedRTV.GetAddressOf(), cachedDSV.GetAddressOf());
	UINT numViewports = 1;
	dc->RSGetViewports(&numViewports, &cachedViewport);

	// レンダーターゲット切り替え
	// 第2引数: レンダーターゲットの配列(ポインタのポインタ) -> GetAddressOf()
	// 第3引数: 深度ステンシルビュー(単体のポインタ) -> Get()
	dc->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), depthStencilView.Get());

	dc->RSSetViewports(1, &viewport);
}

void RenderTarget::Deactivate(ID3D11DeviceContext* dc)
{

	// 【修正前】rtv は未定義であり、depthStencilView を指定するのは間違い
	// dc->OMSetRenderTargets(1, &rtv, depthStencilView.Get());

	// 【修正後】保存しておいた cachedRTV と cachedDSV をセットする
	dc->OMSetRenderTargets(1, cachedRTV.GetAddressOf(), cachedDSV.Get());

	dc->RSSetViewports(1, &cachedViewport);

	cachedRTV.Reset();
	cachedDSV.Reset();
}

void RenderTarget::Clear(ID3D11DeviceContext* dc, float r, float g, float b, float a)
{
	float color[4] = { r, g, b, a };
	dc->ClearRenderTargetView(renderTargetView.Get(), color);
	dc->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}