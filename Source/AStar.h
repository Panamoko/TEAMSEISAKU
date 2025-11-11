#pragma once

#include<vector>
#include <memory>
#include <queue>
#include "GridMap.h"
#include <unordered_map>

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

	float move_cost = 1.0f;//移動コスト

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

	struct pair_hash
	{
		template<class T1,class T2>
		std::size_t operator()(const std::pair<T1, T2>& p)const
		{
			auto h1 = std::hash<T1>{}(p.first);
			auto h2 = std::hash<T2>{}(p.second);
			return h1 ^ (h2 << 1);//XORで合成
		}
	};

	//ヒューリスティック関数
	float Heuristic(int current_cellX, int current_cellZ, int goal_cellX, int goal_cellZ)const;

	//隣接セルを取得
	std::vector<std::pair<int, int>> GetNeighbors(int cellX, int cellZ, const GridMap& grid_map)const;

	//メモリ管理用
	std::vector<std::unique_ptr<Node>> allNodes;//動的に作るノードを保持

	//ノード管理用マップ
	std::unordered_map<std::pair<int, int>, Node*, pair_hash> node_map;
};

