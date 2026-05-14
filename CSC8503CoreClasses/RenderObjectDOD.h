#pragma once
#include "Vector.h"

using namespace NCL::Maths;

namespace NCL {
	namespace Rendering {
		class Texture;
		class Shader;
		class Mesh;
	}
	using namespace NCL::Rendering;

	namespace CSC8503 {
		enum class MaterialType {
			Opaque,
			Transparent,
			Effect
		};

		struct GameTechMaterial {
			MaterialType type = MaterialType::Opaque;
			Texture* diffuseTex = nullptr;
			Texture* bumpTex = nullptr;
		};

		struct RenderObjectComp {
			Mesh* mesh;
			GameTechMaterial material;
			Vector4 colour;

			RenderObjectComp()
				: mesh(nullptr),
				material(),
				colour(Vector4(1.0f, 1.0f, 1.0f, 1.0f)) {
			}

			RenderObjectComp(Mesh* renderMesh, const GameTechMaterial& renderMaterial)
				: mesh(renderMesh),
				material(renderMaterial),
				colour(Vector4(1.0f, 1.0f, 1.0f, 1.0f)) {
			}
		};

		struct RenderObjectSys {
			RenderObjectComp data;

			Mesh* GetMesh() const {
				return data.mesh;
			}

			Vector4 GetColour() const {
				return data.colour;
			}

			GameTechMaterial GetMaterial() const {
				return data.material;
			}

			RenderObjectSys& SetMesh(Mesh* renderMesh) {
				data.mesh = renderMesh;
				return *this;
			}

			RenderObjectSys& SetColour(const Vector4& c) {
				data.colour = c;
				return *this;
			}

			RenderObjectSys& SetMaterial(const GameTechMaterial& mat) {
				data.material = mat;
				return *this;
			}

			RenderObjectSys& SetDiffuseTexture(Texture* tex) {
				data.material.diffuseTex = tex;
				return *this;
			}

			RenderObjectSys& SetBumpTexture(Texture* tex) {
				data.material.bumpTex = tex;
				return *this;
			}

			RenderObjectSys& SetMaterialType(MaterialType type) {
				data.material.type = type;
				return *this;
			}

			Texture* GetDiffuseTexture() const {
				return data.material.diffuseTex;
			}

			Texture* GetBumpTexture() const {
				return data.material.bumpTex;
			}

			MaterialType GetMaterialType() const {
				return data.material.type;
			}
		};
	}
}