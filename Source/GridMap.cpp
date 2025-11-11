#include "GridMap.h"

//ゲーム内のオブジェクトから障害物マップを作る
void GridMap::Build(const std::vector<std::shared_ptr<GameObject>>& objects)
{
    grid.assign(width * height, 0); // 全セルを通れる状態に初期化

    for (auto& obj : objects)
    {
        if (!obj->IsActive() || !obj->collider) continue;

        OBB obb = obj->GetOBB();

        //OBBの8頂点を計算
        using namespace DirectX;
        XMVECTOR axes[3] = {
            XMLoadFloat3(&obb.axis[0]),
            XMLoadFloat3(&obb.axis[1]),
            XMLoadFloat3(&obb.axis[2])
        };
        XMVECTOR center = XMLoadFloat3(&obb.center);

        std::vector<XMFLOAT3> points;
        points.reserve(8);
        for (int dx = -1; dx <= 1; dx += 2)
        for (int dy = -1; dy <= 1; dy += 2)
        for (int dz = -1; dz <= 1; dz += 2)
        {
            XMVECTOR pt = center + dx * obb.half.x * axes[0]
                + dy * obb.half.y * axes[1]
                + dz * obb.half.z * axes[2];
            XMFLOAT3 p; XMStoreFloat3(&p, pt);
            points.push_back(p);
        }

        //OBBのワールドAABBを求める
        DirectX::XMFLOAT3 min = points[0];
        DirectX::XMFLOAT3 max = points[0];
        for (auto& p : points)
        {
            min.x = (std::min)(min.x, p.x);
            min.y = (std::min)(min.y, p.y);
            min.z = (std::min)(min.z, p.z);

            max.x = (std::max)(max.x, p.x);
            max.y = (std::max)(max.y, p.y);
            max.z = (std::max)(max.z, p.z);
        }

        //グリッド範囲に変換
        int x0 = (std::max)(0, int(min.x / cell_size));
        int z0 = (std::max)(0, int(min.z / cell_size));
        int x1 = (std::min)(width - 1, int(max.x / cell_size));
        int z1 = (std::min)(height - 1, int(max.z / cell_size));

        //対応するセルを障害物にする
        for (int x = x0; x <= x1; x++)
        {
            for (int z = z0; z <= z1; z++)
            {
                grid[z * width + x] = 1;
            }
        }

    }
}