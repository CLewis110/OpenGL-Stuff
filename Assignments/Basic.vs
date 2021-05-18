#version 430 core

layout(location=0) in vec3 position;
layout(location=1) in vec4 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

out vec4 vertexColor;
out vec4 interPos;
out vec3 interNormal;
out vec2 interUV;

uniform mat4 modelMat;

uniform mat4 viewMat;
uniform mat4 projMat;
uniform mat3 normMat;


void main()
{		
	// Get position of vertex (object space)
	vec4 objPos = vec4(position, 1.0);
	vec4 vpos = viewMat * modelMat * objPos;
	interPos = vpos;

	gl_Position =  projMat * vpos;

	interNormal = normMat * normal;

	interUV = uv;

	// Output per-vertex color
	vertexColor = color;
}
