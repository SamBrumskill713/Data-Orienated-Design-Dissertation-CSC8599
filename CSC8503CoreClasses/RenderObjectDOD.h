#pragma once

#include "Vector.h"
#include "RenderObject.h"

using namespace NCL::Maths;

namespace NCL {
	namespace Rendering {
		class Texture;
		class Shader;
		class Mesh;
	}
	using namespace NCL::Rendering;

	namespace CSC8503 {
		// Use the existing GameTechMaterial from RenderObject.h
		// MaterialType is already defined in RenderObject.h

		struct RenderObjectComp {
			Mesh* mesh;
			GameTechMaterial material;
			Vector4 colour;

			RenderObjectComp()
				: mesh(nullptr),
				material(),
				colour(Vector4(1.0f, 1.0f, 1.0f, 1.0f)) {
			}
		};

		// Free functions for render operations - pure DOD
		namespace RenderOps {

			inline void SetColour(RenderObjectComp& render, const Vector4& c) {
				render.colour = c;
			}

			inline void SetMaterial(RenderObjectComp& render, const GameTechMaterial& mat) {
				render.material = mat;
			}

			inline void SetMesh(RenderObjectComp& render, Mesh* mesh) {
				render.mesh = mesh;
			}

			inline void SetDiffuseTexture(RenderObjectComp& render, Texture* tex) {
				render.material.diffuseTex = tex;
			}

			inline void SetBumpTexture(RenderObjectComp& render, Texture* tex) {
				render.material.bumpTex = tex;
			}

			inline void SetMaterialType(RenderObjectComp& render, MaterialType type) {
				render.material.type = type;
			}
		}
	}
}