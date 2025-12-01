#pragma once
#include <d3d11.h>
#include <wrl.h>

class RenderTarget
{
public:
	RenderTarget(UINT width, UINT height);
	~RenderTarget() = default;

	// 描画先をこのレンダーターゲットに切り替える
	void Activate(ID3D11DeviceContext* dc);

	// 描画先を元のバックバッファ（画面）に戻す
	void Deactivate(ID3D11DeviceContext* dc);

	// 作成した画像（シェーダーリソースビュー）を取得
	ID3D11ShaderResourceView* GetSRV() const { return shaderResourceView.Get(); }

	// クリア処理
	void Clear(ID3D11DeviceContext* dc, float r, float g, float b, float a);

private:
	UINT width;
	UINT height;

	D3D11_VIEWPORT viewport;
	Microsoft::WRL::ComPtr<ID3D11Texture2D>          renderTargetTexture;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   renderTargetView;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView;
	Microsoft::WRL::ComPtr<ID3D11Texture2D>          depthStencilTexture;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>   depthStencilView;

	// バックアップ用（元に戻すため）
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   cachedRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>   cachedDSV;
	D3D11_VIEWPORT                                   cachedViewport;
};