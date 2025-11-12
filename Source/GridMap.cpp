#include "GridMap.h"

GridMap::GridMap(int map_width, int map_height, float cell)
    :width(map_width), height(map_height), cell_size(cell)
{
    //各セルを空ベクターで初期化
    grid.resize(width * height);
}

//ゲーム内のオブジェクトから障害物マップを作る
void GridMap::Build(const std::vector<std::shared_ptr<GameObject>>& objects)
{
    //全セルをクリア
    for (auto& cell : grid)cell.clear();

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

        DirectX::XMFLOAT3 minPos = { FLT_MAX, FLT_MAX, FLT_MAX };
        DirectX::XMFLOAT3 maxPos = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

        for (int dx = -1; dx <= 1; dx += 2)
        for (int dy = -1; dy <= 1; dy += 2)
        for (int dz = -1; dz <= 1; dz += 2)
        {
            XMVECTOR vertex = center 
                + dx * obb.half.x * axes[0]
                + dy * obb.half.y * axes[1]
                + dz * obb.half.z * axes[2];

            XMFLOAT3 pos;
            XMStoreFloat3(&pos, vertex);

            minPos.x = (std::min)(minPos.x, pos.x);
            minPos.y = (std::min)(minPos.y, pos.y);
            minPos.z = (std::min)(minPos.z, pos.z);
                      
            maxPos.x = (std::max)(maxPos.x, pos.x);
            maxPos.y = (std::max)(maxPos.y, pos.y);
            maxPos.z = (std::max)(maxPos.z, pos.z);
        }

        //グリッド範囲に変換
        int startX = static_cast<int>(0, minPos.x / cell_size);
        int startZ = static_cast<int>(0, minPos.z / cell_size);
        int endX = static_cast<int>(width - 1, int(maxPos.x / cell_size));
        int endZ = static_cast<int>(height - 1, int(maxPos.z / cell_size));

        //グリッド範囲に収める
        for (int x = startX; x <= endX; x++)
        {
            for (int z = startZ; z <= endZ; z++)
            {
                grid[z * width + x].push_back(obj->GetID());
            }
        }
    }
}

bool GridMap::IsBloked(int x, int z) const
{
    //範囲外は強制的にブロック扱い
    if (x < 0 || x >= width || z < 0 || z >= height)return true;

    return !grid[z * width + x].empty();//空でなければブロック
}
