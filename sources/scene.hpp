#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>

struct InstanceModel {
  short int meshID;
  short int materialID;
  Matrix finalMatrix;
};

struct InstanceBill {
  short int textureID;
  Vector3 position;
};

enum NodeType {
  NODE_TYPE_EMPTY,
  NODE_TYPE_MODEL,
  NODE_TYPE_BILLBOARD,
  NODE_TYPE_LIGHT,
};

struct Node {
  Vector3 position;
  Quaternion rotation;
  short int typeHandle;
  short int materialHandle;
  NodeType type;
  std::vector<Node> children;

  Matrix getMatrix(void);

  void generateRenderBatch(Matrix parentMatrix,
                           std::vector<InstanceModel> &modelBatch,
                           std::vector<InstanceBill> &billBatch);
  void createChildNode(Vector3 position, Quaternion rotation,
                     short int typeHandle, short int materialHandle, NodeType type);
};
struct Scene {
  Node root;
  Scene() { root = Node{.rotation = QuaternionIdentity(), .type = NODE_TYPE_EMPTY}; }
  void generateRenderBatch(std::vector<InstanceModel> &modelBatch,
                           std::vector<InstanceBill> &billBatch);
};