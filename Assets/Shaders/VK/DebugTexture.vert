#version 450
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive	   : enable

#include "Data.glslh"

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec2 inTexCoord;

layout (location = 2) in vec2 inInstancePos;
layout (location = 3) in vec2 inInstanceScale;
layout (location = 4) in vec4 inInstanceColour;
layout (location = 5) in int  inInstanceTex;

layout (location = 0) out Vertex {
	vec4		colour;
	vec2		texCoord;
	flat int	texID;
} OUT;

void main() {
	vec4 worldPos	= vec4((inPosition.xy * inInstanceScale.xy) + inInstancePos.xy, 0, 1);

	OUT.colour		= inInstanceColour;
	OUT.texCoord	= inTexCoord;
	OUT.texID		= inInstanceTex;

	gl_Position 	= orthoMatrix * worldPos;
}