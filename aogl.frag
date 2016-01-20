#version 410 core

#define POSITION	0
#define NORMAL		1
#define TEXCOORD	2
#define FRAG_COLOR	0

#define PI 3.14159265359

precision highp int;

uniform float Time;
uniform sampler2D Diffuse;
uniform sampler2D Specular;

uniform vec3 PointLightPosition;
uniform float PointLightIntensity;
uniform vec3 PointLightColor;

uniform vec3 PointLightPosition2;
uniform float PointLightIntensity2;
uniform vec3 PointLightColor2;

uniform vec3 DirectionalLightDirection;
uniform float DirectionalLightIntensity;
uniform vec3 DirectionalLightColor;

uniform vec3 SpotLightPosition;
uniform vec3 SpotLightDirection;
uniform float SpotLightIntensity;
uniform float SpotLightAngle;
uniform float SpotLightFallOffAngle;
uniform vec3 SpotLightColor;

uniform vec3 Camera;
uniform int SpecularPower;
uniform int NbPointLights;

uniform struct Light {
   vec4 _position;
   vec3 _color;
   float _intensity; 
} PointLights;


// layout(location = FRAG_COLOR, index = 0) out vec3 FragColor;

// Write in GL_COLOR_ATTACHMENT0
layout(location = 0 ) out vec4 Color;
// Write in GL_COLOR_ATTACHMENT1
layout(location = 1) out vec4 Normal;

in block
{
	vec2 TexCoord;
	vec3 Position; 
	vec3 Normal;
} In;


vec3 pointLight(){

	vec3 n = normalize(In.Normal);
	vec3 l = normalize(PointLightPosition - In.Position);
	vec3 v = normalize(Camera - In.Position);
	vec3 h = normalize(l + v);
	float ndoth = clamp(dot(n, h), 0.0, 1.0);
	float ndotl = clamp(dot(n, l), 0.0, 1.0);
	float d = distance(PointLightPosition, In.Position);
	float a = 1 / d * d; // attenuation

	vec3 diffuseColor = texture(Diffuse, In.TexCoord).rgb;
	vec3 specularColor = texture(Specular, In.TexCoord).rgb;
	
	vec3 color = a * PointLightColor * PointLightIntensity * PI * ( (diffuseColor / PI ) + ( ( pow(ndoth,SpecularPower) * specularColor ) / ( (SpecularPower+8) / (8*PI) ) ) ) * ndotl ;
	// vec3 color =  a * PointLightColor * PointLightIntensity * (diffuseColor * ndotl + specularColor * pow(ndoth, SpecularPower));

	return color;
}

vec3 pointLight2(){

	vec3 n = normalize(In.Normal);
	vec3 l = normalize(PointLightPosition2 - In.Position);
	vec3 v = normalize(Camera - In.Position);
	vec3 h = normalize(l + v);
	float ndoth = clamp(dot(n, h), 0.0, 1.0);
	float ndotl = clamp(dot(n, l), 0.0, 1.0);
	float d = distance(PointLightPosition2, In.Position);
	float a = 1 / d * d; // attenuation

	vec3 diffuseColor = texture(Diffuse, In.TexCoord).rgb;
	vec3 specularColor = texture(Specular, In.TexCoord).rgb;
	
	vec3 color = a * PointLightColor2 * PointLightIntensity2 * PI * ( (diffuseColor / PI ) + ( ( pow(ndoth,SpecularPower) * specularColor ) / ( (SpecularPower+8) / (8*PI) ) ) ) * ndotl ;
	// vec3 color =  a * PointLightColor2 * PointLightIntensity2 * (diffuseColor * ndotl + specularColor * pow(ndoth, SpecularPower));

	return color;
}

vec3 directionalLight(){

	vec3 n = normalize(In.Normal);
	vec3 l = normalize(-DirectionalLightDirection);
	vec3 v = normalize(Camera - In.Position);
	vec3 h = normalize(l + v);
	float ndoth = clamp(dot(n, h), 0.0, 1.0);
	float ndotl = clamp(dot(n, l), 0.0, 1.0);

	vec3 diffuseColor = texture(Diffuse, In.TexCoord).rgb;
	vec3 specularColor = texture(Specular, In.TexCoord).rgb;
	
	vec3 color = DirectionalLightColor * DirectionalLightIntensity * PI * ( (diffuseColor / PI ) + ( ( pow(ndoth,SpecularPower) *specularColor ) / ( (SpecularPower+8) / (8*PI) ) ) ) * ndotl ;
	// vec3 color =  DirectionalLightColor * DirectionalLightIntensity * (diffuseColor * ndotl + specularColor * pow(ndoth, SpecularPower));

	return color;
}

