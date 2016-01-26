#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include <vector>

#include <cmath>

#include "glew/glew.h"

#include "GLFW/glfw3.h"
#include "stb/stb_image.h"
#include "imgui/imgui.h"
#include "imgui/imguiRenderGL3.h"

#include "glm/glm.hpp"
#include "glm/vec3.hpp" // glm::vec3
#include "glm/vec4.hpp" // glm::vec4, glm::ivec4
#include "glm/mat4x4.hpp" // glm::mat4
#include "glm/gtc/matrix_transform.hpp" // glm::translate, glm::rotate, glm::scale, glm::perspective
#include "glm/gtc/type_ptr.hpp" // glm::value_ptr

#ifndef DEBUG_PRINT
#define DEBUG_PRINT 1
#endif

#if DEBUG_PRINT == 0
#define debug_print(FORMAT, ...) ((void)0)
#else
#ifdef _MSC_VER
#define debug_print(FORMAT, ...) \
    fprintf(stderr, "%s() in %s, line %i: " FORMAT "\n", \
        __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#else
#define debug_print(FORMAT, ...) \
    fprintf(stderr, "%s() in %s, line %i: " FORMAT "\n", \
        __func__, __FILE__, __LINE__, __VA_ARGS__)
#endif
#endif

// Font buffers
extern const unsigned char DroidSans_ttf[];
extern const unsigned int DroidSans_ttf_len;    



// Shader utils
int check_link_error(GLuint program);
int check_compile_error(GLuint shader, const char ** sourceBuffer);
GLuint compile_shader(GLenum shaderType, const char * sourceBuffer, int bufferSize);
GLuint compile_shader_from_file(GLenum shaderType, const char * fileName);

// OpenGL utils
bool checkError(const char* title);

struct Camera
{
    float radius;
    float theta;
    float phi;
    glm::vec3 o;
    glm::vec3 eye;
    glm::vec3 up;
};
void camera_defaults(Camera & c);
void camera_zoom(Camera & c, float factor);
void camera_turn(Camera & c, float phi, float theta);
void camera_pan(Camera & c, float x, float y);

struct GUIStates
{
    bool panLock;
    bool turnLock;
    bool zoomLock;
    int lockPositionX;
    int lockPositionY;
    int camera;
    double time;
    bool playing;
    static const float MOUSE_PAN_SPEED;
    static const float MOUSE_ZOOM_SPEED;
    static const float MOUSE_TURN_SPEED;
};
const float GUIStates::MOUSE_PAN_SPEED = 0.001f;
const float GUIStates::MOUSE_ZOOM_SPEED = 0.05f;
const float GUIStates::MOUSE_TURN_SPEED = 0.005f;
void init_gui_states(GUIStates & guiStates);

glm::mat4 rotationMatrix(glm::vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;

    return glm::mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}

int main( int argc, char **argv )
{
    int width = 1024*1.6, height= 768*1.2;
    float widthf = (float) width, heightf = (float) height;
    double t;
    float fps = 0.f;

    // Initialise GLFW
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        exit( EXIT_FAILURE );
    }
    glfwInit();
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GL_TRUE);
    glfwWindowHint(GLFW_DECORATED, GL_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    int const DPI = 2; // For retina screens only
#else
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    int const DPI = 1;
# endif

    // Open a window and create its OpenGL context
    GLFWwindow * window = glfwCreateWindow(width/DPI, height/DPI, "aogl", 0, 0);
    if( ! window )
    {
        fprintf( stderr, "Failed to open GLFW window\n" );
        glfwTerminate();
        exit( EXIT_FAILURE );
    }
    glfwMakeContextCurrent(window);

    // Init glew
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
          /* Problem: glewInit failed, something is seriously wrong. */
          fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
          exit( EXIT_FAILURE );
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode( window, GLFW_STICKY_KEYS, GL_TRUE );

    // Enable vertical sync (on cards that support it)
    glfwSwapInterval( 1 );
    GLenum glerr = GL_NO_ERROR;
    glerr = glGetError();

    if (!imguiRenderGLInit(DroidSans_ttf, DroidSans_ttf_len))
    {
        fprintf(stderr, "Could not init GUI renderer.\n");
        exit(EXIT_FAILURE);
    }


    // Try to load and compile shaders
    GLuint vertShaderId = compile_shader_from_file(GL_VERTEX_SHADER, "aogl.vert");
    GLuint fragShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "aogl.frag");
    // GLuint geomShaderId = compile_shader_from_file(GL_GEOMETRY_SHADER, "aogl.geom");
    GLuint programObject = glCreateProgram();
    glAttachShader(programObject, vertShaderId);
    glAttachShader(programObject, fragShaderId);
    // glAttachShader(programObject, geomShaderId);    
    glLinkProgram(programObject);
    if (check_link_error(programObject) < 0)
        exit(1);

    // Try to load and compile blit shaders
    GLuint blitVertShaderId = compile_shader_from_file(GL_VERTEX_SHADER, "blit.vert");
    GLuint blitFragShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "blit.frag");
    GLuint blitProgramObject = glCreateProgram();
    glAttachShader(blitProgramObject, blitVertShaderId);
    glAttachShader(blitProgramObject, blitFragShaderId);
    glLinkProgram(blitProgramObject);
    if (check_link_error(blitProgramObject) < 0)
        exit(1);

    // Try to load and compile point light shaders
    GLuint pointLightFragShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "pointLight.frag");
    GLuint pointLightProgramObject = glCreateProgram();
    glAttachShader(pointLightProgramObject, blitVertShaderId);
    glAttachShader(pointLightProgramObject, pointLightFragShaderId);
    glLinkProgram(pointLightProgramObject);
    if (check_link_error(pointLightProgramObject) < 0)
        exit(1);

    // Try to load and compile directional light shaders
    GLuint directionalLightFragShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "directionalLight.frag");
    GLuint directionalLightProgramObject = glCreateProgram();
    glAttachShader(directionalLightProgramObject, blitVertShaderId);
    glAttachShader(directionalLightProgramObject, directionalLightFragShaderId);
    glLinkProgram(directionalLightProgramObject);
    if (check_link_error(directionalLightProgramObject) < 0)
        exit(1);

    // Try to load and compile spot light shaders
    GLuint spotLightFragShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "spotLight.frag");
    GLuint spotLightProgramObject = glCreateProgram();
    glAttachShader(spotLightProgramObject, blitVertShaderId);
    glAttachShader(spotLightProgramObject, spotLightFragShaderId);
    glLinkProgram(spotLightProgramObject);
    if (check_link_error(spotLightProgramObject) < 0)
        exit(1);

    
    // Upload uniforms /     // Initialize uniform location

                // General Program Uniform
    GLuint mvpLocation = glGetUniformLocation(programObject, "MVP");
    GLuint mvLocation = glGetUniformLocation(programObject, "MV");
    GLuint mLocation = glGetUniformLocation(programObject, "M");
    GLuint timeLocation = glGetUniformLocation(programObject, "Time");
    GLuint diffuseLocation = glGetUniformLocation(programObject, "Diffuse");
    GLuint specularLocation = glGetUniformLocation(programObject, "Specular");

    GLuint instanceCountLocation = glGetUniformLocation(programObject, "InstanceCount");
    GLuint cameraLocation = glGetUniformLocation(programObject, "Camera");
    GLuint cameraPositionLocation = glGetUniformLocation(programObject, "CameraPosition");
    GLuint specularPowerLocation = glGetUniformLocation(programObject, "SpecularPower");
    GLuint NbPointLightsLocation = glGetUniformLocation(programObject, "NbPointLights");
    GLuint CounterCubeLocation = glGetUniformLocation(programObject, "CounterCube");
    GLuint CounterPlaneLocation = glGetUniformLocation(programObject, "CounterPlane");
    glProgramUniform1i(programObject, diffuseLocation, 0);
    glProgramUniform1i(programObject, specularLocation, 1);

                // Blit Program Uniform
    GLuint invMvLocation = glGetUniformLocation(blitProgramObject, "InvMV");
    GLuint blitTextureLocation = glGetUniformLocation(blitProgramObject, "Texture");
    glProgramUniform1i(blitProgramObject, blitTextureLocation, 0);

                // Point Light Program Uniform
    GLuint pointLightScreenToWorldLocation = glGetUniformLocation(pointLightProgramObject, "ScreenToWorld");
    GLuint pointlightColorBufferLocation = glGetUniformLocation(pointLightProgramObject, "ColorBuffer");
    GLuint pointlightNormalLocation = glGetUniformLocation(pointLightProgramObject, "NormalBuffer");
    GLuint pointlightDepthLocation = glGetUniformLocation(pointLightProgramObject, "DepthBuffer");
    GLuint pointLightCameraPositionLocation = glGetUniformLocation(pointLightProgramObject, "CameraPosition");
    GLuint pointLightPositionLocation = glGetUniformLocation(pointLightProgramObject, "PointLightPosition");
    GLuint pointLightIntensityLocation = glGetUniformLocation(pointLightProgramObject, "PointLightIntensity");
    GLuint pointlightColorLocation = glGetUniformLocation(pointLightProgramObject, "PointLightColor");
    GLuint pointlightTimeLocation = glGetUniformLocation(pointLightProgramObject, "Time");
    GLuint pointlightCounterLocation = glGetUniformLocation(pointLightProgramObject, "CounterPointLight");
    GLuint pointLightInvMvLocation = glGetUniformLocation(pointLightProgramObject, "InvMV");
    glProgramUniform1i(pointLightProgramObject, pointlightColorBufferLocation, 0);
    glProgramUniform1i(pointLightProgramObject, pointlightNormalLocation, 1);
    glProgramUniform1i(pointLightProgramObject, pointlightDepthLocation, 2);

                // Directional Light Program Uniform
    GLuint directionalLightScreenToWorldLocation = glGetUniformLocation(directionalLightProgramObject, "ScreenToWorld");
    GLuint directionalLightColorBufferLocation = glGetUniformLocation(directionalLightProgramObject, "ColorBuffer");
    GLuint directionalLightNormalLocation = glGetUniformLocation(directionalLightProgramObject, "NormalBuffer");
    GLuint directionalLightDepthLocation = glGetUniformLocation(directionalLightProgramObject, "DepthBuffer");
    GLuint directionalLightCameraPositionLocation = glGetUniformLocation(directionalLightProgramObject, "CameraPosition");
    GLuint directionalLightDirectionLocation = glGetUniformLocation(directionalLightProgramObject, "DirectionalLightDirection");
    GLuint directionalLightIntensityLocation = glGetUniformLocation(directionalLightProgramObject, "DirectionalLightIntensity");
    GLuint directionalLightColorLocation = glGetUniformLocation(directionalLightProgramObject, "DirectionalLightColor");
    GLuint directionalLightInvMvLocation = glGetUniformLocation(directionalLightProgramObject, "InvMV");
    glProgramUniform1i(directionalLightProgramObject, directionalLightColorBufferLocation, 0);
    glProgramUniform1i(directionalLightProgramObject, directionalLightNormalLocation, 1);
    glProgramUniform1i(directionalLightProgramObject, directionalLightDepthLocation, 2);

                // Spot Light Program Uniform
    GLuint spotLightScreenToWorldLocation = glGetUniformLocation(spotLightProgramObject, "ScreenToWorld");
    GLuint spotLightColorBufferLocation = glGetUniformLocation(spotLightProgramObject, "ColorBuffer");
    GLuint spotLightNormalLocation = glGetUniformLocation(spotLightProgramObject, "NormalBuffer");
    GLuint spotLightDepthLocation = glGetUniformLocation(spotLightProgramObject, "DepthBuffer");
    GLuint spotLightCameraPositionLocation = glGetUniformLocation(spotLightProgramObject, "CameraPosition");
    GLuint spotLightPositionLocation = glGetUniformLocation(spotLightProgramObject, "SpotLightPosition");
    GLuint spotLightDirectionLocation = glGetUniformLocation(spotLightProgramObject, "SpotLightDirection");
    GLuint spotLightIntensityLocation = glGetUniformLocation(spotLightProgramObject, "SpotLightIntensity");
    GLuint spotLightAngleLocation = glGetUniformLocation(spotLightProgramObject, "SpotLightAngle");
    GLuint spotLightFallOffAngleLocation = glGetUniformLocation(spotLightProgramObject, "SpotLightFallOffAngle");
    GLuint spotLightColorLocation = glGetUniformLocation(spotLightProgramObject, "SpotLightColor");
    GLuint spotLightInvMvLocation = glGetUniformLocation(spotLightProgramObject, "InvMV");
    glProgramUniform1i(spotLightProgramObject, spotLightColorBufferLocation, 0);
    glProgramUniform1i(spotLightProgramObject, spotLightNormalLocation, 1);
    glProgramUniform1i(spotLightProgramObject, spotLightDepthLocation, 2);

    if (!checkError("Uniforms"))
        exit(1);


    // Load textures
    int x, y, comp;
    unsigned char * diffuse = stbi_load("textures/spnza_bricks_a_diff.tga", &x, &y, &comp, 3);
    unsigned char * specular = stbi_load("textures/spnza_bricks_a_spec.tga", &x, &y, &comp, 3);
    unsigned char * bump = stbi_load("textures/spnza_bricks_a_bump.png", &x, &y, &comp, 3);

    GLuint textures[3];
    glGenTextures(3, textures);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, diffuse);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, specular);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, textures[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, bump);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    checkError("Texture Initialization");


    // Framebuffer object handle
    GLuint gbufferFbo;
    // Texture handles
    GLuint gbufferTextures[3];
    glGenTextures(3, gbufferTextures);
    // 2 draw buffers for color and normal
    GLuint gbufferDrawBuffers[2];

    // Create color texture
    glBindTexture(GL_TEXTURE_2D, gbufferTextures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create normal texture
    glBindTexture(GL_TEXTURE_2D, gbufferTextures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create depth texture
    glBindTexture(GL_TEXTURE_2D, gbufferTextures[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create Framebuffer Object
    glGenFramebuffers(1, &gbufferFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gbufferFbo);

    // Initialize DrawBuffers
    gbufferDrawBuffers[0] = GL_COLOR_ATTACHMENT0;
    gbufferDrawBuffers[1] = GL_COLOR_ATTACHMENT1;
    glDrawBuffers(2, gbufferDrawBuffers);

    // Attach textures to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, gbufferTextures[0], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1 , GL_TEXTURE_2D, gbufferTextures[1], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gbufferTextures[2], 0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        fprintf(stderr, "Error on building framebuffer\n");
        exit( EXIT_FAILURE );
    }



    // Init viewer structures
    Camera camera;
    camera_defaults(camera);
    GUIStates guiStates;
    init_gui_states(guiStates);
    float dummySlider = 0.f;

    float counterCube = 16.0;
    float counterPlane = 100.0;

    // Init objects geometry
    int cube_triangleCount = 12;
    int cube_triangleList[] = {0, 1, 2, 2, 1, 3, 4, 5, 6, 6, 5, 7, 8, 9, 10, 10, 9, 11, 12, 13, 14, 14, 13, 15, 16, 17, 18, 19, 17, 20, 21, 22, 23, 24, 25, 26, };
    float cube_uvs[] = {0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f,  1.f, 0.f,  1.f, 1.f,  0.f, 1.f,  1.f, 1.f,  0.f, 0.f, 0.f, 0.f, 1.f, 1.f,  1.f, 0.f,  };
    float cube_vertices[] = {-0.5, -0.5, 0.5, 0.5, -0.5, 0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, -0.5, -0.5, 0.5, -0.5, -0.5, -0.5, -0.5, -0.5, 0.5, -0.5, -0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, 0.5, -0.5, 0.5, -0.5, -0.5, 0.5, -0.5, -0.5, -0.5, 0.5, -0.5, 0.5, 0.5 };
    float cube_normals[] = {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, }; 

    int plane_triangleCount = 2;
    int plane_triangleList[] = {0, 1, 2, 2, 1, 3}; 
    float plane_uvs[] = {0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f};
    float plane_vertices[] = {-5.0, -1.0, 5.0, 5.0, -1.0, 5.0, -5.0, -1.0, -5.0, 5.0, -1.0, -5.0};
    float plane_normals[] = {0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0};

    int quad_triangleCount = 2;
    int quad_triangleList[] = {0, 1, 2, 2, 1, 3}; 
    float quad_vertices[] =  {-1.0, -1.0, 1.0, -1.0, -1.0, 1.0, 1.0, 1.0};




    // Init LIGHTS
    float _specularPower = 12.0;

    struct PointLight{
        glm::vec4 _position;
        glm::vec3 _color;
        float _intensity;
    };
    float nbPointLights = 1.f;
    int counterCircle = 0; 

    // PointLight pl1, pl2;
    // int nbPointLights = 2; 

    // pl1._position = glm::vec4(7.0, 0.9, 12.0, 1.0); // Point Light Position
    // pl1._color = glm::vec3(0.1,0.5,0.1); // Point Light Color
    // pl1._intensity = 0.95; // Point Light Intensity
            
    // pl2._position = glm::vec4(11.0, 0.9, 1.0, 1.0); // Point Light 2 Position
    // pl2._color = glm::vec3(0.1,0.1,0.95); // Point Light 2 Color
    // pl2._intensity = 0.95; // Point Light 2 Intensity

    // PointLight pointLights[nbPointLights];
    // pointLights[0] = pl1;
    // pointLights[1] = pl2;


    struct DirectionalLight{
        glm::vec4 _direction;
        glm::vec3 _color;
        float _intensity;
    };

    int nbDirectionalLights = 1;

    DirectionalLight dl1;
    // dl1._direction = glm::vec4(-1.0, -1.0, -1.0, 0.0);
    dl1._direction = glm::vec4(-1.0, -1.0, -1.0, 0.0);
    dl1._color = glm::vec3(0.9, 0.9, 0.9);
    dl1._intensity = 0.2;

    DirectionalLight directionalLights[nbDirectionalLights];
    directionalLights[0] = dl1;


    struct SpotLight{
        glm::vec4 _position;
        glm::vec4 _direction;
        glm::vec3 _color;
        float _angle;
        float _falloffangle;
        float _intensity;
    };

    int nbSpotLights = 1;

    SpotLight sl1 = {
        glm::vec4(3.0, 4.0, 5.0, 1.0),
        glm::vec4(-1.0, -1.0, -1.0, 1.0),
        glm::vec3(0.9, 0.1, 0.6),
        50.0,
        65.0,
        0.01
    };

    SpotLight spotLights[nbDirectionalLights];
    spotLights[0] = sl1;


    // Create a Vertex Array Object
    GLuint vao[3];
    glGenVertexArrays(3, vao);

    // Create a VBO for each array
    GLuint vbo[10];
    glGenBuffers(10, vbo);

    // Bind the VAO 0 FOR THE CUBES
    glBindVertexArray(vao[0]);

    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_triangleList), cube_triangleList, GL_STATIC_DRAW);

    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glEnableVertexAttribArray(0); 
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

    // Bind normals and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_normals), cube_normals, GL_STATIC_DRAW);

    // Bind uv coords and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_uvs), cube_uvs, GL_STATIC_DRAW);

    // Unbind everything
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Bind the VAO 1 FOR THE PLANE
    glBindVertexArray(vao[1]);

    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[4]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(plane_triangleList), plane_triangleList, GL_STATIC_DRAW);

    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[5]);
    glEnableVertexAttribArray(0); 
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_vertices), plane_vertices, GL_STATIC_DRAW);

    // Bind normals and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[6]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_normals), plane_normals, GL_STATIC_DRAW);

    // Bind uv coords and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[7]);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_uvs), plane_uvs, GL_STATIC_DRAW);

    // Unbind everything
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // BIND THE VAO 2 FOR THE QUAD
    glBindVertexArray(vao[2]);
    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[8]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_triangleList), quad_triangleList, GL_STATIC_DRAW);
    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[9]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    // Unbind everything
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


    // Viewport 
    glViewport( 0, 0, width, height  );

    do
    {

        t = glfwGetTime();


        // Mouse states
        int leftButton = glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_LEFT );
        int rightButton = glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_RIGHT );
        int middleButton = glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_MIDDLE );

        if( leftButton == GLFW_PRESS )
            guiStates.turnLock = true;
        else
            guiStates.turnLock = false;

        if( rightButton == GLFW_PRESS )
            guiStates.zoomLock = true;
        else
            guiStates.zoomLock = false;

        if( middleButton == GLFW_PRESS )
            guiStates.panLock = true;
        else
            guiStates.panLock = false;

        // Camera movements
        int altPressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT);
        if (!altPressed && (leftButton == GLFW_PRESS || rightButton == GLFW_PRESS || middleButton == GLFW_PRESS))
        {
            double x; double y;
            glfwGetCursorPos(window, &x, &y);
            guiStates.lockPositionX = x;
            guiStates.lockPositionY = y;
        }
        if (altPressed == GLFW_PRESS)
        {
            double mousex; double mousey;
            glfwGetCursorPos(window, &mousex, &mousey);
            int diffLockPositionX = mousex - guiStates.lockPositionX;
            int diffLockPositionY = mousey - guiStates.lockPositionY;
            if (guiStates.zoomLock)
            {
                float zoomDir = 0.0;
                if (diffLockPositionX > 0)
                    zoomDir = -1.f;
                else if (diffLockPositionX < 0 )
                    zoomDir = 1.f;
                camera_zoom(camera, zoomDir * GUIStates::MOUSE_ZOOM_SPEED);
            }
            else if (guiStates.turnLock)
            {
                camera_turn(camera, diffLockPositionY * GUIStates::MOUSE_TURN_SPEED,
                            diffLockPositionX * GUIStates::MOUSE_TURN_SPEED);

            }
            else if (guiStates.panLock)
            {
                camera_pan(camera, diffLockPositionX * GUIStates::MOUSE_PAN_SPEED,
                            diffLockPositionY * GUIStates::MOUSE_PAN_SPEED);
            }
            guiStates.lockPositionX = mousex;
            guiStates.lockPositionY = mousey;
        }

        // Default states
        glEnable(GL_DEPTH_TEST);
        // Clear the front buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Viewport 
        glViewport( 0, 0, width, height);
        // Bind gbuffer
        glBindFramebuffer(GL_FRAMEBUFFER, gbufferFbo);
        // Clear the gbuffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



        // Get camera matrices
        glm::mat4 projection = glm::perspective(45.0f, widthf / heightf, 0.1f, 100.f); 
        glm::mat4 worldToView = glm::lookAt(camera.eye, camera.o, camera.up);
        glm::mat4 objectToWorld;
        glm::mat4 mv = worldToView * objectToWorld;
        glm::mat4 invMv = glm::inverse(mv);
        glm::mat4 mvp = projection * mv;
        // Compute the inverse worldToScreen matrix
        glm::mat4 screenToWorld = glm::inverse(mvp); // invMVP


        // Select shader
        glUseProgram(programObject);

            // Upload uniforms
            glProgramUniformMatrix4fv(programObject, mvpLocation, 1, 0, glm::value_ptr(mvp));
            glProgramUniformMatrix4fv(programObject, mvLocation, 1, 0, glm::value_ptr(mv));
            glProgramUniformMatrix4fv(programObject, mLocation, 1, 0, glm::value_ptr(worldToView));
            glProgramUniform1f(programObject, timeLocation, t);
            glProgramUniform1i(programObject, diffuseLocation, 0);
            glProgramUniform1i(programObject, specularLocation, 1);
            glProgramUniform3fv(programObject, cameraLocation, 1, glm::value_ptr(camera.eye));
            glProgramUniform3fv(programObject, cameraPositionLocation, 1, glm::value_ptr(camera.eye));
            glProgramUniform1f(programObject, specularPowerLocation, _specularPower);      
            glProgramUniform1f(programObject, CounterCubeLocation, counterCube);
            glProgramUniform1f(programObject, CounterPlaneLocation, counterPlane);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures[0]);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, textures[1]);


            glBindVertexArray(vao[0]); // CUBES    
            // glBindTexture(GL_TEXTURE_2D, textures[2]);
            glDrawElementsInstanced(GL_TRIANGLES, cube_triangleCount * 3, GL_UNSIGNED_INT, (void*)0, counterCube);
     
            //update uniforms for plans
            glProgramUniform1f(programObject, timeLocation, 0.0);
            // glProgramUniform1i(programObject, diffuseLocation, 1);

            glBindVertexArray(vao[1]); // PLANES
            // glDrawElements(GL_TRIANGLES, plane_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
            glDrawElementsInstanced(GL_TRIANGLES, plane_triangleCount * 3, GL_UNSIGNED_INT, (void*)0, counterPlane);


        glBindFramebuffer(GL_FRAMEBUFFER, 0);        // Unbind the framebuffer



        glDisable(GL_DEPTH_TEST);        // Disable the depth test
        glEnable(GL_BLEND);        // Enable blending
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_ONE, GL_ONE);        // Setup additive blending

        glViewport( 0, 0, width, height );        // Set a full screen viewport

        // Select textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[1]);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[2]);

        // Bind quad vao - lights vao
        glBindVertexArray(vao[2]);

        // Use the deffered pointLight program
        glUseProgram(pointLightProgramObject);

            // int rayons[] = {5,15,25,35,45,55,65,75,85,95,105, 115, 125};
            int fd = 10; // first decal
            int nbLightsByCircle[] = {6, 12, 18, 24, 30, 36, 42, 48, 54, 60, 66, 72, 78};
            int rayon = 5;
            int counterCircle = 0; 
            srand (time(NULL));
            for(int i = 0; i < nbPointLights; ++i){

                PointLight p;

                if( i == nbLightsByCircle[counterCircle] ){
                  counterCircle++;
                  rayon += 6; 
                } 
                float coeff = rayon;
                float w = t + t;
                w = 0;
                p._position = glm::vec4( 
                    fd+ coeff * cos(i+w*2* M_PI /nbPointLights)  
                    ,0.5
                    ,fd+ coeff * sin(i+w*2* M_PI /nbPointLights) 
                    ,1.0);

                float red = fmaxf(sin(i)+cos(counterCircle), 0.2);
                float green = fmaxf(cos(i), 0.2);
                float blue = fmaxf(sin(counterCircle)+cos(counterCircle), 0.2);
                
                if(red<0.4 && green <0.4 && blue < 0.4){
                    int r = rand() % 3 + 1;
                    if(r==1) blue +=0.5; else if(r==2) green +=0.5; else red +=0.5; 
                } 

                p._color = glm::vec3( red , green , blue);
                // p._color = glm::vec3(0.5,0.5,0.95);

                p._intensity = 0.8;

                glProgramUniformMatrix4fv(pointLightProgramObject, pointLightScreenToWorldLocation, 1, 0, glm::value_ptr(screenToWorld));
                glProgramUniformMatrix4fv(pointLightProgramObject, pointLightInvMvLocation, 1, 0, glm::value_ptr(invMv));
                glProgramUniform1f(pointLightProgramObject, pointLightIntensityLocation, p._intensity);
                glProgramUniform3fv(pointLightProgramObject, pointLightPositionLocation, 1, glm::value_ptr(glm::vec3(p._position) / p._position.w));
                glProgramUniform3fv(pointLightProgramObject, pointlightColorLocation, 1, glm::value_ptr(glm::vec3(p._color)));
                glProgramUniform3fv(pointLightProgramObject, pointLightCameraPositionLocation, 1, glm::value_ptr(camera.eye));
                glProgramUniform1f(pointLightProgramObject, pointlightTimeLocation, t);
                glProgramUniform1f(pointLightProgramObject, pointlightCounterLocation, (int)nbPointLights);
              

                // changer taille des quads selon influence de la light
                float n = 4.0;
                float x = 0.001;
                float dx = std::pow( (1/x) , 1/n);
                dx /= 2;
                // dx = 0.1;
                float linear = 1.7;
                float quadratic = 0.5;
                float maxBrightness = std::max(std::max(p._color.r, p._color.g), p._color.b);
                float radius = (-linear + std::sqrt(linear * linear - 4 * quadratic * (1.0 - (256.0 / 5.0) * maxBrightness))) / (2 * quadratic);
                dx = radius/2;
                // std::cout << "dx = " << dx << std::endl;

                    //list de 8 points du cube
                    //on projete ces points sur le plan Screen (MVM*)
                    // on prend le plus haut et le plus bas et le plus gauche et le plus droite
                    // on construit un quad avec ces coords
                    // et biim

                float px = p._position.x;
                float py = p._position.y;
                // py = 0.01;
                float pz = p._position.z;
                std::vector<glm::vec3> cube;

                cube.push_back(glm::vec3(px-dx,py-dx,pz-dx)); // left bot back
                cube.push_back(glm::vec3(px+dx,py-dx,pz-dx)); // right bot back
                cube.push_back(glm::vec3(px-dx,py-dx,pz+dx)); // left bot front
                cube.push_back(glm::vec3(px+dx,py-dx,pz+dx)); // right bot front
                cube.push_back(glm::vec3(px+dx,py+dx,pz-dx)); // right top back
                cube.push_back(glm::vec3(px-dx,py+dx,pz-dx)); // left top back
                cube.push_back(glm::vec3(px-dx,py+dx,pz+dx)); // left top front
                cube.push_back(glm::vec3(px+dx,py+dx,pz+dx)); // right top front
            
                float wt = t;
                glm::mat4 rotateMatrix = rotationMatrix(glm::vec3(0.,-1.,0.) , wt);

                glm::vec4 projInitPoint = mvp * rotateMatrix * glm::vec4(cube[0], 1.0);
                projInitPoint /= projInitPoint.w;
                glm::vec2 mostLeft(glm::vec2(projInitPoint.x,projInitPoint.y));
                glm::vec2 mostRight(glm::vec2(projInitPoint.x,projInitPoint.y));
                glm::vec2 mostTop(glm::vec2(projInitPoint.x,projInitPoint.y));
                glm::vec2 mostBottom(glm::vec2(projInitPoint.x,projInitPoint.y));

                for(int k=1; k<8; ++k){
                    glm::vec4 projPoint = mvp * rotateMatrix *  glm::vec4(cube[k], 1.0);
                    projPoint /= projPoint.w;
                    if( projPoint.x < mostLeft.x) mostLeft = glm::vec2(projPoint.x, projPoint.y);
                    if( projPoint.y > mostTop.y) mostTop = glm::vec2(projPoint.x, projPoint.y);
                    if( projPoint.x > mostRight.x) mostRight = glm::vec2(projPoint.x, projPoint.y);
                    if( projPoint.y < mostBottom.y) mostBottom = glm::vec2(projPoint.x, projPoint.y);

                // std::cout << projPoint.x << " " << projPoint.y << " " << projPoint.z << " " << projPoint.w << std::endl;

                }   

                // std::cout << "left: " << mostLeft.x << " right: " << mostRight.x << " top: " << mostTop.y << " bot: " << mostBottom.y << std::endl;

                float quad_light_vertices[] =  {mostLeft.x, mostBottom.y, mostRight.x, mostBottom.y,
                                                     mostLeft.x, mostTop.y, mostRight.x, mostTop.y};


                glBindBuffer(GL_ARRAY_BUFFER, vbo[9]);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
                glBufferData(GL_ARRAY_BUFFER, sizeof(quad_light_vertices), quad_light_vertices, GL_STATIC_DRAW);

                // Render quad
                glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
            }

                glBindBuffer(GL_ARRAY_BUFFER, vbo[9]);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
                glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);



        // Use the deffered directionalLight program
        glUseProgram(directionalLightProgramObject);

            for(int i = 0; i < nbDirectionalLights; ++i){
                glProgramUniformMatrix4fv(directionalLightProgramObject, directionalLightScreenToWorldLocation, 1, 0, glm::value_ptr(screenToWorld));
                glProgramUniformMatrix4fv(directionalLightProgramObject, directionalLightInvMvLocation, 1, 0, glm::value_ptr(invMv));
                glProgramUniform1f(directionalLightProgramObject, directionalLightIntensityLocation, directionalLights[i]._intensity);
                glProgramUniform3fv(directionalLightProgramObject, directionalLightDirectionLocation, 1, glm::value_ptr(glm::vec3(directionalLights[i]._direction) ));
                glProgramUniform3fv(directionalLightProgramObject, directionalLightColorLocation, 1, glm::value_ptr(glm::vec3(directionalLights[i]._color)));
                glProgramUniform3fv(directionalLightProgramObject, directionalLightCameraPositionLocation, 1, glm::value_ptr(camera.eye));
                
                // Render quad
                glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
            }    

        // Use the deffered spotLight program
        glUseProgram(spotLightProgramObject);

            for(int i = 0; i < nbSpotLights; ++i){
                glProgramUniformMatrix4fv(spotLightProgramObject, spotLightScreenToWorldLocation, 1, 0, glm::value_ptr(screenToWorld));
                glProgramUniformMatrix4fv(spotLightProgramObject, spotLightInvMvLocation, 1, 0, glm::value_ptr(invMv));
                glProgramUniform3fv(spotLightProgramObject, spotLightColorLocation, 1, glm::value_ptr(spotLights[i]._color));
                glProgramUniform1f(spotLightProgramObject, spotLightIntensityLocation, spotLights[i]._intensity);
                glProgramUniform3fv(spotLightProgramObject, spotLightPositionLocation, 1, glm::value_ptr(glm::vec3(spotLights[i]._position) ));
                glProgramUniform3fv(spotLightProgramObject, spotLightDirectionLocation, 1, glm::value_ptr(glm::vec3(spotLights[i]._direction) ));
                glProgramUniform1f(spotLightProgramObject, spotLightAngleLocation, spotLights[i]._angle);
                glProgramUniform1f(spotLightProgramObject, spotLightFallOffAngleLocation, spotLights[i]._falloffangle); 
                glProgramUniform3fv(spotLightProgramObject, spotLightCameraPositionLocation, 1, glm::value_ptr(camera.eye));
              
                // Render quad
                glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
            }         
            



        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);

        // Use the blit program
        glUseProgram(blitProgramObject);

            glProgramUniformMatrix4fv(blitProgramObject, invMvLocation, 1, 0, glm::value_ptr(invMv));


            // Bind quad VAO
            glBindVertexArray(vao[2]);

            // Viewport
            glViewport( 0, 0, width/3, height/4  );
            // Bind gbuffer color texture
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, gbufferTextures[0]);
            // Draw quad
            glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

            // Viewport 
            glViewport( width/3, 0, width/3, height/4  );
            // Bind gbuffer normal texture
            glBindTexture(GL_TEXTURE_2D, gbufferTextures[1]);
            // Draw quad
            glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

            // Viewport 
            glViewport( 2*width/3 , 0, width/3, height/4  );
            // Bind gbuffer depth texture
            glBindTexture(GL_TEXTURE_2D, gbufferTextures[2]);
            // Draw quad
            glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);





