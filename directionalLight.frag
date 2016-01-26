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

uniform vec3 DirectionalLightDirection;
uniform float DirectionalLightIntensity;
uniform vec3 DirectionalLightColor;


layout(location = 0) out vec4 Color;
 

vec3 directionalLight(vec3 n, vec3 p, vec3 diffuseColor, vec3 specularColor, float specularPower){

	vec3 l = normalize(-DirectionalLightDirection);
	vec3 v = normalize(CameraPosition - p);
	vec3 h = normalize(l + v);
	float ndoth = clamp(dot(n, h), 0.0, 1.0);
	float ndotl = clamp(dot(n, l), 0.0, 1.0);
	
	// specularPower = 20;
	vec3 color = DirectionalLightColor * DirectionalLightIntensity * PI * ( (diffuseColor / PI ) + ( ( pow(ndoth,specularPower) *specularColor ) / ( (specularPower+8) / (8*PI) ) ) ) * ndotl ;
	color =   DirectionalLightColor * DirectionalLightIntensity * PI * ( (diffuseColor / PI )  ) * ndotl ;
	color +=  DirectionalLightColor * DirectionalLightIntensity * PI * ( ( pow(ndoth,specularPower)*specularColor ) ) ;

	return color;
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
	vec4 wP = ScreenToWorld * vec4(xy, depth * 2.0 -1.0, 1.0);
	// Divide by w
	vec3 p = vec3(wP.xyz / wP.w);

	vec3 color =  directionalLight(n, p, diffuseColor, specularColor, specularPower);
    // Color = vec4(1.0, 0.0, 1.0, 1.0);
    Color = vec4(color, 1.0);
}