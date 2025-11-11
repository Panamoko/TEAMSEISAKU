#include "AStar.h"

std::vector<std::pair<int, int>> AStar::FindPath(
    int start_cellX,
    int start_cellZ,
    int goal_cellX,
    int goal_cellZ,
    const GridMap& gridMap)
{
    return std::vector<std::pair<int, int>>();
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
