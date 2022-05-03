//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#include "platform.h"
#include <glad/glad.h>

typedef glm::vec2  vec2;
typedef glm::vec3  vec3;
typedef glm::vec4  vec4;
typedef glm::ivec2 ivec2;
typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;

enum LightType
{
    LIGHT_DIRECTIONAL,
    LIGHT_POINT
};

struct Light 
{
    LightType type;
    vec3 color;
    vec3 direction;
    vec3 position;
};

struct Buffer
{
    GLuint handle;
    GLenum type;
    u32 size;
    u32 head;
    void* data; // Mapped data
};

struct Vao
{
    GLuint handle;
    GLuint programHandle;
};

struct VertexBufferAttribute
{
    u8 location;
    u8 componentCount;
    u8 offset;
};

struct VertexBufferLayout
{
    std::vector<VertexBufferAttribute> attributes;
    u8 stride;
};

struct VertexShaderAttribute
{
    u8 location;
    u8 componentCount;
};

struct VertexShaderLayout
{
    std::vector<VertexShaderAttribute> attributes;
};


//OpenGL info (retrieved in the initialization)
struct OpenGLInfo
{
    std::string glVersion;
    std::string glRenderer;
    std::string glVendor;
    std::string glShadingLenguageVersion;
    i16 numExtension;
    std::vector<std::string> glExtensions;

};

//Resources

struct Image
{
    void* pixels;
    ivec2 size;
    i32   nchannels;
    i32   stride;
};

struct Texture
{
    GLuint      handle;
    std::string filepath;
};

struct Material
{
    std::string name;
    vec3        albedo;
    vec3        emissive;
    f32         smoothness;
    u32         albedoTextureIdx;
    u32         emissiveTextureIdx;
    u32         specularTextureIdx;
    u32         normalsTextureIdx;
    u32         bumpTextureIdx;
};



struct Submesh
{
    VertexBufferLayout vertexBufferLayout;
    std::vector<float> vertices;
    std::vector<u32>   indices;
    u32                vertexOffset;
    u32                indexOffset;

    std::vector<Vao>   vaos;
};

struct Mesh
{
    std::vector<Submesh> submeshes;
    GLuint               vertexBufferHandle;
    GLuint               indexBufferHandle;
};

struct Model
{
    u32 meshIdx;
    std::vector<u32> materialIdx;
};



struct Program
{
    GLuint             handle;
    std::string        filepath;
    std::string        programName;
    u64                lastWriteTimestamp; // What is this for?

    VertexShaderLayout vertexInputLayout;
};

enum Mode
{
    Mode_TexturedQuad,
    Mode_TexturedMesh,
    Mode_Count
};

struct VertexV3V2
{
    glm::vec3 pos;
    glm::vec2 uv;
};

glm::mat4 TransformScale(const vec3& scaleFactors);

glm::mat4 TransformPositionScale(const vec3& pos, const vec3& scaleFactor);

struct Transform
{
    glm::mat4 matrix;
    vec3 position;
};

struct Camera
{
    vec3 position;

    vec3 target;
};

struct GameObject
{
    std::string name;
    Transform transform; //(World matrix)
    GLuint bufferHandle;
    u32 blockOffset;
};

struct Light
{
    //unsigned int type;
    vec3 color;
    vec3 direction;
    vec3 position;
};

struct App
{
    GameObject gameObjects[50];

    int activeGameObjects=0;

    //OpenGL info for output purposes
    OpenGLInfo openGLInfo;

    // Loop
    f32  deltaTime;
    f32  runTime;
    bool isRunning;

    // Input
    Input input;

    // Graphics
    char gpuName[64];
    char openGlVersion[64];

    ivec2 displaySize;

    GLuint bufferHandle; //Hold the number of the Uniform Buffer (It is used before rendering on glBindBufferRange)

    Camera camera;

    bool displayRotate = false; //Mode that makes the camera rotate to display models

    float cameraSpeed = 20.f; //Movement of the camera with WASD
    float cameraSprintSpeed = 40.f; //Movement of the camera with WASD when pressing SHIFT

    float aspectRatio;
    float zNear = 0.1f;
    float zFar = 1000.0f;

    glm::mat4 view; //Matrix defining "where the camera is" (Camera Coordinates)
    glm::mat4 projection; //Clip coordinates

    glm::mat4 world; //The model matrix (?) (Should this be on the model?)
    glm::mat4 worldViewProjection;

    std::vector<Texture>  textures;
    std::vector<Material> materials;
    std::vector<Mesh>     meshes;
    std::vector<Model>    models;
    std::vector<Program>  programs;

    // program indices
    u32 texturedGeometryProgramIdx;

    u32 texturedMeshProgramIdx;
    
    // texture indices
    u32 diceTexIdx;
    u32 whiteTexIdx;
    u32 blackTexIdx;
    u32 normalTexIdx;
    u32 magentaTexIdx;

    u32 patrickTexIdx;

    // Mode
    Mode mode;

    // Embedded geometry (in-editor simple meshes such as
    // a screen filling quad, a cube, a sphere...)
    GLuint embeddedVertices;
    GLuint embeddedElements;

    // Location of the texture uniform in the textured quad shader
    GLuint programUniformTexture;

    GLint maxUniformBufferSize, uniformBlockAlignment;

    // VAO object to link our screen filling quad with our textured quad shader
    GLuint vao;
};

void Init(App* app);

void InitQuad(App* app);

void Gui(App* app);

void UpdateInput(App* app);

void Update(App* app);

void Render(App* app);

u32 LoadTexture2D(App* app, const char* filepath);

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program);
