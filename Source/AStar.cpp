#include "AStar.h"
#include <set>
#include <unordered_set>


/*
1 : 初期化
    ノードプールとマップをリセット
    スタートノードを作成し、オープンリスト（未探索ノード集合）に登録。

2 : ループ開始
    優先度付きキュー（open_list）から最小 fCost のノードを取り出す。
    取り出したノードがゴールなら経路を復元して終了。

3 : 隣接ノード探索
    GetNeighbors()で上下左右の隣接セルを取得。
    それぞれについて通行可能なら新しいノードを作成し、コストを計算。

4 : コスト比較・更新
    既存ノードよりコストが安ければ更新し、再びopen_listに追加。

5 : 経路復元
    ゴール到達時に parent をたどって経路を逆算、リストにして返す。
*/

std::vector<std::pair<int, int>> AStar::FindPath(
    int start_cellX,
    int start_cellZ,
    int goal_cellX,
    int goal_cellZ,
    const GridMap& gridMap)
{
    //再初期化
    node_pool.Reset();

    //新しいノード管理の初期化
    map_width_ = gridMap.GetWidth();
    size_t map_size = static_cast<size_t>(gridMap.GetWidth() * gridMap.GetHeight());

    node_grid_pointers.assign(map_size, nullptr);

    node_pool.Reserve(map_size);

    //スタートノードを作成
    auto start_node = node_pool.GetNode();       //ノードを作成
    start_node->node_x = start_cellX;            //ノードのX座標をセット
    start_node->node_z = start_cellZ;            //ノードのZ座標をセット
    start_node->goal_cost = 0.0f;                //スタートからのコストは0
    start_node->h_cost = Heuristic(start_cellX, start_cellZ, goal_cellX, goal_cellZ);//推定コストを計算
    
    size_t start_index = start_cellZ * map_width_ + start_cellX;

    node_grid_pointers[start_index] = start_node;
    
    std::priority_queue<Node*, std::vector<Node*>, CompareNode> open_list;
    open_list.push(start_node);
    max_search_nodes = 5000;

    //探索ループ開始
    while (!open_list.empty())
    {
        if (node_pool.GetNextFreeIndex() > max_search_nodes)
        {
            //探索ノードが制限を超えたら終了
            return{};
        }

        //fCost が最小のノードを取得
        Node* current_node = open_list.top();
        open_list.pop();

        //既にクローズド済みならスキップ
        if (current_node->node_state == Node::CLOSED)
            continue;

        //ゴールに到達したか判定
        if (current_node->node_x == goal_cellX && current_node->node_z == goal_cellZ)
        {
            //経路復元
            std::vector<std::pair<int, int>> path;
            for (Node* trace = current_node; trace; trace = trace->parent)
                path.emplace_back(trace->node_x, trace->node_z);
            std::reverse(path.begin(), path.end());//正しい順番で返す
            return path;//経路を返す
        }

        //現ノードをクローズリストに追加
        current_node->node_state = Node::CLOSED; // ノードの状態をCLOSEDに更新

        // 隣接セルを取得するためのスタック上の配列
        std::pair<int, int> neighbors_array[4]; // 4方向なのでサイズは4
        size_t neighbor_count = GetNeighbors(current_node->node_x, current_node->node_z, gridMap, neighbors_array);


        for (size_t i = 0; i < neighbor_count; ++i)
        {
            //ノードが既に作成済みかチェック
            Node* neighbor_node = nullptr;
            float new_cost = current_node->goal_cost + move_cost;

            auto neighbor_x = neighbors_array[i].first;
            auto neighbor_z = neighbors_array[i].second;

            size_t neighbor_index = neighbor_z * map_width_ + neighbor_x;
            neighbor_node = node_grid_pointers[neighbor_index];

            if(neighbor_node != nullptr)
            {
                if (neighbor_node->node_state == Node::CLOSED)
                {
                    constexpr float EPS = 1e-5f;
                    if (new_cost + EPS < neighbor_node->goal_cost)
                    {
                        neighbor_node->goal_cost = new_cost;
                        neighbor_node->parent = current_node;

                        //オープンリストに再追加
                        open_list.push(neighbor_node);
                    }
                }

            }
            else
            {
                //新しいノードを作成
                auto neighborNode = node_pool.GetNode();
                neighborNode->Reset();
                neighborNode->node_x = neighbor_x;
                neighborNode->node_z = neighbor_z;
                neighborNode->goal_cost = current_node->goal_cost + move_cost;
                neighborNode->h_cost = Heuristic(neighbor_x, neighbor_z, goal_cellX, goal_cellZ);
                neighborNode->parent = current_node;
                neighborNode->node_state = Node::OPEN;

                node_grid_pointers[neighbor_index] = neighborNode;
                open_list.push(neighborNode);
            }
        }
    }

    //ゴールに到達できなかった場合
    return {};
}

