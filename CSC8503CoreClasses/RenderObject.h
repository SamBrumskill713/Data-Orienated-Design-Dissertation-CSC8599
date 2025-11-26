#pragma once

namespace NCL {
	namespace Rendering {
		class Texture;
		class Shader;
		class Mesh;
	}
	using namespace NCL::Rendering;

	namespace CSC8503 {
		class Transform;
		using namespace Maths;

		enum class MaterialType{
			Opaque,
			Transparent,
			Effect
		};

		struct GameTechMaterial
		{
			MaterialType	type		= MaterialType::Opaque;
			Texture*		diffuseTex	= nullptr;
			Texture*		bumpTex		= nullptr;
		};

		class RenderObject
		{
		public:
			RenderObject(Transform& parentTransform, Mesh* mesh, const GameTechMaterial& material);
			~RenderObject() = default;

			Mesh*	GetMesh() const 
			{
				return mesh;
			}

			Transform&		GetTransform() const 
			{
				return transform;
			}

			void SetColour(const Vector4& c) 
			{
				colour = c;
			}

			Vector4 GetColour() const 
			{
				return colour;
			}

			GameTechMaterial GetMaterial() const 
			{
				return material;
			}

		protected:
			Transform&	transform;
			GameTechMaterial material;

			Mesh*		mesh;
			Vector4		colour;
		};
	}
}