vec3 spotLight(){


	vec3 color = vec3(0.0,0.0,0.0);
	vec3 n = normalize(In.Normal);
	vec3 l = normalize(SpotLightPosition - In.Position);
	vec3 v = normalize(Camera - In.Position);
	vec3 h = normalize(l + v);
	float ndoth = clamp(dot(n, h), 0.0, 1.0);
	float ndotl = clamp(dot(n, l), 0.0, 1.0);
	float ldotld = max(dot(-l, normalize(SpotLightDirection)), 0.0);
	float d = distance(SpotLightPosition, In.Position);
	float a = 1 / d * d; // attenuation

	float theta = ldotld ;
	float phy = radians(SpotLightAngle);
	float pPhy = radians(SpotLightFallOffAngle);

	float falloff = clamp( pow( (theta-cos(phy)) / (cos(phy) - cos(pPhy)) , 4 ), 0.0, 1.0); // coeff d'attenuation angulaire
	// falloff = 1.;
	 // falloff = 1 - falloff;
	vec3 diffuseColor = texture(Diffuse, In.TexCoord).rgb;
	vec3 specularColor = texture(Specular, In.TexCoord).rgb;

	 // if(ldotld > cos( radians(SpotLightAngle)/2 ))
	 color = a * falloff * SpotLightColor * SpotLightIntensity * PI * ( (diffuseColor / PI ) + ( ( pow(ndoth,SpecularPower)*specularColor ) / ( (SpecularPower+8) / (8*PI) ) ) ) * ndotl ;
	 // color = a * SpotLightColor * SpotLightIntensity * PI * ( (diffuseColor / PI ) + ( ( pow(ndoth,SpecularPower)*specularColor ) / ( (SpecularPower+8) / (8*PI) ) ) ) * ndotl ;

	return color;
}

vec3 computeIllumination(){

	// return directionalLight() ;
	return  pointLight() + pointLight2() + directionalLight() + spotLight() ;

}

void main()
{
 
	// TD LIGHTING FORWARD SHADING
	// vec3 color = computeIllumination();
	// color = mix(texture(Diffuse, In.TexCoord).rgb, texture(Specular, In.TexCoord).xyz, 0.8) + directionalLight();
	// color = texture(Diffuse, In.TexCoord).rgb;
	// color = vec3(gl_FragCoord.z,  gl_FragCoord.z , gl_FragCoord.z );
	// FragColor = color;

	// TD LIGHTING DEFFERED SHADING
	vec4 color = vec4(texture(Diffuse, In.TexCoord).rgb, texture(Diffuse, In.TexCoord).r);
	Color = color;
	
	vec4 normal = vec4(In.Normal, SpecularPower);
	Normal = normal;


	// // EX
	// FragColor = vec4(1.0, 0.0, 1.0, 1.0);
	// FragColor = vec4(abs(In.Normal.x), abs(In.Normal.y), abs(In.Normal.z), 1.0);
	// FragColor = vec4(abs(In.Position.x), abs(In.Position.y), abs(In.Position.z), 1.0);

	// // EX2
	// float x = sin(In.TexCoord.x*30) + cos(30*In.TexCoord.y) * sin(Time);
	// FragColor = vec4(x, 0.0, 0.0, 1.0) + vec4(0.0, In.TexCoord.y, In.TexCoord.x, 1.0);

	// // EX3
	// vec3 color = mix(texture(Diffuse, In.TexCoord).rgb, texture(Specular, In.TexCoord).xyz, 0.8); ;
	// FragColor = vec4(color, 1.0);

	// // EX4
	// vec3 diffuseColor = texture(Diffuse, In.TexCoord).rgb;
	// vec3 specularColor = texture(Specular, In.TexCoord).rgb;
	// vec3 n = normalize(In.Normal);
	// vec3 l = normalize(LightPosition - In.Position);
	// float ndotl =  clamp(dot(n, l), 0.0, 1.0);
	// vec3 v = normalize(Camera - In.Position);
	// vec3 h = normalize(l+v);
	// float ndoth = clamp(dot(n, h), 0.0, 1.0);
	// // vec3 color = diffuseColor * (0.20+ndotl) + specularColor * pow(ndoth, 0);
	// vec3 color = diffuseColor * (0.10+ndotl) + specularColor * pow(ndoth, SpecularPower);


}	


