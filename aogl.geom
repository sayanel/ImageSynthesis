#version 410 core

#define POSITION	0
#define NORMAL		1
#define TEXCOORD	2
#define FRAG_COLOR	0

precision highp float;
precision highp int;
layout(std140, column_major) uniform;
layout(triangles) in;
layout(triangle_strip, max_vertices = 4) out;

mat4 rotationMatrix(vec3 axis, float angle);

in block
{
    vec2 TexCoord;
    vec3 Position;
    vec3 Normal;
} In[]; 

out block
{
    vec2 TexCoord;
    vec3 Position;
    vec3 Normal;
}Out;

uniform mat4 MVP;
uniform mat4 MV;
uniform float Time;

void main()
{   

	float w = Time * sin(gl_PrimitiveIDIn);
	w = 0;
	mat4 rotateMatrix = rotationMatrix(vec3(1.,0.,1.) , w);


    for(int i = 0; i < In.length(); ++i)
    {
    	gl_Position = MVP * vec4(In[i].Position,1.0);
        Out.TexCoord = In[i].TexCoord;
        Out.Position = In[i].Position;
        Out.Normal = In[i].Normal;
        EmitVertex();
    }
    EndPrimitive();
}








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