#if 1
        // Draw UI
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glViewport(0, 0, width, height);

        unsigned char mbut = 0;
        int mscroll = 0;
        double mousex; double mousey;
        glfwGetCursorPos(window, &mousex, &mousey);
        mousex*=DPI;
        mousey*=DPI;
        mousey = height - mousey;

        if( leftButton == GLFW_PRESS )
            mbut |= IMGUI_MBUT_LEFT;

        imguiBeginFrame(mousex, mousey, mbut, mscroll);
        int logScroll = 0;
        char lineBuffer[512];
        imguiBeginScrollArea("aogl", width - 210, height - 310, 200, 300, &logScroll);
        sprintf(lineBuffer, "FPS %f", fps);
        imguiLabel(lineBuffer);
        imguiSlider("Dummy", &dummySlider, 0.0, 3.0, 0.1);
        imguiSlider("Counter Cube", &counterCube, 0.0, 100.0, 1);
        imguiSlider("Counter Plane", &counterPlane, 0.0, 400.0, 1);
        imguiSlider("Counter Point Light", &nbPointLights, 0.0, 100.0, 1);
        imguiSlider("Specular Power", &_specularPower, 0.0, 100.0, 1.0);
        // imguiSlider("Point light n°1 Intensity", &pointLights[0]._intensity, 0.0, 1.0, 0.1);
        // imguiSlider("Point light n°2 Intensity", &pointLights[1]._intensity, 0.0, 1.0, 0.1);
        // imguiSlider("Point light n°1 Pos Y", &pointLights[0]._position.y, -10.0, 20.0, 0.5);
        imguiSlider("Directional light n°1 Intensity", &directionalLights[0]._intensity, 0.0, 1.0, 0.1);
        imguiSlider("Spot light n°1 Intensity", &spotLights[0]._intensity, 0.0, 1.0, 0.1);
        imguiSlider("Spot Light Angle", &spotLights[0]._angle, 0.0, 180.0, 0.1);
        imguiSlider("Spot Light FallOfAngle", &spotLights[0]._falloffangle, 0.0, 180.0, 0.1);



        imguiEndScrollArea();
        imguiEndFrame();
        imguiRenderGLDraw(width, height);

        glDisable(GL_BLEND);
