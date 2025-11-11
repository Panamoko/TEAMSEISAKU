#include "AStar.h"
#include <set>

std::vector<std::pair<int, int>> AStar::FindPath(
    int start_cellX,
    int start_cellZ,
    int goal_cellX,
    int goal_cellZ,
    const GridMap& gridMap)
{
    //スタートノードを作成
    auto startNode = std::make_unique<Node>();  //ノードを作成
    startNode->node_x = start_cellX;            //ノードのX座標をセット
    startNode->node_z = start_cellZ;            //ノードのZ座標をセット
    startNode->goal_cost = 0.0f;                //スタートからのコストは0
    startNode->h_cost = Heuristic(start_cellX, start_cellZ, goal_cellX, goal_cellZ);//推定コストを計算

    // allNodesに追加して管理
    Node* startNodePtr = startNode.get();       //生ポインタを取得
    allNodes.push_back(std::move(startNode));   //allNodesに所有権を移して追加

    // オープンリストにスタートノードを追加
    std::priority_queue<Node*, std::vector<Node*>, CompareNode>open_list;
    open_list.push(startNodePtr);

    //クローズリストとして座標を保持するセット
    std::set<std::pair<int, int>> closed_set;

    while (!open_list.empty())
    {
        //fCost が最小のノードを取得
        Node* currentNode = open_list.top();
        open_list.pop();

        //ゴールに到達したか判定
        if (currentNode->node_x == goal_cellX && currentNode->node_z == goal_cellZ)
        {
            //経路復元
            std::vector<std::pair<int, int>> path;
            Node* traceNode = currentNode;
            while (traceNode)
            {
                path.push_back({ traceNode->node_x,traceNode->node_z });
                traceNode = traceNode->parent;
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        //現ノードをクローズリストに追加
        closed_set.insert({ currentNode->node_x,currentNode->node_z });

        // 隣接セルを取得
        auto neighbors = GetNeighbors(currentNode->node_x, currentNode->node_z, gridMap);

        for (auto& neighbor : neighbors)
        {
            int neighborX = neighbor.first;
            int neighborZ = neighbor.second;

            //クローズ済みならスキップ
            if (closed_set.find({ neighborX,neighborZ }) != closed_set.end()); 
            continue;

            //新しいノードを作成
            auto neighborNode = std::make_unique<Node>();
            neighborNode->node_x = neighborX;
            neighborNode->node_z = neighborZ;
            neighborNode->goal_cost = currentNode->goal_cost + move_cost;
            neighborNode->h_cost = Heuristic(neighborX, neighborZ, goal_cellX, goal_cellZ);
            neighborNode->parent = currentNode;

            Node* neighborNodePtr = neighborNode.get();
            allNodes.push_back(std::move(neighborNode));
            open_list.push(neighborNodePtr);
        }
    }

    //ゴールに到達できなかった場合
    return std::vector<std::pair<int, int>>();
}

float AStar::Heuristic(
    int current_cellX,
    int current_cellZ,
    int goal_cellX,
    int goal_cellZ) const
{
    //abs()で座標差を計算し、合計する
    int deltaX = abs(current_cellX - goal_cellX);
    int deltaZ = abs(current_cellZ - goal_cellZ);

    return static_cast<float>(deltaX + deltaZ);
}

std::vector<std::pair<int, int>> AStar::GetNeighbors(
    int cellX,
    int cellZ,
    const GridMap& grid_map) const
{
    std::vector<std::pair<int, int>> neighbors;

    //4方向（左・右・上・下）
    const float offsetX[4] = { -1,1,0,0 };
    const float offsetZ[4] = { 0,0,-1,1 };

    for (int i = 0; i < 4; i++)
    {
        float neighborX = cellX + offsetX[i];
        float neighborZ = cellZ + offsetZ[i];

        //通行可能なら隣接セルとして追加
        if (!grid_map.IsBloked(neighborX, neighborZ))
        {
            neighbors.push_back({ neighborX ,neighborZ });
        }
    }

    return neighbors;
}
