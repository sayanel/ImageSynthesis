#version 410 core
in block
{
    vec2 Texcoord;
} In; 
uniform sampler2D Texture;
uniform mat4 InvMV;

layout(location = 0, index = 0) out vec4  Color;

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
    vec3 color = texture(Texture, In.Texcoord).rgb;

    vec3 normal = decodeStereographicProjection(color);
    normal = ( InvMV * vec4(normal, 0.0) ).xyz;
    Color = vec4(normal, 1.0);

    // Color = vec4(color, 1.0);
    // Color = vec4(InvMV[1]);
}