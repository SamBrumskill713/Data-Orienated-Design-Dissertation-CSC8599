#pragma once
#include <vector>
#include <functional>
#include "Vector.h"
#include "CollisionDetectionSOA.h"
#include "GameObjectSOA.h"

namespace NCL {
	using namespace NCL::Maths;
	namespace CSC8503 {

		template<class T>
		struct QuadTreeEntrySOA {
			Vector3 pos;
			Vector3 size;
			T object;

			QuadTreeEntrySOA() : pos(Vector3()), size(Vector3()), object(T()) {}

			QuadTreeEntrySOA(T obj, Vector3 objPos, Vector3 objSize)
				: pos(objPos), size(objSize), object(obj) {
			}
		};

		template<class T>
		struct QuadTreeNodeSOA {
			Vector2 position;
			Vector2 size;
			std::vector<QuadTreeEntrySOA<T>> contents;
			int childrenIndices[4];

			QuadTreeNodeSOA() : position(Vector2()), size(Vector2()) {
				for (int i = 0; i < 4; ++i) {
					childrenIndices[i] = -1;
				}
			}

			QuadTreeNodeSOA(Vector2 pos, Vector2 sz)
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
		class QuadTreeSOA {
		public:
			typedef std::function<void(std::vector<QuadTreeEntrySOA<T>>&)> QuadTreeFunc;

			QuadTreeSOA(Vector2 size, int maxDepth = 6, int maxSize = 5)
				: maxDepth(maxDepth), maxSize(maxSize) {

				nodes.push_back(QuadTreeNodeSOA<T>(Vector2(), size));
			}

			~QuadTreeSOA() = default;

			void Insert(T object, const Vector3& pos, const Vector3& objSize) {
				InsertRecursive(0, object, pos, objSize, maxDepth);
			}

			void OperateOnContents(QuadTreeFunc func) {
				OperateOnContentsRecursive(0, func);
			}

			void DebugDraw() {

			}

		private:
			std::vector<QuadTreeNodeSOA<T>> nodes;
			int maxDepth;
			int maxSize;

			void InsertRecursive(int nodeIndex, T object, const Vector3& pos,
				const Vector3& objSize, int depthLeft) {

				if (nodeIndex < 0 || nodeIndex >= (int)nodes.size()) {
					return;
				}

				QuadTreeNodeSOA<T>& node = nodes[nodeIndex];

				if (!CollisionDetectionSOA::AABBTest(pos,
					Vector3(node.position.x, 0, node.position.y), objSize,
					Vector3(node.size.x, 1000.0f, node.size.y))) {
					return;
				}

				if (node.HasChildren()) {
					for (int i = 0; i < 4; ++i) {
						if (node.childrenIndices[i] != -1) {
							InsertRecursive(node.childrenIndices[i], object, pos, objSize, depthLeft - 1);
						}
					}
				}
				else {
					node.contents.push_back(QuadTreeEntrySOA<T>(object, pos, objSize));

					if ((int)node.contents.size() > maxSize && depthLeft > 0) {
						if (node.IsLeaf()) {
							SplitNode(nodeIndex);

							std::vector<QuadTreeEntrySOA<T>> oldContents = node.contents;
							node.contents.clear();

							for (const auto& entry : oldContents) {
								for (int j = 0; j < 4; ++j) {
									if (node.childrenIndices[j] != -1) {
										InsertRecursive(node.childrenIndices[j], entry.object,
											entry.pos, entry.size, depthLeft - 1);
									}
								}
							}
						}
					}
				}
			}

			void SplitNode(int nodeIndex) {
				QuadTreeNodeSOA<T>& node = nodes[nodeIndex];

				if (node.HasChildren()) {
					return;
				}

				Vector2 halfSize = node.size / 2.0f;
				int baseIndex = (int)nodes.size();

				nodes.push_back(QuadTreeNodeSOA<T>(node.position + Vector2(-halfSize.x, halfSize.y), halfSize));
				nodes.push_back(QuadTreeNodeSOA<T>(node.position + Vector2(halfSize.x, halfSize.y), halfSize));
				nodes.push_back(QuadTreeNodeSOA<T>(node.position + Vector2(-halfSize.x, -halfSize.y), halfSize));
				nodes.push_back(QuadTreeNodeSOA<T>(node.position + Vector2(halfSize.x, -halfSize.y), halfSize));

				for (int i = 0; i < 4; ++i) {
					node.childrenIndices[i] = baseIndex + i;
				}
			}

			void OperateOnContentsRecursive(int nodeIndex, QuadTreeFunc& func) {
				if (nodeIndex < 0 || nodeIndex >= (int)nodes.size()) {
					return;
				}

				QuadTreeNodeSOA<T>& node = nodes[nodeIndex];

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