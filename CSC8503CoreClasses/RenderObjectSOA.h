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
				return render.meshes.size();
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
				render.colours[index] = c;
			}

			inline void SetMesh(RenderObjectCompSOA& render, int index, Mesh* mesh) {
				render.meshes[index] = mesh;
			}

			inline void SetDiffuseTexture(RenderObjectCompSOA& render, int index, Texture* tex) {
				render.diffuseTextures[index] = tex;
			}

			inline void SetBumpTexture(RenderObjectCompSOA& render, int index, Texture* tex) {
				render.bumpTextures[index] = tex;
			}

			inline void SetMaterialType(RenderObjectCompSOA& render, int index, MaterialType type) {
				render.materialTypes[index] = type;
			}

			inline void SetMaterial(RenderObjectCompSOA& render, int index,
				MaterialType type, Texture* diffuseTex, Texture* bumpTex) {
				render.materialTypes[index] = type;
				render.diffuseTextures[index] = diffuseTex;
				render.bumpTextures[index] = bumpTex;
			}

			inline void SetColours(RenderObjectCompSOA& render,
				const std::vector<int>& indices,
				const std::vector<Vector4>& colours) {
				for (size_t i = 0; i < indices.size() && i < colours.size(); ++i) {
					render.colours[indices[i]] = colours[i];
				}
			}

			inline void SetDiffuseTextures(RenderObjectCompSOA& render,
				const std::vector<int>& indices,
				const std::vector<Texture*>& textures) {
				for (size_t i = 0; i < indices.size() && i < textures.size(); ++i) {
					render.diffuseTextures[indices[i]] = textures[i];
				}
			}

			inline void SetBumpTextures(RenderObjectCompSOA& render,
				const std::vector<int>& indices,
				const std::vector<Texture*>& textures) {
				for (size_t i = 0; i < indices.size() && i < textures.size(); ++i) {
					render.bumpTextures[indices[i]] = textures[i];
				}
			}

			inline void SetMaterialTypes(RenderObjectCompSOA& render,
				const std::vector<int>& indices,
				const std::vector<MaterialType>& types) {
				for (size_t i = 0; i < indices.size() && i < types.size(); ++i) {
					render.materialTypes[indices[i]] = types[i];
				}
			}

			inline void ApplyColourToAll(RenderObjectCompSOA& render, const Vector4& colour) {
				int count = GetCount(render);
				for (int i = 0; i < count; ++i) {
					render.colours[i] = colour;
				}
			}

			inline void ApplyDiffuseTextureToAll(RenderObjectCompSOA& render, Texture* texture) {
				int count = GetCount(render);
				for (int i = 0; i < count; ++i) {
					render.diffuseTextures[i] = texture;
				}
			}

			inline void ApplyBumpTextureToAll(RenderObjectCompSOA& render, Texture* texture) {
				int count = GetCount(render);
				for (int i = 0; i < count; ++i) {
					render.bumpTextures[i] = texture;
				}
			}

			inline void ApplyMaterialTypeToAll(RenderObjectCompSOA& render, MaterialType type) {
				int count = GetCount(render);
				for (int i = 0; i < count; ++i) {
					render.materialTypes[i] = type;
				}
			}

			inline void ApplyMaterialToAll(RenderObjectCompSOA& render,
				MaterialType type, Texture* diffuseTex, Texture* bumpTex) {
				int count = GetCount(render);
				for (int i = 0; i < count; ++i) {
					render.materialTypes[i] = type;
					render.diffuseTextures[i] = diffuseTex;
					render.bumpTextures[i] = bumpTex;
				}
			}

			inline std::vector<int> GetObjectsWithDiffuseTexture(const RenderObjectCompSOA& render, Texture* texture) {
				std::vector<int> result;
				int count = GetCount(render);
				for (int i = 0; i < count; ++i) {
					if (render.diffuseTextures[i] == texture) {
						result.push_back(i);
					}
				}
				return result;
			}

			inline std::vector<int> GetObjectsWithBumpTexture(const RenderObjectCompSOA& render, Texture* texture) {
				std::vector<int> result;
				int count = GetCount(render);
				for (int i = 0; i < count; ++i) {
					if (render.bumpTextures[i] == texture) {
						result.push_back(i);
					}
				}
				return result;
			}

			inline std::vector<int> GetObjectsWithMaterialType(const RenderObjectCompSOA& render, MaterialType type) {
				std::vector<int> result;
				int count = GetCount(render);
				for (int i = 0; i < count; ++i) {
					if (render.materialTypes[i] == type) {
						result.push_back(i);
					}
				}
				return result;
			}

			inline Texture* GetDiffuseTexture(const RenderObjectCompSOA& render, int index) {
				return render.diffuseTextures[index];
			}

			inline Texture* GetBumpTexture(const RenderObjectCompSOA& render, int index) {
				return render.bumpTextures[index];
			}

			inline MaterialType GetMaterialType(const RenderObjectCompSOA& render, int index) {
				return render.materialTypes[index];
			}

			inline Vector4 GetColour(const RenderObjectCompSOA& render, int index) {
				return render.colours[index];
			}

			inline Mesh* GetMesh(const RenderObjectCompSOA& render, int index) {
				return render.meshes[index];
			}

			inline GameTechMaterial GetMaterial(const RenderObjectCompSOA& render, int index) {
				GameTechMaterial mat;
				mat.type = render.materialTypes[index];
				mat.diffuseTex = render.diffuseTextures[index];
				mat.bumpTex = render.bumpTextures[index];
				return mat;
			}
		}
	}
}