#pragma once
#include "CollisionDetection.h"
#include "Debug.h"

namespace NCL {
	using namespace NCL::Maths;
	namespace CSC8503 {
		template<class T>
		class QuadTree;

		template<class T>
		struct QuadTreeEntry
		{
			Vector3 pos;
			Vector3 size;
			T object;

			QuadTreeEntry(const T& obj, Vector3 pos, Vector3 size)
			{
				object = obj;
				this->pos = pos;
				this->size = size;
			}
		};

		template<class T>
		class QuadTreeNode {
		public:
			typedef std::function<void(std::list<QuadTreeEntry<T>>&)> QuadTreeFunc;
		protected:
			friend class QuadTree<T>;

			QuadTreeNode() {}

			QuadTreeNode(Vector2 pos, Vector2 size)
			{
				children = nullptr;
				this->position = pos;
				this->size = size;
			}

			~QuadTreeNode()
			{
				delete[] children;
			}

			bool ContainsAABB(const Vector3& innerPos, const Vector3& innerSize,
				const Vector3& outerPos, const Vector3& outerSize) const
			{
				Vector3 innerHalf = innerSize * 0.5f;
				Vector3 outerHalf = outerSize * 0.5f;

				Vector3 innerMin = innerPos - innerHalf;
				Vector3 innerMax = innerPos + innerHalf;
				Vector3 outerMin = outerPos - outerHalf;
				Vector3 outerMax = outerPos + outerHalf;

				return innerMin.x >= outerMin.x && innerMax.x <= outerMax.x &&
					innerMin.z >= outerMin.z && innerMax.z <= outerMax.z;
			}

			int GetContainingChildIndex(const Vector3& objectPos, const Vector3& objectSize) const
			{
				if (!children) {
					return -1;
				}

				for (int i = 0; i < 4; ++i) {
					Vector3 childPos(children[i].position.x, 0.0f, children[i].position.y);
					Vector3 childSize(children[i].size.x, 1000.0f, children[i].size.y);

					if (ContainsAABB(objectPos, objectSize, childPos, childSize)) {
						return i;
					}
				}

				return -1;
			}

			void Insert(const T& object, const Vector3& objectPos, const Vector3& objectSize, int depthLeft, int maxSize)
			{
				if (!CollisionDetection::AABBTest(objectPos,
					Vector3(position.x, 0, position.y), objectSize,
					Vector3(size.x, 1000.0f, size.y))) {
					return;
				}

				if (children) {
					int childIndex = GetContainingChildIndex(objectPos, objectSize);
					if (childIndex != -1) {
						children[childIndex].Insert(object, objectPos, objectSize, depthLeft - 1, maxSize);
					}
					else {
						contents.push_back(QuadTreeEntry<T>(object, objectPos, objectSize));
					}
				}
				else {
					contents.push_back(QuadTreeEntry<T>(object, objectPos, objectSize));
					if ((int)contents.size() > maxSize && depthLeft > 0) {
						if (!children) {
							std::list<QuadTreeEntry<T>> oldContents = contents;
							contents.clear();
							Split();

							for (const auto& entry : oldContents) {
								int childIndex = GetContainingChildIndex(entry.pos, entry.size);
								if (childIndex != -1) {
									children[childIndex].Insert(entry.object, entry.pos,
										entry.size, depthLeft - 1, maxSize);
								}
								else {
									contents.push_back(entry);
								}
							}
						}
					}
				}
			}

			void Split()
			{
				Vector2 halfSize = size / 2.0f;
				children = new QuadTreeNode<T>[4];
				children[0] = QuadTreeNode<T>(position + Vector2(-halfSize.x, halfSize.y), halfSize);
				children[1] = QuadTreeNode<T>(position + Vector2(halfSize.x, halfSize.y), halfSize);
				children[2] = QuadTreeNode<T>(position + Vector2(-halfSize.x, -halfSize.y), halfSize);
				children[3] = QuadTreeNode<T>(position + Vector2(halfSize.x, -halfSize.y), halfSize);
			}

			void DebugDraw()
			{
			}

			void OperateOnContents(QuadTreeFunc& func)
			{
				if (children) {
					for (int i = 0; i < 4; ++i) {
						children[i].OperateOnContents(func);
					}
				}
				else {
					if (!contents.empty()) {
						func(contents);
					}
				}
			}

		protected:
			std::list< QuadTreeEntry<T> >	contents;

			Vector2 position;
			Vector2 size;

			QuadTreeNode<T>* children;
		};
	}
}

namespace NCL {
	using namespace NCL::Maths;
	namespace CSC8503 {
		template<class T>
		class QuadTree
		{
		public:
			QuadTree(Vector2 size, int maxDepth = 6, int maxSize = 5)
			{
				root = QuadTreeNode<T>(Vector2(), size);
				this->maxDepth = maxDepth;
				this->maxSize = maxSize;
			}
			~QuadTree() = default;

			void Insert(T object, const Vector3& pos, const Vector3& size)
			{
				root.Insert(object, pos, size, maxDepth, maxSize);
			}

			void DebugDraw()
			{
				root.DebugDraw();
			}

			void OperateOnContents(typename QuadTreeNode<T>::QuadTreeFunc  func)
			{
				root.OperateOnContents(func);
			}

		protected:
			QuadTreeNode<T> root;
			int maxDepth;
			int maxSize;
		};
	}
}