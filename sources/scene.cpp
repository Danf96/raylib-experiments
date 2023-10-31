#include "scene.hpp"

Matrix Node::getMatrix(void) {
  return MatrixMultiply(MatrixInvert(QuaternionToMatrix(rotation)),
                        MatrixTranslate(position.x, position.y, position.z));
}

void Node::generateRenderBatch(Matrix parentMatrix,
                               std::vector<InstanceModel> &modelBatch,
                               std::vector<InstanceBill> &billBatch) {
  Matrix newMatrix = MatrixMultiply(getMatrix(), parentMatrix);
  if (type == NODE_TYPE_MODEL) {
    modelBatch.push_back({typeHandle, materialHandle, newMatrix});
  } else if (type == NODE_TYPE_BILLBOARD) {
    billBatch.push_back(
        {typeHandle, Vector3Transform({}, newMatrix)});
  }
  for (auto &child : children) {
    child.generateRenderBatch(newMatrix, modelBatch, billBatch);
  }
}

void Node::createChildNode(Vector3 position, Quaternion rotation,
                           short int typeHandle, short int materialHandle,
                           NodeType type) {
  this->children.push_back(
      {position, rotation, typeHandle, materialHandle, type, {}});
}

void Scene::generateRenderBatch(std::vector<InstanceModel> &modelBatch,
                                std::vector<InstanceBill> &billBatch) {
  root.generateRenderBatch(MatrixIdentity(), modelBatch, billBatch);
}