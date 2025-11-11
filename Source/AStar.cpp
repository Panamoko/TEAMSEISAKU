#include "AStar.h"

std::vector<std::pair<int, int>> AStar::FindPath(
    int start_cellX,
    int start_cellZ,
    int goal_cellX,
    int goal_cellZ,
    const GridMap& gridMap)
{
    auto startNode = std::make_unique<Node>();
    startNode->node_x = start_cellX;
    startNode->node_z = start_cellZ;
    startNode->goal_cost = 0.0f;
    startNode->h_cost = Heuristic(start_cellX, start_cellZ, goal_cellX, goal_cellZ);

    // allNodes‚É’Ç‰Á
    Node* startNodePtr = startNode.get();
    allNodes.push_back(std::move(startNode));
}

float AStar::Heuristic(
    int current_cellX,
    int current_cellZ,
    int goal_cellX,
    int goal_cellZ) const
{
    return 0.0f;
}

std::vector<std::pair<int, int>> AStar::GetNeighbors(
    int cellX,
    int cellZ,
    const GridMap& grid_map) const
{
    return std::vector<std::pair<int, int>>();
}
