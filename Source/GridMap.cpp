#include "GridMap.h"
#include <cmath>
#include <algorithm> // clamp用

GridMap::GridMap()
{
    width = 0;
    height = 0;
    cell_size = 0;
}

void GridMap::Initialize(int map_width, int map_height, float cell)
{
    width = map_width;
    height = map_height;
    cell_size = cell;

    //各セルを空ベクターで初期化
    grid.resize(width * height);
}

//ゲーム内のオブジェクトから障害物マップを作る
void GridMap::Build(const std::vector<std::shared_ptr<GameObject>>& objects)
{
    using namespace DirectX;

    // 1. 全セルをクリア
    for (auto& cell : grid)
        cell.clear();

    int cx = width / 2;
    int cz = height / 2;

    auto WorldToCellLocal = [&](float x, float z) {
        int gx = cx + static_cast<int>(std::floor(x / cell_size));
        int gz = cz + static_cast<int>(std::floor(z / cell_size));
        return std::pair<int, int>{gx, gz};
        };

    for (auto& obj : objects)
    {
        if (!obj->IsActive() || !obj->collider)
            continue;

        if (obj->type != GameObject::Type::Gimmic)
            continue;

        if (obj->class_name == "Core")
            continue;

        Collider* col = obj->collider.get();

        switch (col->type)
        {
        case ColliderType::OBB:
        {
            OBB* obb = static_cast<OBB*>(col);

            // ★修正ポイント：判定用に一時的にOBBを太らせるコピーを作る
            OBB checkOBB = *obb;
            // セルサイズの半分くらい太らせることで、中心点がズレても引っかかるようにする
            float fatMargin = cell_size * 0.5f;
            checkOBB.half.x += fatMargin;
            checkOBB.half.z += fatMargin;

            // 以下の計算は checkOBB を使用する
            XMVECTOR center = XMLoadFloat3(&checkOBB.center);
            XMVECTOR axes[3] = {
                XMLoadFloat3(&checkOBB.axis[0]),
                XMLoadFloat3(&checkOBB.axis[1]),
                XMLoadFloat3(&checkOBB.axis[2])
            };
            XMFLOAT3 half = checkOBB.half;

            // ... (minPos, maxPos の計算はそのまま checkOBB の値を使用) ...
            XMFLOAT3 minPos = { FLT_MAX, FLT_MAX, FLT_MAX };
            XMFLOAT3 maxPos = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

            for (int dx = -1; dx <= 1; dx += 2)
                for (int dy = -1; dy <= 1; dy += 2)
                    for (int dz = -1; dz <= 1; dz += 2)
                    {
                        // checkOBBの情報を使って頂点を計算
                        XMVECTOR vertex = center
                            + dx * half.x * axes[0]
                            + dy * half.y * axes[1]
                            + dz * half.z * axes[2];

                        XMFLOAT3 pos;
                        XMStoreFloat3(&pos, vertex);

                        minPos.x = (std::min)(minPos.x, pos.x);
                        minPos.y = (std::min)(minPos.y, pos.y);
                        minPos.z = (std::min)(minPos.z, pos.z);
                        maxPos.x = (std::max)(maxPos.x, pos.x);
                        maxPos.y = (std::max)(maxPos.y, pos.y);
                        maxPos.z = (std::max)(maxPos.z, pos.z);
                    }

            auto [startX, startZ] = WorldToCellLocal(minPos.x, minPos.z);
            auto [endX, endZ] = WorldToCellLocal(maxPos.x, maxPos.z);

            startX = std::clamp(startX, 0, width - 1);
            startZ = std::clamp(startZ, 0, height - 1);
            endX = std::clamp(endX, 0, width - 1);
            endZ = std::clamp(endZ, 0, height - 1);

            for (int x = startX; x <= endX; ++x)
            {
                for (int z = startZ; z <= endZ; ++z)
                {
                    float wx = (x - cx + 0.5f) * cell_size;
                    float wz = (z - cz + 0.5f) * cell_size;
                    XMFLOAT3 point{ wx, checkOBB.center.y, wz };

                    // ★ここも太らせたOBBで判定
                    if (IsPointInsideOBB(&checkOBB, point))
                        grid[z * width + x].push_back(obj->GetID());
                }
            }
            break;
        }

        case ColliderType::Box:
        {
            BoxCollider* box = static_cast<BoxCollider*>(col);
            auto [startX, startZ] = WorldToCellLocal(box->box_min.x, box->box_min.z);
            auto [endX, endZ] = WorldToCellLocal(box->box_max.x, box->box_max.z);
            startX = std::clamp(startX, 0, width - 1);
            startZ = std::clamp(startZ, 0, height - 1);
            endX = std::clamp(endX, 0, width - 1);
            endZ = std::clamp(endZ, 0, height - 1);
            for (int x = startX; x <= endX; ++x)
                for (int z = startZ; z <= endZ; ++z)
                    grid[z * width + x].push_back(obj->GetID());
            break;
        }

        case ColliderType::Sphere:
        {
            SphereCollider* sphere = static_cast<SphereCollider*>(col);
            int r = static_cast<int>(std::ceil(sphere->radius / cell_size));
            auto [cxCell, czCell] = WorldToCellLocal(sphere->center.x, sphere->center.z);
            for (int x = cxCell - r; x <= cxCell + r; ++x)
                for (int z = czCell - r; z <= czCell + r; ++z)
                {
                    float dx = (x - cx) * cell_size - sphere->center.x;
                    float dz = (z - cz) * cell_size - sphere->center.z;
                    if (dx * dx + dz * dz <= sphere->radius * sphere->radius)
                        grid[z * width + x].push_back(obj->GetID());
                }
            break;
        }

        case ColliderType::Cylinder:
        {
            CylinderCollider* cyl = static_cast<CylinderCollider*>(col);
            int r = static_cast<int>(std::ceil(cyl->radius / cell_size));
            auto [cxCell, czCell] = WorldToCellLocal(cyl->center.x, cyl->center.z);
            for (int x = cxCell - r; x <= cxCell + r; ++x)
                for (int z = czCell - r; z <= czCell + r; ++z)
                {
                    float dx = (x - cx) * cell_size - cyl->center.x;
                    float dz = (z - cz) * cell_size - cyl->center.z;
                    if (dx * dx + dz * dz <= cyl->radius * cyl->radius)
                        grid[z * width + x].push_back(obj->GetID());
                }
            break;
        }

        default:
            break;
        }
    }
}