#endif
        // Check for errors
        checkError("End loop");

        glfwSwapBuffers(window);
        glfwPollEvents();

        double newTime = glfwGetTime();
        fps = 1.f/ (newTime - t);
    } // Check if the ESC key was pressed
    while( glfwGetKey( window, GLFW_KEY_ESCAPE ) != GLFW_PRESS );

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    exit( EXIT_SUCCESS );
}

// No windows implementation of strsep
char * strsep_custom(char **stringp, const char *delim)
{
    register char *s;
    register const char *spanp;
    register int c, sc;
    char *tok;
    if ((s = *stringp) == NULL)
        return (NULL);
    for (tok = s; ; ) {
        c = *s++;
        spanp = delim;
        do {
            if ((sc = *spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                *stringp = s;
                return (tok);
            }
        } while (sc != 0);
    }
    return 0;
}

int check_compile_error(GLuint shader, const char ** sourceBuffer)
{
    // Get error log size and print it eventually
    int logLength;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 1)
    {
        char * log = new char[logLength];
        glGetShaderInfoLog(shader, logLength, &logLength, log);
        char *token, *string;
        string = strdup(sourceBuffer[0]);
        int lc = 0;
        while ((token = strsep_custom(&string, "\n")) != NULL) {
           printf("%3d : %s\n", lc, token);
           ++lc;
        }
        fprintf(stderr, "Compile : %s", log);
        delete[] log;
    }
    // If an error happend quit
    int status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
        return -1;     
    return 0;
}

