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

vec3 mod289(vec3 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec2 mod289(vec2 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec3 permute(vec3 x) {
  return mod289(((x*34.0)+1.0)*x);
}

// Author snoise function: Ian McEwan, Ashima Arts. https://github.com/ashima/webgl-noise
float snoise(vec2 v){
	const vec4 C = vec4(0.211324865405187,  // (3.0-sqrt(3.0))/6.0
	              0.366025403784439,  // 0.5*(sqrt(3.0)-1.0)
	             -0.577350269189626,  // -1.0 + 2.0 * C.x
	              0.024390243902439); // 1.0 / 41.0
	// First corner
	vec2 i  = floor(v + dot(v, C.yy) );
	vec2 x0 = v -   i + dot(i, C.xx);

	// Other corners
	vec2 i1;
	//i1.x = step( x0.y, x0.x ); // x0.x > x0.y ? 1.0 : 0.0
	//i1.y = 1.0 - i1.x;
	i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
	// x0 = x0 - 0.0 + 0.0 * C.xx ;
	// x1 = x0 - i1 + 1.0 * C.xx ;
	// x2 = x0 - 1.0 + 2.0 * C.xx ;
	vec4 x12 = x0.xyxy + C.xxzz;
	x12.xy -= i1;

	// Permutations
	i = mod289(i); // Avoid truncation effects in permutation
	vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 )) + i.x + vec3(0.0, i1.x, 1.0 ));

	vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
	m = m*m ;
	m = m*m ;

	// Gradients: 41 points uniformly over a line, mapped onto a diamond.
	// The ring size 17*17 = 289 is close to a multiple of 41 (41*7 = 287)
	vec3 x = 2.0 * fract(p * C.www) - 1.0;
	vec3 h = abs(x) - 0.5;
	vec3 ox = floor(x + 0.5);
	vec3 a0 = x - ox;

	// Normalise gradients implicitly by scaling m
	// Approximation of: m *= inversesqrt( a0*a0 + h*h );
	m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );

	// Compute final noise value at P
	vec3 g;
	g.x  = a0.x  * x0.x  + h.x  * x0.y;
	g.yz = a0.yz * x12.xz + h.yz * x12.yw;
	return 130.0 * dot(m, g);
}

void main()
{	


	// float w = Time * gl_InstanceID/10;
	float w = Time;
	w = 0;
	mat4 rotateMatrix = rotationMatrix(vec3(0.,1.,0.) , w);


	vec3 pos = Position;

	if(Time != 0){ // Cubes
		// pos.y += 1.2*gl_InstanceID; 	
		vec3 newPos = vec3(0.0,0.0,0.0);
		newPos.x = mod(gl_InstanceID, int(sqrt(CounterCube)));
		newPos.z = gl_InstanceID / int(sqrt(CounterCube));
		newPos.x *= 2.0;
		newPos.z *= 2.0;
		newPos.y =  snoise(newPos.xz * 5.0);
		newPos.x +=  snoise(newPos.xz * 5.0);
		newPos.z +=  snoise(newPos.xz* 5.0);
		newPos.x -= 300;
		newPos.z -= 300;


		pos+= newPos;

		rotateMatrix =  rotationMatrix(pos+vec3(0.0, 0.5, 0.0) , w);
	} 
	else{ // Planes 	
		vec3 newPos;
		int decal = 1200;
		newPos.x = mod(gl_InstanceID, int(sqrt(CounterPlane)));
		newPos.y = 0;
		newPos.z = gl_InstanceID / int(sqrt(CounterPlane));
		newPos.x *=10.2;
		newPos.z *=10.2;
		newPos.x -= decal;
		newPos.z -= decal;

		pos+= newPos;
	}

	

	vec3 nor = Normal;

	Out.TexCoord = TexCoord;

	// // Repère Camera
	// Out.Position = (MV * rotateMatrix  * vec4(pos,1.0)).xyz;
	// Out.Normal = (transpose(inverse(MV)) * rotateMatrix  * vec4(nor,1.0)).xyz;
	
	// Repère Objet
	Out.Position = (rotateMatrix  * vec4(pos,1.0)).xyz;
	Out.Normal = (MV * rotateMatrix  * vec4(nor,0.0)).xyz ;

	gl_Position =  MVP * rotateMatrix * vec4(pos, 1.0);
}