bool GridMap::IsBlocked(int x, int z) const
{
    //範囲外は強制的にブロック扱い
    if (x < 0 || x >= width || z < 0 || z >= height)return true;

    //セル内に何かデータが存在する場合は通行不可とする
    return !grid[z * width + x].empty();
}

void GridMap::RenderDebugPrimitive(const RenderContext& rc, ShapeRenderer* renderer)
{
    using namespace DirectX;

    if (!renderer) return;

    // グリッド1マスあたりのサイズ（例: 1m×1m）
    float halfCell = cell_size * 0.5f;

    // グリッド全体の半分（左右対称に描くための中心オフセット）
    float halfWidth = (width * cell_size) * 0.5f;
    float halfHeight = (height * cell_size) * 0.5f;

    // 原点を中心にグリッドを描画
    for (int z = 0; z < height; ++z)
    {
        for (int x = 0; x < width; ++x)
        {
            // 障害物があるセルだけ描画（負荷軽減のため）
            if (!grid[z * width + x].empty())
            {
                float worldX = (x * cell_size) - halfWidth + halfCell;
                float worldZ = (z * cell_size) - halfHeight + halfCell;

                DirectX::XMFLOAT3 cellPos = { worldX, 0.1f, worldZ }; // 少し浮かせる
                DirectX::XMFLOAT3 angle = { 0,0,0 };
                DirectX::XMFLOAT3 size = { cell_size * 0.9f, 0.1f, cell_size * 0.9f }; // 少し隙間を空ける

                // 赤色で表示
                renderer->RenderBox(rc, cellPos, angle, size, { 1.0f, 0.0f, 0.0f, 0.5f });
            }
        }
    }

    // グリッドの境界線などを追加したい場合：
    XMFLOAT4 borderColor = { 1.0f, 1.0f, 1.0f, 0.2f };
    for (int z = 0; z <= height; ++z)
    {
        float zPos = (z * cell_size) - halfHeight;
        XMFLOAT3 start = { -halfWidth, 0.02f, zPos };
        XMFLOAT3 end = { halfWidth, 0.02f, zPos };
        renderer->RenderBox(rc, start, end, { 0,0,0 }, borderColor);
    }
    for (int x = 0; x <= width; ++x)
    {
        float xPos = (x * cell_size) - halfWidth;
        XMFLOAT3 start = { xPos, 0.02f, -halfHeight };
        XMFLOAT3 end = { xPos, 0.02f,  halfHeight };
        renderer->RenderBox(rc, start, end, { 0,0,0 }, borderColor);
    }
}

//動的ブロック管理
void GridMap::SetBlocked(int x, int z, bool blocked)
{
    if (x < 0 || x >= width || z < 0 || z >= height)
        return;

    grid[z][x] = blocked ? 1 : 0;
}

//ワールド座標 (worldX, worldZ) をグリッド座標に変換
std::pair<int, int> GridMap::WorldToCell(float worldX, float worldZ) const
{
    // グリッド中央を原点に対応させる
    int cx = width / 2;
    int cz = height / 2;

    // ワールド座標をセル単位に変換
    int gx = cx + static_cast<int>(std::floor(worldX / cell_size));
    int gz = cz + static_cast<int>(std::floor(worldZ / cell_size));

    // グリッド範囲に収める
    gx = std::clamp(gx, 0, width - 1);
    gz = std::clamp(gz, 0, height - 1);

    return { gx, gz };
}

// ★追加: セル座標からワールド座標を取得
DirectX::XMFLOAT3 GridMap::GetWorldPosition(int cellX, int cellZ) const
{
    int cx = width / 2;
    int cz = height / 2;

    // セルの中心座標を計算
    float worldX = (cellX - cx + 0.5f) * cell_size;
    float worldZ = (cellZ - cz + 0.5f) * cell_size;

    return DirectX::XMFLOAT3(worldX, 0.0f, worldZ);
}

bool GridMap::IsOnMap(int cell_x, int cell_z) const
{
    //X座標が [0, width-1] の範囲内か
    if (cell_x < 0 || cell_x >= width)
    {
        return false;
    }

    //Z座標が [0, height-1] の範囲内か
    if (cell_z < 0 || cell_z >= height)
    {
        return false;
    }

    return true;
}