int check_link_error(GLuint program)
{
    // Get link error log size and print it eventually
    int logLength;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 1)
    {
        char * log = new char[logLength];
        glGetProgramInfoLog(program, logLength, &logLength, log);
        fprintf(stderr, "Link : %s \n", log);
        delete[] log;
    }
    int status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);        
    if (status == GL_FALSE)
        return -1;
    return 0;
}


GLuint compile_shader(GLenum shaderType, const char * sourceBuffer, int bufferSize)
{
    GLuint shaderObject = glCreateShader(shaderType);
    const char * sc[1] = { sourceBuffer };
    glShaderSource(shaderObject, 
                   1, 
                   sc,
                   NULL);
    glCompileShader(shaderObject);
    check_compile_error(shaderObject, sc);
    return shaderObject;
}

GLuint compile_shader_from_file(GLenum shaderType, const char * path)
{
    FILE * shaderFileDesc = fopen( path, "rb" );
    if (!shaderFileDesc)
        return 0;
    fseek ( shaderFileDesc , 0 , SEEK_END );
    long fileSize = ftell ( shaderFileDesc );
    rewind ( shaderFileDesc );
    char * buffer = new char[fileSize + 1];
    fread( buffer, 1, fileSize, shaderFileDesc);
    buffer[fileSize] = '\0';
    GLuint shaderObject = compile_shader(shaderType, buffer, fileSize );
    delete[] buffer;
    return shaderObject;
}


