#version 410 core

#define PI 3.14159265359

in block
{
    vec2 Texcoord;
} In; 

uniform sampler2D ColorBuffer;
uniform sampler2D NormalBuffer;
uniform sampler2D DepthBuffer;

uniform mat4 ScreenToWorld;
uniform mat4 InvMV;
uniform vec3 CameraPosition;

uniform vec3 PointLightPosition;
uniform vec3 PointLightColor;
uniform float PointLightIntensity;

uniform float Time;
uniform float CounterPointLight;

layout(location = 0) out vec4 Color;
 

vec3 pointLight(vec3 n, vec3 p, vec3 diffuseColor, vec3 specularColor, float specularPower, float linear, float quadratic){

	vec3 l = normalize(PointLightPosition - p);
	vec3 v = normalize(CameraPosition-p);
	vec3 h = normalize(l + v);
	float ndoth = clamp(dot(n, h), 0.0, 1.0);
	float ndotl = clamp(dot(n, l), 0.0, 1.0);
	float d = distance(PointLightPosition, p);
	float a = 1 / d * d; // attenuation

	float attenuation = 1.0 / (1.0 + linear * d + quadratic * d * d);
	vec3 color = attenuation * PointLightColor * PointLightIntensity * PI * ( (diffuseColor / PI ) + ( ( pow(ndoth,specularPower) * specularColor ) / ( (specularPower+8) / (8*PI) ) ) ) * ndotl ;

	return color;
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

vec3 decodeStereographicProjection(vec3 enc){
    float scale = 1.7777;
    vec3 nn =
        enc.xyz*vec3(2*scale,2*scale,0) +
        vec3(-scale,-scale,1);
    float g = 2.0 / dot(nn.xyz,nn.xyz);
    vec3 n;
    n.xy = g*nn.xy;
    n.z = g-1;
    return n;
}

vec3 decodeSphereMapTransform(vec3 enc)
{
    vec2 fenc = enc.xy*4-2;
    float f = dot(fenc,fenc);
    float g = sqrt(1-f/4);
    vec3 n;
    n.xy = fenc*g;
    n.z = 1-f/2;
    return n;
}

void main(void)
{
	float w = Time;
	// w = 0;
	mat4 rotateMatrix = rotationMatrix(vec3(0.,1.,0.) , w);

	// Read gbuffer values
	vec4 colorBuffer = texture(ColorBuffer, In.Texcoord).rgba;
	vec4 normalBuffer = texture(NormalBuffer, In.Texcoord).rgba;
	float depth = texture(DepthBuffer, In.Texcoord).r;

	// Unpack values stored in the gbuffer
	vec3 diffuseColor = colorBuffer.rgb;
	vec3 specularColor = colorBuffer.aaa;
	float specularPower = normalBuffer.a * 100;
	vec3 n = normalBuffer.rgb;

	n = decodeStereographicProjection(n);
    n = ( InvMV * vec4(n, 0.0) ).xyz;

	// Convert texture coordinates into screen space coordinates
	vec2 xy = In.Texcoord * 2.0 -1.0;
	// Convert depth to -1,1 range and multiply the point by ScreenToWorld matrix
	vec4 wP = ScreenToWorld * vec4(xy, depth * 2.0 -1.0, 1.0) ;
	
	// Divide by w and rotate
	vec3 p = ( rotateMatrix * vec4(wP.xyz / wP.w, 1.0) ).xyz;

    float linear = 1.7;
    float quadratic = 0.5;
 	float maxBrightness = max(max(colorBuffer.r, colorBuffer.g), colorBuffer.b);
    float radius = (-linear + sqrt(linear * linear - 4 * quadratic * (1.0 - (256.0 / 5.0) * maxBrightness))) / (2 * quadratic);
	// Calculate distance between light source and current fragment
    float dist = length(PointLightPosition - p);
    vec3 color = vec3(0,0,0);
    radius+=-0.8f;
	if(dist < radius)
		color =  pointLight(n, p, diffuseColor, specularColor, specularPower, linear, quadratic);
    // Color = vec4(1.0, 0.0, 1.0, 1.0);
     // color += vec3(0.2,0, 0);

    Color = vec4(color, 1.0);
}
