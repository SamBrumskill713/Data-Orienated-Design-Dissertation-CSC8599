#include <vector>
#include <functional>
#include "Vector.h"
#include "CollisionDetectionDOD.h"

namespace NCL {
	using namespace NCL::Maths;
	namespace CSC8503 {

		template<class T>
		struct QuadTreeEntryDOD {
			Vector3 pos;
			Vector3 size;
			T object;

			QuadTreeEntryDOD() : pos(Vector3()), size(Vector3()), object(T()) {}

			QuadTreeEntryDOD(T obj, Vector3 objPos, Vector3 objSize)
				: pos(objPos), size(objSize), object(obj) {
			}
		};

		template<class T>
		struct QuadTreeNodeDOD {
			Vector2 position;
			Vector2 size;
			std::vector<QuadTreeEntryDOD<T>> contents;
			int childrenIndices[4];

			QuadTreeNodeDOD() : position(Vector2()), size(Vector2()) {
				for (int i = 0; i < 4; ++i) {
					childrenIndices[i] = -1;
				}
			}

			QuadTreeNodeDOD(Vector2 pos, Vector2 sz)
				: position(pos), size(sz) {
				for (int i = 0; i < 4; ++i) {
					childrenIndices[i] = -1;
				}
			}

			bool HasChildren() const {
				return childrenIndices[0] != -1;
			}

			bool IsLeaf() const {
				return !HasChildren();
			}
		};

		template<class T>
		class QuadTreeDOD {
		public:
			typedef std::function<void(std::vector<QuadTreeEntryDOD<T>>&)> QuadTreeFunc;

			QuadTreeDOD(Vector2 size, int maxDepth = 6, int maxSize = 5)
				: maxDepth(maxDepth), maxSize(maxSize), treeSize(size) {
				nodes.reserve(256);
				nodes.emplace_back(QuadTreeNodeDOD<T>(Vector2(), size));
			}

			~QuadTreeDOD() = default;

			void Insert(T object, const Vector3& pos, const Vector3& objSize) {
				InsertRecursive(0, object, pos, objSize, maxDepth);
			}

			void OperateOnContents(QuadTreeFunc func) {
				OperateOnContentsRecursive(0, func);
			}

			void Clear() {
				nodes.clear();
				nodes.emplace_back(QuadTreeNodeDOD<T>(Vector2(), treeSize));
			}

			void DebugDraw() {

			}

		private:
			std::vector<QuadTreeNodeDOD<T>> nodes;
			int maxDepth;
			int maxSize;
			Vector2 treeSize;

			bool ContainsAABB(const Vector3& innerPos, const Vector3& innerSize,
				const Vector3& outerPos, const Vector3& outerSize) const {

				Vector3 innerHalf(innerSize.x * 0.5f, innerSize.y * 0.5f, innerSize.z * 0.5f);
				Vector3 outerHalf(outerSize.x * 0.5f, outerSize.y * 0.5f, outerSize.z * 0.5f);

				Vector3 innerMin = innerPos - innerHalf;
				Vector3 innerMax = innerPos + innerHalf;
				Vector3 outerMin = outerPos - outerHalf;
				Vector3 outerMax = outerPos + outerHalf;

				return innerMin.x >= outerMin.x && innerMax.x <= outerMax.x &&
					innerMin.z >= outerMin.z && innerMax.z <= outerMax.z;
			}

			int GetContainingChildIndex(int nodeIndex, const Vector3& pos, const Vector3& objSize) const {
				if (nodeIndex < 0 || nodeIndex >= (int)nodes.size()) {
					return -1;
				}

				const QuadTreeNodeDOD<T>& node = nodes[nodeIndex];

				for (int i = 0; i < 4; ++i) {
					int childIndex = node.childrenIndices[i];
					if (childIndex == -1) {
						continue;
					}

					const QuadTreeNodeDOD<T>& child = nodes[childIndex];
					Vector3 childPos(child.position.x, 0.0f, child.position.y);
					Vector3 childSize(child.size.x, 1000.0f, child.size.y);

					if (ContainsAABB(pos, objSize, childPos, childSize)) {
						return childIndex;
					}
				}

				return -1;
			}

			void InsertRecursive(int nodeIndex, T object, const Vector3& pos,
				const Vector3& objSize, int depthLeft) {

				if (nodeIndex < 0 || nodeIndex >= (int)nodes.size()) {
					return;
				}

				QuadTreeNodeDOD<T>& node = nodes[nodeIndex];

				if (!CollisionDetectionDOD::AABBTest(pos,
					Vector3(node.position.x, 0, node.position.y), objSize,
					Vector3(node.size.x, 1000.0f, node.size.y))) {
					return;
				}

				if (node.HasChildren()) {
					int childIndex = GetContainingChildIndex(nodeIndex, pos, objSize);
					if (childIndex != -1) {
						InsertRecursive(childIndex, object, pos, objSize, depthLeft - 1);
					}
					else {
						node.contents.emplace_back(QuadTreeEntryDOD<T>(object, pos, objSize));
					}
				}
				else {
					node.contents.emplace_back(QuadTreeEntryDOD<T>(object, pos, objSize));

					if ((int)node.contents.size() > maxSize && depthLeft > 0) {
						std::vector<QuadTreeEntryDOD<T>> oldContents = node.contents;
						node.contents.clear();

						SplitNode(nodeIndex);

						for (const auto& entry : oldContents) {
							int childIndex = GetContainingChildIndex(nodeIndex, entry.pos, entry.size);
							if (childIndex != -1) {
								InsertRecursive(childIndex, entry.object, entry.pos, entry.size, depthLeft - 1);
							}
							else {
								nodes[nodeIndex].contents.emplace_back(entry);
							}
						}
					}
				}
			}

			void SplitNode(int nodeIndex) {
				if (nodeIndex < 0 || nodeIndex >= (int)nodes.size()) {
					return;
				}

				if (nodes[nodeIndex].HasChildren()) {
					return;
				}

				Vector2 nodePos = nodes[nodeIndex].position;
				Vector2 nodeSize = nodes[nodeIndex].size;
				Vector2 halfSize = nodeSize / 2.0f;
				int baseIndex = (int)nodes.size();

				nodes.emplace_back(QuadTreeNodeDOD<T>(nodePos + Vector2(-halfSize.x, halfSize.y), halfSize));
				nodes.emplace_back(QuadTreeNodeDOD<T>(nodePos + Vector2(halfSize.x, halfSize.y), halfSize));
				nodes.emplace_back(QuadTreeNodeDOD<T>(nodePos + Vector2(-halfSize.x, -halfSize.y), halfSize));
				nodes.emplace_back(QuadTreeNodeDOD<T>(nodePos + Vector2(halfSize.x, -halfSize.y), halfSize));

				for (int i = 0; i < 4; ++i) {
					nodes[nodeIndex].childrenIndices[i] = baseIndex + i;
				}
			}

			void OperateOnContentsRecursive(int nodeIndex, QuadTreeFunc& func) {
				if (nodeIndex < 0 || nodeIndex >= (int)nodes.size()) {
					return;
				}

				QuadTreeNodeDOD<T>& node = nodes[nodeIndex];

				if (node.HasChildren()) {
					for (int i = 0; i < 4; ++i) {
						if (node.childrenIndices[i] != -1) {
							OperateOnContentsRecursive(node.childrenIndices[i], func);
						}
					}
				}
				else {
					if (!node.contents.empty()) {
						func(node.contents);
					}
				}
			}
		};
	}
}