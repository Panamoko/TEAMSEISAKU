#include "GridMap.h"

GridMap::GridMap(int map_width, int map_height, float cell)
    :width(map_width)
{

}

//ゲーム内のオブジェクトから障害物マップを作る
void GridMap::Build(const std::vector<std::shared_ptr<GameObject>>& objects)
{
    grid.assign(width * height, 0); // 全セルを通れる状態に初期化

    for (auto& obj : objects)
    {
        //非アクティブまたはコライダーが無い オブジェクトは無視
        if (!obj->IsActive() || !obj->collider) continue;

        //ブジェクトのワールド空間の Oriented Bounding Box を取得
        OBB obb = obj->GetOBB();

        //OBBの8頂点を計算
        using namespace DirectX;
        XMVECTOR axes[3] = {
            XMLoadFloat3(&obb.axis[0]),
            XMLoadFloat3(&obb.axis[1]),
            XMLoadFloat3(&obb.axis[2])
        };
        XMVECTOR center = XMLoadFloat3(&obb.center);

        std::vector<XMFLOAT3> obbVertices;
        obbVertices.reserve(8);
        for (int dx = -1; dx <= 1; dx += 2)
        for (int dy = -1; dy <= 1; dy += 2)
        for (int dz = -1; dz <= 1; dz += 2)
        {
            XMVECTOR vertex = center + dx * obb.half.x * axes[0]
                + dy * obb.half.y * axes[1]
                + dz * obb.half.z * axes[2];
            XMFLOAT3 p; XMStoreFloat3(&p, vertex);
            obbVertices.push_back(p);
        }

        //OBBのワールドAABBを求める
        DirectX::XMFLOAT3 min = obbVertices[0];
        DirectX::XMFLOAT3 max = obbVertices[0];
        for (auto& vertex : obbVertices)
        {
            min.x = (std::min)(min.x, vertex.x);
            min.y = (std::min)(min.y, vertex.y);
            min.z = (std::min)(min.z, vertex.z);
                                      
            max.x = (std::max)(max.x, vertex.x);
            max.y = (std::max)(max.y, vertex.y);
            max.z = (std::max)(max.z, vertex.z);
        }

        //グリッド範囲に変換
        int gridXStart = (std::max)(0, int(min.x / cell_size));
        int gridZStart = (std::max)(0, int(min.z / cell_size));
        int gridXEnd = (std::min)(width - 1, int(max.x / cell_size));
        int gridZEnd = (std::min)(height - 1, int(max.z / cell_size));

        //対応するセルを障害物にする
        for (int x = gridXStart; x <= gridXEnd; x++)
        {
            for (int z = gridZStart; z <= gridZEnd; z++)
            {
                grid[z * width + x] = 1;
            }
        }
    }
}

bool GridMap::IsBloked(int x, int z) const
{
    return false;
}
