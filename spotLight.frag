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
uniform vec3 CameraPosition;

uniform vec3 SpotLightPosition;
uniform vec3 SpotLightDirection;
uniform float SpotLightIntensity;
uniform float SpotLightAngle;
uniform float SpotLightFallOffAngle;
uniform vec3 SpotLightColor;


layout(location = 0) out vec4 Color;
 

vec3 spotLight(vec3 n, vec3 p, vec3 diffuseColor, vec3 specularColor, float specularPower){

	vec3 color = vec3(0.0,0.0,0.0);
	vec3 l = normalize(SpotLightPosition - p);
	vec3 v = normalize(CameraPosition - p);
	vec3 h = normalize(l + v);
	float ndoth = clamp(dot(n, h), 0.0, 1.0);
	float ndotl = clamp(dot(n, l), 0.0, 1.0);
	float ldotld = max(dot(-l, normalize(SpotLightDirection)), 0.0);
	float d = distance(SpotLightPosition, p);
	float a = 1 / d * d; // attenuation

	float theta = ldotld ;
	float phy = radians(SpotLightAngle);
	float pPhy = radians(SpotLightFallOffAngle);

	float falloff = clamp( pow( (theta-cos(phy)) / (cos(phy) - cos(pPhy)) , 4 ), 0.0, 1.0); // coeff d'attenuation angulaire
	// falloff = 1.;
	 // falloff = 1 - falloff;

	 // if(ldotld > cos( radians(SpotLightAngle)/2 ))
	 color = a * falloff * SpotLightColor * SpotLightIntensity * PI * ( (diffuseColor / PI ) + ( ( pow(ndoth,specularPower)*specularColor ) / ( (specularPower+8) / (8*PI) ) ) ) * ndotl ;
	 // color = a * SpotLightColor * SpotLightIntensity * PI * ( (diffuseColor / PI ) + ( ( pow(ndoth,SpecularPower)*specularColor ) / ( (SpecularPower+8) / (8*PI) ) ) ) * ndotl ;

	return color;
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
	float specularPower = normalBuffer.a;
	vec3 n = normalBuffer.rgb;

	// Convert texture coordinates into screen space coordinates
	vec2 xy = In.Texcoord * 2.0 -1.0;
	// Convert depth to -1,1 range and multiply the point by ScreenToWorld matrix
	vec4 wP = vec4(xy, depth * 2.0 -1.0, 1.0) * ScreenToWorld;
	// Divide by w
	vec3 p = vec3(wP.xyz / wP.w);

	vec3 color =  spotLight(n, p, diffuseColor, specularColor, specularPower);
    // Color = vec4(1.0, 0.0, 1.0, 1.0);
    Color = vec4(color, 1.0);
}