#pragma once

#include <memory>
#include <vector>
#include <wrl.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include "Model.h"
#include "Shader.h"

enum class ShaderId
{
	Basic,
	Lambert,

	EnumCount
};

class ModelRenderer
{
public:
	ModelRenderer(ID3D11Device* device);
	~ModelRenderer() {}

	//フレーム開始処理（シーン定数バッファの更新など、フレームで1回やればいい処理）
	void BeginFrame(const RenderContext& rc);

	// 描画実行
	void Render(const RenderContext& rc,
		const DirectX::XMFLOAT4X4& worldTransform,
		const Model* model,
		ShaderId shaderId,
		const DirectX::XMFLOAT4& color = { 1,1,1,1 });
private:

	struct CbScene
	{
		DirectX::XMFLOAT4X4		viewProjection;
		DirectX::XMFLOAT4		lightDirection;
	};

	struct CbSkeleton
	{
		DirectX::XMFLOAT4X4		boneTransforms[256];
	};

	std::unique_ptr<Shader>					shaders[static_cast<int>(ShaderId::EnumCount)];

	Microsoft::WRL::ComPtr<ID3D11Buffer>	sceneConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>	skeletonConstantBuffer;
};
