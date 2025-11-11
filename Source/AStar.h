#pragma once

#include<vector>
#include <memory>
#include <queue>
#include "GridMap.h"

class AStar
{
public:
	/*
		startX,startZ:開始セル座標
		goalX,goalZ:目的セル座標
		gridMap:通行可能/不可情報を持つグリッドマップ
	*/
	std::vector<std::pair<int, int>>FindPath(
		int start_cellX, int start_cellZ,
		int goal_cellX, int goal_cellZ,
		const GridMap& gridMap
	);

private:

	//探索ノード
	struct Node
	{
		int node_x, node_z;//ノード座標
		float goal_cost = 0.0f;//スタートからこのノードまでの累計コスト
		float h_cost = 0.0f;//このノードからゴールまでの推定コスト
		float fCost() const { return goal_cost + h_cost; }//総コスト

		Node* parent = nullptr;//経路復元用の親ノード
	};

	//ノード比較用（優先度付きキューでfCostが小さい順）
	struct CompareNode
	{
		bool operator()(const Node* nodeA, const Node* nodeB)const
		{
			return nodeA->fCost() > nodeB->fCost();
		}
	};

	//ヒューリスティック関数
	float Heuristic(int current_cellX, int current_cellZ, int goal_cellX, int goal_cellZ)const;

	//隣接セルを取得
	std::vector<std::pair<int, int>> GetNeighbors(int cellX, int cellZ, const GridMap& grid_map)const;

	//メモリ管理用
	std::vector<std::unique_ptr<Node>> allNodes;//動的に作るノードを保持

};