bool checkError(const char* title)
{
    int error;
    if((error = glGetError()) != GL_NO_ERROR)
    {
        std::string errorString;
        switch(error)
        {
        case GL_INVALID_ENUM:
            errorString = "GL_INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            errorString = "GL_INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            errorString = "GL_INVALID_OPERATION";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            errorString = "GL_INVALID_FRAMEBUFFER_OPERATION";
            break;
        case GL_OUT_OF_MEMORY:
            errorString = "GL_OUT_OF_MEMORY";
            break;
        default:
            errorString = "UNKNOWN";
            break;
        }
        fprintf(stdout, "OpenGL Error(%s): %s\n", errorString.c_str(), title);
    }
    return error == GL_NO_ERROR;
}

void camera_compute(Camera & c)
{
    c.eye.x = cos(c.theta) * sin(c.phi) * c.radius + c.o.x;   
    c.eye.y = cos(c.phi) * c.radius + c.o.y ;
    c.eye.z = sin(c.theta) * sin(c.phi) * c.radius + c.o.z;   
    c.up = glm::vec3(0.f, c.phi < M_PI ?1.f:-1.f, 0.f);
}

void camera_defaults(Camera & c)
{
    c.phi = 3.14/2.f;
    c.theta = 3.14/2.f;
    c.radius = 10.f;
    camera_compute(c);
}

