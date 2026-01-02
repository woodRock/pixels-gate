#pragma once
#include "Tilemap.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_map>
#include <vector>

namespace PixelsEngine {

struct Node {
  int x, y;
  float gCost, hCost;
  Node *parent;

  float fCost() const { return gCost + hCost; }

  bool operator>(const Node &other) const { return fCost() > other.fCost(); }
};

class Pathfinding {
public:
  static std::vector<std::pair<int, int>>
  FindPath(const Tilemap &map, int startX, int startY, int endX, int endY) {
    std::vector<std::pair<int, int>> path;

    if (!map.IsWalkable(endX, endY))
      return path;

    if (startX == endX && startY == endY)
      return path;

    struct NodePtr {
        Node* node;
        float fCost; // Store fCost at time of queuing
        bool operator>(const NodePtr& other) const { return fCost > other.fCost; }
    };

    std::priority_queue<NodePtr, std::vector<NodePtr>, std::greater<NodePtr>> openSet;
    std::unordered_map<int, Node*> allNodes;

    auto GetKey = [](int x, int y) { return x * 10000 + y; };

    Node* startNode = new Node{startX, startY, 0, Heuristic(startX, startY, endX, endY), nullptr};
    allNodes[GetKey(startX, startY)] = startNode;
    openSet.push({startNode, startNode->fCost()});

    int nodesProcessed = 0;
    const int MAX_NODES = 5000; 

    while (!openSet.empty() && nodesProcessed < MAX_NODES) {
      NodePtr currentPtr = openSet.top();
      openSet.pop();
      Node* current = currentPtr.node;

      // If we found a better path to this node since it was queued, skip this entry
      if (currentPtr.fCost > current->fCost()) continue;

      nodesProcessed++;

      if (current->x == endX && current->y == endY) {
        RetracePath(startNode, current, path);
        for (auto& pair : allNodes) delete pair.second;
        return path;
      }

      int dx[] = {0, 0, -1, 1, -1, -1, 1, 1}; // 8-directional
      int dy[] = {-1, 1, 0, 0, -1, 1, -1, 1};

      for (int i = 0; i < 8; ++i) {
        int nx = current->x + dx[i];
        int ny = current->y + dy[i];

        if (!map.IsWalkable(nx, ny)) continue;

        float moveCost = (i < 4) ? 1.0f : 1.414f;
        float gScore = current->gCost + moveCost;
        int key = GetKey(nx, ny);

        if (allNodes.find(key) == allNodes.end() || gScore < allNodes[key]->gCost) {
            if (allNodes.find(key) == allNodes.end()) {
                Node* neighbor = new Node{nx, ny, gScore, Heuristic(nx, ny, endX, endY), current};
                allNodes[key] = neighbor;
                openSet.push({neighbor, neighbor->fCost()});
            } else {
                allNodes[key]->gCost = gScore;
                allNodes[key]->parent = current;
                openSet.push({allNodes[key], allNodes[key]->fCost()});
            }
        }
      }
    }

    for (auto& pair : allNodes) delete pair.second;
    return path;
  }

private:
  static float Heuristic(int x1, int y1, int x2, int y2) {
    // Manhattan distance for 4-way movement
    return std::abs(x1 - x2) + std::abs(y1 - y2);
  }

  static void RetracePath(Node *startNode, Node *endNode,
                          std::vector<std::pair<int, int>> &path) {
    Node *current = endNode;
    int safety = 0;
    while (current != startNode && current != nullptr && safety < 1000) {
      path.push_back({current->x, current->y});
      current = current->parent;
      safety++;
    }
    std::reverse(path.begin(), path.end());
  }

  static bool IsInList(const std::vector<Node *> &list, int x, int y) {
    for (auto *node : list) {
      if (node->x == x && node->y == y)
        return true;
    }
    return false;
  }

  static Node *GetFromList(const std::vector<Node *> &list, int x, int y) {
    for (auto *node : list) {
      if (node->x == x && node->y == y)
        return node;
    }
    return nullptr;
  }

  static void Cleanup(std::vector<Node *> &open, std::vector<Node *> &closed) {
    for (auto *n : open)
      delete n;
    for (auto *n : closed)
      delete n;
  }
};

} // namespace PixelsEngine
