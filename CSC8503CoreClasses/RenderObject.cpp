#include "RenderObject.h"
#include "Mesh.h"

using namespace NCL::CSC8503;
using namespace NCL;

RenderObject::RenderObject(Transform& parentTransform, Mesh* mesh, const GameTechMaterial& material)
	: transform(parentTransform)
{
	this->material	= material;
	this->mesh		= mesh;
	this->colour	= Vector4(1.0f, 1.0f, 1.0f, 1.0f);
}