void camera_zoom(Camera & c, float factor)
{
    c.radius += factor * c.radius ;
    if (c.radius < 0.1)
    {
        c.radius = 10.f;
        c.o = c.eye + glm::normalize(c.o - c.eye) * c.radius;
    }
    camera_compute(c);
}

void camera_turn(Camera & c, float phi, float theta)
{
    c.theta += 1.f * theta;
    c.phi   -= 1.f * phi;
    if (c.phi >= (2 * M_PI) - 0.1 )
        c.phi = 0.00001;
    else if (c.phi <= 0 )
        c.phi = 2 * M_PI - 0.1;
    camera_compute(c);
}

void camera_pan(Camera & c, float x, float y)
{
    glm::vec3 up(0.f, c.phi < M_PI ?1.f:-1.f, 0.f);
    glm::vec3 fwd = glm::normalize(c.o - c.eye);
    glm::vec3 side = glm::normalize(glm::cross(fwd, up));
    c.up = glm::normalize(glm::cross(side, fwd));
    c.o[0] += up[0] * y * c.radius * 2;
    c.o[1] += up[1] * y * c.radius * 2;
    c.o[2] += up[2] * y * c.radius * 2;
    c.o[0] -= side[0] * x * c.radius * 2;
    c.o[1] -= side[1] * x * c.radius * 2;
    c.o[2] -= side[2] * x * c.radius * 2;       
    camera_compute(c);
}

