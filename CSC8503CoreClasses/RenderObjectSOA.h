#pragma once

#include "Vector.h"
#include "RenderObject.h"
#include <vector>

using namespace NCL::Maths;

namespace NCL {
	namespace Rendering {
		class Texture;
		class Shader;
		class Mesh;
	}
	using namespace NCL::Rendering;

	namespace CSC8503 {
		struct RenderObjectCompSOA {
			std::vector<Mesh*> meshes;
			std::vector<MaterialType> materialTypes;
			std::vector<Texture*> diffuseTextures;
			std::vector<Texture*> bumpTextures;
			std::vector<Vector4> colours;
		};

		namespace RenderOpsSOA {

			inline int GetCount(const RenderObjectCompSOA& render) {
				return (int)render.meshes.size();
			}

			inline int AddRenderObject(RenderObjectCompSOA& render,
				Mesh* mesh = nullptr,
				MaterialType materialType = MaterialType::Opaque,
				Texture* diffuseTex = nullptr,
				Texture* bumpTex = nullptr,
				const Vector4& colour = Vector4(1.0f, 1.0f, 1.0f, 1.0f)) {
				int index = GetCount(render);
				render.meshes.push_back(mesh);
				render.materialTypes.push_back(materialType);
				render.diffuseTextures.push_back(diffuseTex);
				render.bumpTextures.push_back(bumpTex);
				render.colours.push_back(colour);
				return index;
			}

			inline void RemoveRenderObject(RenderObjectCompSOA& render, int index) {
				if (index < 0 || index >= GetCount(render)) return;

				int lastIndex = GetCount(render) - 1;
				if (index != lastIndex) {
					render.meshes[index] = render.meshes[lastIndex];
					render.materialTypes[index] = render.materialTypes[lastIndex];
					render.diffuseTextures[index] = render.diffuseTextures[lastIndex];
					render.bumpTextures[index] = render.bumpTextures[lastIndex];
					render.colours[index] = render.colours[lastIndex];
				}

				render.meshes.pop_back();
				render.materialTypes.pop_back();
				render.diffuseTextures.pop_back();
				render.bumpTextures.pop_back();
				render.colours.pop_back();
			}

			inline void SetColour(RenderObjectCompSOA& render, int index, const Vector4& c) {
				if (index < 0 || index >= GetCount(render)) return;
				render.colours[index] = c;
			}

			inline void SetMesh(RenderObjectCompSOA& render, int index, Mesh* mesh) {
				if (index < 0 || index >= GetCount(render)) return;
				render.meshes[index] = mesh;
			}

			inline void SetDiffuseTexture(RenderObjectCompSOA& render, int index, Texture* tex) {
				if (index < 0 || index >= GetCount(render)) return;
				render.diffuseTextures[index] = tex;
			}

			inline void SetBumpTexture(RenderObjectCompSOA& render, int index, Texture* tex) {
				if (index < 0 || index >= GetCount(render)) return;
				render.bumpTextures[index] = tex;
			}

			inline void SetMaterialType(RenderObjectCompSOA& render, int index, MaterialType type) {
				if (index < 0 || index >= GetCount(render)) return;
				render.materialTypes[index] = type;
			}

			inline void SetMaterial(RenderObjectCompSOA& render, int index,
				MaterialType type, Texture* diffuseTex, Texture* bumpTex) {
				if (index < 0 || index >= GetCount(render)) return;
				render.materialTypes[index] = type;
				render.diffuseTextures[index] = diffuseTex;
				render.bumpTextures[index] = bumpTex;
			}
		}
	}
}