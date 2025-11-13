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

	//動的再探索
	std::vector<std::pair<int, int>> ReplanPath(
		int startX, int startZ,
		int goalX, int goalZ,
		const GridMap& gridMap,
		int agent_cellX, int agent_cellZ
	);

private:

	float move_cost = 1.0f;//移動コスト

	//A*探索の最小単位、セルごとの情報を持つ
	struct Node
	{
		int node_x, node_z;//ノード座標
		float goal_cost = 0.0f;//スタートからこのノードまでの累計コスト
		float h_cost = 0.0f;//このノードからゴールまでの推定コスト
		float fCost() const { return goal_cost + h_cost; }//総コスト

		Node* parent = nullptr;//経路復元用の親ノード

		void Reset()
		{
			goal_cost = 0.0f;
			h_cost = 0.0f;
			parent = nullptr;
			node_x = node_z = 0;
		}
	};

	class NodePool
	{
	public:
		void Reserve(size_t size) { pool.reserve(size); }
		void Reset() { next_free_index = 0; }
		Node* GetNode()
		{
			if (next_free_index >= pool.size())
			{
				pool.emplace_back();
			}
			return &pool[next_free_index++];
		}
	private:
		std::vector<Node> pool;
		size_t next_free_index = 0;
	};

	NodePool node_pool;

	//ノード比較用（優先度付きキューでfCostが小さい順）
	struct CompareNode
	{
		bool operator()(const Node* nodeA, const Node* nodeB)const
		{
			if (nodeA->fCost() == nodeB->fCost())
				return nodeA->h_cost > nodeB->h_cost;//同値なら推定コストが小さい方を優先
			return nodeA->fCost() > nodeB->fCost();
		}
	};

	//(x,z) の座標ペアを unordered_map で使うためのハッシュ関数
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

	std::vector<std::pair<int, int>> last_path; // 前回の経路
};

