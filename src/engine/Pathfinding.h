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
      return path; // Target unreachable

    // Open Set (Nodes to be evaluated)
    // Using a simple vector for simplicity, priority_queue is better for
    // performance but harder to search/update
    std::vector<Node *> openSet;
    std::vector<Node *> closedSet;

    Node *startNode = new Node{startX, startY, 0, 0, nullptr};
    startNode->hCost = Heuristic(startX, startY, endX, endY);
    openSet.push_back(startNode);

    while (!openSet.empty()) {
      // Find node with lowest F cost
      auto currentIt = openSet.begin();
      for (auto it = openSet.begin(); it != openSet.end(); ++it) {
        if ((*it)->fCost() < (*currentIt)->fCost() ||
            ((*it)->fCost() == (*currentIt)->fCost() &&
             (*it)->hCost < (*currentIt)->hCost)) {
          currentIt = it;
        }
      }

      Node *current = *currentIt;

      // Remove from Open, Add to Closed
      openSet.erase(currentIt);
      closedSet.push_back(current);

      // Found target?
      if (current->x == endX && current->y == endY) {
        RetracePath(startNode, current, path);
        Cleanup(openSet, closedSet);
        return path;
      }

      // Neighbors
      // 4-Directional movement (or 8?)
      int dx[] = {0, 0, -1, 1};
      int dy[] = {-1, 1, 0, 0};

      for (int i = 0; i < 4; ++i) {
        int nx = current->x + dx[i];
        int ny = current->y + dy[i];

        if (!map.IsWalkable(nx, ny))
          continue;
        if (IsInList(closedSet, nx, ny))
          continue;

        float newMovementCostToNeighbor =
            current->gCost + 1; // Distance between neighbors is 1

        Node *neighbor = GetFromList(openSet, nx, ny);
        if (neighbor == nullptr) {
          neighbor = new Node{nx, ny, 0, 0, nullptr};
          openSet.push_back(neighbor);
        } else if (newMovementCostToNeighbor >= neighbor->gCost) {
          continue; // Not a better path
        }

        neighbor->gCost = newMovementCostToNeighbor;
        neighbor->hCost = Heuristic(nx, ny, endX, endY);
        neighbor->parent = current;
      }
    }

    Cleanup(openSet, closedSet);
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
    while (current != startNode) {
      path.push_back({current->x, current->y});
      current = current->parent;
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
