#include "AStar.h"
#include <set>
#include <unordered_set>

std::vector<std::pair<int, int>> AStar::FindPath(
    int start_cellX,
    int start_cellZ,
    int goal_cellX,
    int goal_cellZ,
    const GridMap& gridMap)
{
    //再初期化
    node_pool.Reset();
    node_map.clear();

    node_pool.Reserve(gridMap.GetWidth() * gridMap.GetHeight());

    //スタートノードを作成
    auto startNode = node_pool.GetNode();       //ノードを作成
    startNode->node_x = start_cellX;            //ノードのX座標をセット
    startNode->node_z = start_cellZ;            //ノードのZ座標をセット
    startNode->goal_cost = 0.0f;                //スタートからのコストは0
    startNode->h_cost = Heuristic(start_cellX, start_cellZ, goal_cellX, goal_cellZ);//推定コストを計算
    node_map[{start_cellX, start_cellZ}] = startNode;

    std::priority_queue<Node*, std::vector<Node*>, CompareNode> open_list;
    std::unordered_set<std::pair<int, int>, pair_hash> closed_set;
    open_list.push(startNode);

    //探索ループ開始
    while (!open_list.empty())
    {
        //fCost が最小のノードを取得
        Node* currentNode = open_list.top();
        open_list.pop();

        //既にクローズド済みならスキップ
        if (closed_set.find({ currentNode->node_x,currentNode->node_z }) != closed_set.end())
            continue;

        //ゴールに到達したか判定
        if (currentNode->node_x == goal_cellX && currentNode->node_z == goal_cellZ)
        {
            //経路復元
            std::vector<std::pair<int, int>> path;
            for (Node* trace = currentNode; trace; trace = trace->parent)
                path.emplace_back(trace->node_x, trace->node_z);
            std::reverse(path.begin(), path.end());//正しい順番で返す
            return path;//経路を返す
        }

        //現ノードをクローズリストに追加
        closed_set.insert({ currentNode->node_x,currentNode->node_z });

        // 隣接セルを取得
        auto neighbors = GetNeighbors(currentNode->node_x, currentNode->node_z, gridMap);

        for (auto&[neighborX,neighborZ]:GetNeighbors(currentNode->node_x,currentNode->node_z,gridMap))
        {
            if (closed_set.count({ neighborX,neighborZ }))
                continue;

            //ノードが既に作成済みかチェック
            Node* neighborNodeRtr = nullptr;
            float new_cost = currentNode->goal_cost + move_cost;

            auto it = node_map.find({ neighborX,neighborZ });
            if (it != node_map.end())
            {
                neighborNodeRtr = it->second;

                constexpr float EPS = 1e-5f;
                if (new_cost + EPS < neighborNodeRtr->goal_cost)
                {
                    neighborNodeRtr->goal_cost = new_cost;
                    neighborNodeRtr->parent = currentNode;

                    //オープンリストに再追加
                    open_list.push(neighborNodeRtr);
                }

            }
            else
            {
                //新しいノードを作成
                auto neighborNode = node_pool.GetNode();
                neighborNode->Reset();
                neighborNode->node_x = neighborX;
                neighborNode->node_z = neighborZ;
                neighborNode->goal_cost = currentNode->goal_cost + move_cost;
                neighborNode->h_cost = Heuristic(neighborX, neighborZ, goal_cellX, goal_cellZ);
                neighborNode->parent = currentNode;

                node_map[{neighborX, neighborZ}] = neighborNode;
                open_list.push(neighborNode);
            }
        }
    }

    //ゴールに到達できなかった場合
    return {};
}

/*
    1 : 前回探索済みノードを残しておく
    2 : GridMap の変化を検知
    3 : 通れなくなったノードを無効化
    4 : 開いている経路が遮断された場合だけ再探索
    5 : 経路が有効なら再利用
*/

