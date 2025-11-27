#include "SpriteResource.h"
#include <System/GpuResourceUtils.h>
#include "System/Misc.h"

HRESULT SpriteResource::Load(ID3D11Device* device_ptr, const char* file_name)
{
	HRESULT hr = S_OK;

	//テクスチャファイル読み込み
	D3D11_TEXTURE2D_DESC desc;
	hr = GpuResourceUtils::LoadTexture(device_ptr, file_name, shader_resource_view.GetAddressOf(), &desc);
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

    if (SUCCEEDED(hr))
    {
        texture_width = static_cast<float>(desc.Width);
        texture_height = static_cast<float>(desc.Height);
        file_path = file_name;
    }

    return hr;
}
