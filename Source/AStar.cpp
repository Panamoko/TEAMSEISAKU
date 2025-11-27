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
    size_t map_size = static_cast<size_t>(gridMap.GetWidth() * gridMap.GetHeight());//マップの全セル数（幅×高さ）を計算し、map_sizeに保存

    /*全セルに対応するノードポインタの配列（node_grid_pointers）を、
    マップサイズで初期化し、全てnullptr（ノード未作成）に設定*/
    node_grid_pointers.assign(map_size, nullptr);

    //ノードプールがマップの全セルを保持できるように、メモリを事前に確保
    node_pool.Reserve(map_size);

    //スタートノードを作成
    auto start_node = node_pool.GetNode();       //ノードを作成
    start_node->node_x = start_cellX;            //ノードのX座標をセット
    start_node->node_z = start_cellZ;            //ノードのZ座標をセット
    start_node->goal_cost = 0.0f;                //スタートからのコストは0
    start_node->h_cost = Heuristic(start_cellX, start_cellZ, goal_cellX, goal_cellZ);//推定コストを計算
    
    //スタートノードの座標を一次元配列のインデックスに変換
    size_t start_index = start_cellZ * map_width_ + start_cellX;

    //ノードポインタ配列の対応する位置に、作成したスタートノードのポインタを保存
    node_grid_pointers[start_index] = start_node;
    
    //探索待ちのノードを管理する優先度付きキュー（Open List）を初期化
    std::priority_queue<Node*, std::vector<Node*>, CompareNode> open_list;

    open_list.push(start_node);//作成したスタートノードをOpen Listに入れる
    max_search_nodes = 5000;//ノード探索数の上限を設定

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
            //ゴールノードからparentポインタをたどり、スタートノードまで逆向きに経路（座標のリスト）を復元
            std::vector<std::pair<int, int>> path;
            for (Node* trace = current_node; trace; trace = trace->parent)
                path.emplace_back(trace->node_x, trace->node_z);
            std::reverse(path.begin(), path.end());//復元した経路を正しい順序（スタートからゴールへ）に反転
            return path;//経路を返す
        }

        //現在のノードの探索が完了したため、その状態をCLOSEDに設定
        current_node->node_state = Node::CLOSED;

        // 隣接セルを取得するためのスタック上の配列
        std::pair<int, int> neighbors_array[4]; // 4方向なのでサイズは4

        /*現在のノードから通行可能な隣接セルの座標を取得し、
        その数を格納*/
        size_t neighbor_count = GetNeighbors(current_node->node_x, current_node->node_z, gridMap, neighbors_array);

        //取得した隣接セルを一つずつ処理するループを開始
        for (size_t i = 0; i < neighbor_count; ++i)
        {
            //ノードが既に作成済みかチェック
            Node* neighbor_node = nullptr;
            float new_cost = current_node->goal_cost + move_cost;//隣接セルまでの新しいGコスト（new_cost）を計算

            //隣接セルのX座標とZ座標を取得
            auto neighbor_x = neighbors_array[i].first;
            auto neighbor_z = neighbors_array[i].second;

            //マップ境界チェック
            if (!gridMap.IsOnMap(neighbor_x, neighbor_z)) continue;

            //IsBlocked チェック
            if (gridMap.IsBlocked(neighbor_x, neighbor_z))continue;

            //隣接セルの座標からインデックスを計算し、対応するノードポインタを配列から取得
            size_t neighbor_index = neighbor_z * map_width_ + neighbor_x;
            neighbor_node = node_grid_pointers[neighbor_index];

            //隣接セルに対応するノードが既に存在する場合
            if(neighbor_node != nullptr)
            {
                //隣接ノードがCLOSED（既に処理済み）であるかチェック
                if (neighbor_node->node_state == Node::CLOSED)
                {
                    /*現在の経路で到達する新しいGコストが、
                    既存のノードが持つ古いGコストより小さいかチェック*/

                    constexpr float EPS = 1e-5f;//浮動小数点誤差対策
                    if (new_cost + EPS < neighbor_node->goal_cost)
                    {
                        //ノードのGコストを更新し、親ノードを現在のノードに設定
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
    //ゴール到達判定の緩和
    const int GOAL_TOLERANCE_SQUARED = 2;
    int dist_x = agent_cellX - goalX;
    int dist_z = agent_cellZ - goalZ;
    int dist_sq = (dist_x * dist_x) + (dist_z * dist_z);

    if (dist_sq <= GOAL_TOLERANCE_SQUARED)
    {
        last_path.clear();
        return last_path;//ゴールに十分近いため、経路探索を終了
    }

    std::pair<int, int> neighbors_array[4];
    size_t neighbor_count;

    //ゴールチェック 
    int new_goalX = goalX;
    int new_goalZ = goalZ;

    if (gridMap.IsBlocked(goalX, goalZ))
    {
        //探索範囲の限界を設定
        const int MAX_SEARCH_DISTANCE = 50;

        std::queue<std::pair<int, int>> search_queue;
        std::unordered_set<std::pair<int, int>, pair_hash> visited_cells;

        search_queue.push({ goalX,goalZ });
        visited_cells.insert({ goalX,goalZ });

        bool found = false;

        //BFS(幅優先探索) を実行し、最も近い通行可能セルを探す
        while (!search_queue.empty())
        {
            auto current_pos = search_queue.front();
            search_queue.pop();

            int current_x = current_pos.first;
            int current_z = current_pos.second;

            //探索距離が制限を超えたら終了
            if (std::abs(current_x - goalX) > MAX_SEARCH_DISTANCE ||
                std::abs(current_z - goalZ) > MAX_SEARCH_DISTANCE)
            {
                break;
            }

            //通行可能なセルが見つかった場合
            if (!gridMap.IsBlocked(current_x, current_z))
            {
                new_goalX = current_x;
                new_goalZ = current_z;
                found = true;
                break;
            }

            //隣接セル（4方向）を取得
            neighbor_count = GetNeighbors(current_x, current_z, gridMap, neighbors_array);
            for (size_t i = 0; i < neighbor_count; i++)
            {
                auto next_pos = neighbors_array[i];

                if (!gridMap.IsOnMap(next_pos.first, next_pos.second))continue;

                if (visited_cells.find(next_pos) == visited_cells.end())
                {
                    visited_cells.insert(next_pos);
                    search_queue.push(next_pos);
                }
            }

        }

        if (!found)
        {
            //代替ゴールが見つからなかった
            last_path.clear();
            return last_path;
        }

        //見つかった場合は、新しいゴール座標をセット
        goalX = new_goalX;
        goalZ = new_goalZ;
    }

    //自身の位置がふさがれていた場合
    if (gridMap.IsBlocked(agent_cellX, agent_cellZ))
    {
        const int MAX_SEARCH_DISTANCE = 50; // ゴール探索と同じ距離制限

        std::queue<std::pair<int, int>> search_queue;
        std::unordered_set<std::pair<int, int>, pair_hash> visited_cells;

        search_queue.push({ agent_cellX, agent_cellZ });
        visited_cells.insert({ agent_cellX, agent_cellZ });

        int new_agent_cellX = agent_cellX;
        int new_agent_cellZ = agent_cellZ;
        bool found = false;

        // BFS (幅優先探索) を実行し、最も近い通行可能セルを探す
        while (!search_queue.empty())
        {
            auto current_pos = search_queue.front();
            search_queue.pop();

            int current_x = current_pos.first;
            int current_z = current_pos.second;

            if (std::abs(current_x - agent_cellX) > MAX_SEARCH_DISTANCE ||
                std::abs(current_z - agent_cellZ) > MAX_SEARCH_DISTANCE)
            {
                break;
            }

            // 通行可能なセルが見つかった場合
            if (!gridMap.IsBlocked(current_x, current_z))
            {
                new_agent_cellX = current_x;
                new_agent_cellZ = current_z;
                found = true;
                break;
            }

            neighbor_count = GetNeighbors(current_x, current_z, gridMap, neighbors_array);
            for (size_t i = 0; i < neighbor_count; ++i)
            {
                auto next_pos = neighbors_array[i];

                if (!gridMap.IsOnMap(next_pos.first, next_pos.second))continue;

                if (visited_cells.find(next_pos) == visited_cells.end())
                {
                    visited_cells.insert(next_pos);
                    search_queue.push(next_pos);
                }
            }
        }
        if (!found)
        {
            //周囲すべて障害物 → 経路破綻
            last_path.clear();
            return last_path;
        }

        //見つかった場合は、新しいエージェント座標をセット
        agent_cellX = new_agent_cellX;
        agent_cellZ = new_agent_cellZ;
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
        constexpr int LOOKAHEAD_NODES = 20;
        int check_end_index = (std::min)(
            static_cast<int>(last_path.size()),
            replan_start_index + LOOKAHEAD_NODES
            );

        for (int i = replan_start_index; i < check_end_index; i++)
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
    {
        //last_path = SmoothPath(last_path, gridMap);
        return last_path;
    }

    /*部分再探索の始点を決定*/
    int new_start_x = agent_cellX;
    int new_start_z = agent_cellZ;

    //部分再探索実行
    auto new_path_segment = FindPath(
        new_start_x, new_start_z, // 始点をエージェントの現在地に設定
        goalX, goalZ, gridMap
    );

    if (new_path_segment.empty())
    {
       // last_path = SmoothPath(last_path, gridMap);

        return last_path;
    }

    if (!last_path.empty() && !path_blocked)
    {
        std::vector<std::pair<int, int>> merged_path;

        //エージェントの現在地より前の、古い経路部分 (replan_start_indexまで) をコピー
        for (int i = 0; i < replan_start_index; i++)
        {
            merged_path.push_back(last_path[i]);
        }

        //新しい経路セグメント (new_path_segment) を結合
        for (const auto& cell : new_path_segment)
        {
            if (merged_path.empty() || merged_path.back() != cell)
            {
                merged_path.push_back(cell);
            }
        }

        //結合結果を last_path に反映
        last_path = merged_path;
    }
    else
    {
        last_path = new_path_segment;
    }

    //last_path = SmoothPath(last_path, gridMap);

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

        // ★隣接するすべてのセルを返す。

        out_neighbors[count] = { neighborX, neighborZ }; // 配列に直接書き込み
        count++;
    }

    return count; // 常に 4 を返す (4方向の場合)}
}

size_t AStar::CoordinateToIndex(int cell_x, int cell_z) const
{
    return static_cast<size_t>(cell_z * map_width_ + cell_x);
}

std::vector<std::pair<int, int>> AStar::SmoothPath(
    const std::vector<std::pair<int, int>>& path,
    const GridMap& gridMap) const
{
    // 経路が空、または短すぎる場合は平滑化しない
    if (path.size() <= 2)
    {
        return path;
    }

    std::vector<std::pair<int, int>> smooth_path;

    //始点 P_start は、必ず最初のノード
    smooth_path.push_back(path[0]);
    int start_index = 0;

    //経路の残りのノードをチェックしていく
    for (size_t current_index = 1; current_index < path.size(); current_index++)
    {
        auto current_pos = path[current_index];

        //P_start から P_candidate まで視線が通るかチェック
        bool line_of_sight = HasLineOfSight(
            smooth_path.back().first, smooth_path.back().second,
            current_pos.first, current_pos.second,
            gridMap
        );

        if (line_of_sight)
        {
            //視線が通る場合
            if (current_index == path.size() - 1)
            {
                smooth_path.push_back(current_pos);
            }
        }
        else
        {
            //視線が通らない場合

            size_t final_index = current_index - 1;

            //P_start と final_index のノードが異なる場合のみ追加 (重複防止)
            if (final_index > start_index)
            {
                smooth_path.push_back(path[final_index]);
                start_index = final_index;
            }

            start_index = final_index;
            current_index--;
        }
    }

    return smooth_path;
}

//経路上のセルをチェック
bool AStar::HasLineOfSight(
    int start_x, int start_z,
    int end_x, int end_z,
    const GridMap& gridMap) const
{
    // 始点と終点が同じ場合は、視線は通っている
    if (start_x == end_x && start_z == end_z) return true;

    int dx = std::abs(end_x - start_x);
    int dz = std::abs(end_z - start_z);
    int step_x = (start_x < end_x) ? 1 : -1;
    int step_z = (start_z < end_z) ? 1 : -1;
    int error_val;

    int x = start_x;
    int z = start_z;

    if (dx >= dz) // X軸方向の移動が主
    {
        error_val = 2 * dz - dx;
        while (x != end_x)
        {
            x += step_x; // 常にX軸方向にステップ

            if (error_val >= 0)
            {
                z += step_z; // Z軸方向にもステップ（対角移動）
                error_val -= 2 * dx;
            }
            error_val += 2 * dz;

            // 新しいセル (x, z) が障害物かどうかをチェック
            if (gridMap.IsBlocked(x, z)) return false;
        }
    }
    else // Z軸方向の移動が主
    {
        error_val = 2 * dx - dz;
        while (z != end_z)
        {
            z += step_z; // 常にZ軸方向にステップ

            if (error_val >= 0)
            {
                x += step_x; // X軸方向にもステップ（対角移動）
                error_val -= 2 * dz;
            }
            error_val += 2 * dx;

            // 新しいセル (x, z) が障害物かどうかをチェック
            if (gridMap.IsBlocked(x, z)) return false;
        }
    }

    return true;
}