void init_gui_states(GUIStates & guiStates)
{
    guiStates.panLock = false;
    guiStates.turnLock = false;
    guiStates.zoomLock = false;
    guiStates.lockPositionX = 0;
    guiStates.lockPositionY = 0;
    guiStates.camera = 0;
    guiStates.time = 0.0;
    guiStates.playing = false;
}


/*
Lesson 2 : Lighting:

1.
In general, a fragment can be thought of as the data needed to shade the pixel, 
plus the data needed to test whether the fragment survives to become a pixel 
(depth, alpha, stencil, scissor, window ID, etc.)

2.
For Z pre-pass, all opaque geometry is rendered in two passes. The first pass populates the Z buffer with depth values from all opaque geometry. A null pixel shader is used and the color buffer is not updated. For this first pass, only simple vertex shading is performed so unnecessary constant buffer updates and vertex-layout data should be avoided. For the second pass, the geometry is resubmitted with Z writes disabled but Z testing on and full vertex and pixel shading is performed. The graphics hardware takes advantage of Early-Z to avoid performing pixel shading on geometry that is not visible.

Performing a Z pre-pass can be a significant gain when per pixel costs are the dominant bottleneck and overdraw is significant. The increased draw call, vertex and Z related per pixel costs of the pre-pass may be such that this is not a suitable performance optimization in some scenarios.


3.




*/