//動的再探索
std::vector<std::pair<int, int>> AStar::ReplanPath(
    int startX, int startZ,
    int goalX, int goalZ,
    const GridMap& gridMap,
    int agent_cellX,int agent_cellZ)
{
    //ゴールチェック 
    if (gridMap.IsBlocked(goalX, goalZ))
    {
        //ゴールが塞がれていたら、近傍セルに代替ゴールを探す
        auto neighbors = GetNeighbors(goalX, goalZ, gridMap);
        bool found = false;
        for (auto& [nx, nz] : neighbors)
        {
            if (!gridMap.IsBlocked(nx, nz))
            {
                goalX = nx;
                goalZ = nz;
                found = true;
                break;
            }
        }

        if (!found)
        {
            //周囲すべて障害物 → 経路破綻
            last_path.clear();
            return last_path;
        }
    }

    //自身の位置がふさがれていた場合
    if (gridMap.IsBlocked(agent_cellX, agent_cellZ))
    {
        //周囲に空いているセルを探す
        auto neighbors = GetNeighbors(agent_cellX, agent_cellZ, gridMap);
        bool found = false;
        for (auto& [nx, nz] : neighbors)
        {
            if (!gridMap.IsBlocked(nx, nz))
            {
                agent_cellX = nx;
                agent_cellZ = nz;
                found = true;
                break;
            }
        }

        if (!found)
        {
            //周囲すべて障害物 → 経路破綻
            last_path.clear();
            return last_path;
        }
    }

    //経路上の障害物検出
    bool pathBlocked = false;
    int replanStartIndex = 0;

    if (!last_path.empty())
    {
        //実際の現在位置に最も近いインデックスを探す
        float min_dist = (std::numeric_limits<float>::max)();
        for (int i = 0; i < static_cast<int>(last_path.size()); i++)
        {
            float dx = static_cast<float>(last_path[i].first - agent_cellX);
            float dz = static_cast<float>(last_path[i].second - agent_cellZ);
            float dist = (dx * dx) + (dz * dz);
            if (dist < min_dist)
            {
                min_dist = dist;
                replanStartIndex = i;
            }
        }

        //経路上に障害物があるかチェック
        for (int i = replanStartIndex; i < static_cast<int>(last_path.size()); i++)
        {
            auto [x, z] = last_path[i];
            if (gridMap.IsBlocked(x, z))
            {
                pathBlocked = true;
                break;
            }
        }
    }

    //経路が問題なければそのまま返す
    if (!pathBlocked && !last_path.empty())
        return last_path;

    //ノード再利用による部分再探索 
    for (auto it = node_map.begin(); it != node_map.end();)
    {
        auto [x, z] = it->first;
        //近傍範囲のみをクリア
        if (gridMap.IsBlocked(x, z) ||
            abs(x - agent_cellX) > 5 ||
            abs(z - agent_cellZ) > 5)
        {
            it = node_map.erase(it);
        }
        else
        {
            it++;
        }
    }

    node_pool.Reset();//メモリプールリセット

    //部分再探索開始セルを決定
    auto replan_start = (last_path.empty())
        ? std::make_pair(startX, startZ)
        : last_path[replanStartIndex];

    //部分再探索実行
    auto new_segment = FindPath(
        replan_start.first, replan_start.second,
        goalX, goalZ, gridMap
    );

    if (new_segment.empty())
    {
        //再探索失敗 → 前回の経路をそのまま返す
        return last_path;
    }

    //経路結合
    std::vector<std::pair<int, int>> new_path;
    if (!last_path.empty())
    {
        new_path.insert(
            new_path.end(),
            last_path.begin(),
            last_path.begin() + replanStartIndex + 1
        );
    }

    if (!last_path.empty() && replanStartIndex < static_cast<int>(last_path.size()) - 1)
    {
        new_path.insert(
            new_path.end(),
            last_path.begin(),
            last_path.begin() + replanStartIndex + 1
        );
    }
    last_path = new_path;
    return last_path;
}

float AStar::Heuristic(
    int current_cellX,
    int current_cellZ,
    int goal_cellX,
    int goal_cellZ) const
{
    float deltaX = static_cast<float>(current_cellX - goal_cellX);
    float deltaZ = static_cast<float>(current_cellZ - goal_cellZ);

    return std::sqrt(deltaX * deltaX + deltaZ * deltaZ);
}

std::vector<std::pair<int, int>> AStar::GetNeighbors(
    int cellX,
    int cellZ,
    const GridMap& grid_map) const
{
    std::vector<std::pair<int, int>> neighbors;

    //4方向（左・右・上・下）
    const int offsetX[4] = { -1,1,0,0 };
    const int offsetZ[4] = { 0,0,-1,1 };

    for (int i = 0; i < 4; i++)
    {
        int neighborX = cellX + offsetX[i];
        int neighborZ = cellZ + offsetZ[i];

        //通行可能なら隣接セルとして追加
        if (!grid_map.IsBlocked(neighborX, neighborZ))
        {
            neighbors.push_back({ neighborX ,neighborZ });
        }
    }

    return neighbors;
}
