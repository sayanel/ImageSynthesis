#version 410 core

#define POSITION	0
#define NORMAL		1
#define TEXCOORD	2
#define FRAG_COLOR	0

precision highp float;
precision highp int;

uniform mat4 MVP;
uniform mat4 MV;
uniform mat4 M;
uniform float Time;

uniform float CounterCube;
uniform float CounterPlane;

layout(location = POSITION) in vec3 Position;
layout(location = NORMAL) in vec3 Normal;
layout(location = TEXCOORD) in vec2 TexCoord;

// in int gl_VertexID;
in int gl_InstanceID;

out gl_PerVertex
{
	vec4 gl_Position;

};

out block
{
	vec2 TexCoord;
	vec3 Position; 
	vec3 Normal;
} Out;

mat4 rotationMatrix(vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;

    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}

void main()
{	


	float w = Time * gl_InstanceID;
	// w = 0;
	mat4 rotateMatrix = rotationMatrix(vec3(0.,1.,0.) , w);


	vec3 pos = Position;

	if(Time != 0) pos.y += 1.2*gl_InstanceID; // Cubes
	else{ // Planes 	
		vec3 newPos;
		newPos.x = mod(gl_InstanceID, int(sqrt(CounterPlane)));
		newPos.y = 0;
		newPos.z = gl_InstanceID / int(sqrt(CounterPlane));
		newPos.x *=10.2;
		newPos.z *=10.2;
		pos+= newPos;
	}

	vec3 nor = Normal;

	Out.TexCoord = TexCoord;

	// // Repère Camera
	// Out.Position = (MV * rotateMatrix  * vec4(pos,1.0)).xyz;
	// Out.Normal = (transpose(inverse(MV)) * rotateMatrix  * vec4(nor,1.0)).xyz;
	
	// Repère Objet
	Out.Position = (rotateMatrix  * vec4(pos,1.0)).xyz;
	Out.Normal = (rotateMatrix  * vec4(nor,1.0)).xyz;

	gl_Position =  MVP * rotateMatrix * vec4(pos, 1.0);
}


    // length(fract(5.0 * P))
    // length(abs(fract(5.0 * P) * 2.0 - 1.0))
    // mod(floor(10.0 * P.x) + floor(10.0 * P.y), 2.0)
    // smoothstep(0.3, 0.32, length(fract(5.0 * P) - 0.5))
    // smoothstep(0.4, 0.5, max(abs(fract(8.0 * P.x - 0.5 * mod(floor(8.0 * P.y), 2.0)) - 0.5), abs(fract(8.0 * P.y) - 0.5)))