/*
1 : ゴールや自分の位置が塞がれていないか確認
    塞がれていたら、近くの通行可能なセルを新たなゴール／出発点に設定。

2 : 前回の経路 (last_path) を確認
    もしまだ有効（障害物がない）なら再探索せず再利用。

3 : 経路の一部だけ再探索
    経路の一部が塞がれていた場合だけ、その区間からゴールまでを再探索。
    node_mapから周囲の不要ノードを削除し、再利用可能な範囲だけ残す。
    FindPath()を部分的に呼び出して、新しい経路を接続。

4 : 結果を保存
    新しい経路をlast_pathに保存し、次回の再探索に再利用。*/

//動的再探索
std::vector<std::pair<int, int>> AStar::ReplanPath(
    int startX, int startZ,
    int goalX, int goalZ,
    const GridMap& gridMap,
    int agent_cellX,int agent_cellZ)
{
    std::pair<int, int> neighbors_array[4];
    size_t neighbor_count;
    bool found = false;

    //ゴールチェック 
    if (gridMap.IsBlocked(goalX, goalZ))
    {
        //ゴールが塞がれていたら、近傍セルに代替ゴールを探す
        neighbor_count = GetNeighbors(goalX, goalZ, gridMap, neighbors_array);
        bool found = false;
        for (size_t i = 0; i < neighbor_count; ++i)
        {
            auto [nx, nz] = neighbors_array[i];

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
        neighbor_count = GetNeighbors(agent_cellX, agent_cellZ, gridMap, neighbors_array);        bool found = false;
        for (size_t i = 0; i < neighbor_count; ++i)
        {
            auto [nx, nz] = neighbors_array[i];

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
    bool path_blocked = false;
    int replan_start_index = 0;

    if (!last_path.empty())
    {
        //実際の現在位置に最も近いインデックスを探す
        float min_dist = (std::numeric_limits<float>::max)();

        //last_pathの最初の数ノードのみをチェック
        constexpr int SEARCH_RANGE = 20;
        int check_limit = (std::min)(static_cast<int>(last_path.size()), SEARCH_RANGE);

        for (int i = 0; i < check_limit; i++)
        {
            float dx = static_cast<float>(last_path[i].first - agent_cellX);
            float dz = static_cast<float>(last_path[i].second - agent_cellZ);
            float dist = (dx * dx) + (dz * dz);
            if (dist < min_dist)
            {
                min_dist = dist;
                replan_start_index = i;
            }
        }

        //経路上に障害物があるかチェック
        for (int i = replan_start_index; i < static_cast<int>(last_path.size()); i++)
        {
            auto [x, z] = last_path[i];
            if (gridMap.IsBlocked(x, z))
            {
                path_blocked = true;
                break;
            }
        }
    }

    //経路が問題なければそのまま返す
    if (!path_blocked && !last_path.empty())
        return last_path;




    //部分再探索実行
    auto new_path = FindPath(
        agent_cellX, agent_cellZ, // 始点をエージェントの現在地に設定
        goalX, goalZ, gridMap
    );

    if (new_path.empty())
    {
        //再探索失敗 → 前回の経路をそのまま返す
        return last_path;
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
    float delta_x = static_cast<float>(std::abs(current_cellX - goal_cellX));
    float delta_z = static_cast<float>(std::abs(current_cellZ - goal_cellZ));

    // 4方向探索に最適なマンハッタン距離
    return delta_x + delta_z;

    //float deltaX = static_cast<float>(current_cellX - goal_cellX);
    //float deltaZ = static_cast<float>(current_cellZ - goal_cellZ);

    //return std::sqrt(deltaX * deltaX + deltaZ * deltaZ);
}

size_t AStar::GetNeighbors(
    int cellX, int cellZ,
    const GridMap& grid_map,
    std::pair<int, int>* out_neighbors) const
{
    //4方向（左・右・上・下）
    const int offsetX[4] = { -1,1,0,0 };
    const int offsetZ[4] = { 0,0,-1,1 };
    size_t count = 0; // 見つかったセルの数をカウント

    for (int i = 0; i < 4; i++)
    {
        int neighborX = cellX + offsetX[i];
        int neighborZ = cellZ + offsetZ[i];

        //通行可能なら隣接セルとして追加
        if (!grid_map.IsBlocked(neighborX, neighborZ))
        {
            out_neighbors[count] = { neighborX, neighborZ }; // 配列に直接書き込み
            count++;
        }
    }

    return count; // 見つかったセルの数を返す
}

size_t AStar::CoordinateToIndex(int cell_x, int cell_z) const
{
    return static_cast<size_t>(cell_z * map_width + cell_x);
}
