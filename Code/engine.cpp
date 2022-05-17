//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "engine.h"
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include "assimp_model_loading.h"
#include "buffer_management.h"

GLuint CreateProgramFromSource(String programSource, const char* shaderName)
{
    GLchar  infoLogBuffer[1024] = {};
    GLsizei infoLogBufferSize = sizeof(infoLogBuffer);
    GLsizei infoLogSize;
    GLint   success;

    char versionString[] = "#version 430\n";
    char shaderNameDefine[128];
    sprintf(shaderNameDefine, "#define %s\n", shaderName);
    char vertexShaderDefine[] = "#define VERTEX\n";
    char fragmentShaderDefine[] = "#define FRAGMENT\n";

    const GLchar* vertexShaderSource[] = {
        versionString,
        shaderNameDefine,
        vertexShaderDefine,
        programSource.str
    };
    const GLint vertexShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(vertexShaderDefine),
        (GLint) programSource.len
    };
    const GLchar* fragmentShaderSource[] = {
        versionString,
        shaderNameDefine,
        fragmentShaderDefine,
        programSource.str
    };
    const GLint fragmentShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(fragmentShaderDefine),
        (GLint) programSource.len
    };

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, ARRAY_COUNT(vertexShaderSource), vertexShaderSource, vertexShaderLengths);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, ARRAY_COUNT(fragmentShaderSource), fragmentShaderSource, fragmentShaderLengths);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with fragment shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint programHandle = glCreateProgram();
    glAttachShader(programHandle, vshader);
    glAttachShader(programHandle, fshader);
    glLinkProgram(programHandle);
    glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programHandle, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glLinkProgram() failed with program %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    glUseProgram(0);

    glDetachShader(programHandle, vshader);
    glDetachShader(programHandle, fshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);

    return programHandle;
}

u32 LoadProgram(App* app, const char* filepath, const char* programName)
{
    String programSource = ReadTextFile(filepath);

    Program program = {};
    program.handle = CreateProgramFromSource(programSource, programName);
    program.filepath = filepath;
    program.programName = programName;
    program.lastWriteTimestamp = GetFileLastWriteTimestamp(filepath);
    app->programs.push_back(program);

    return app->programs.size() - 1;
}

Image LoadImage(const char* filename)
{
    Image img = {};
    stbi_set_flip_vertically_on_load(true);
    img.pixels = stbi_load(filename, &img.size.x, &img.size.y, &img.nchannels, 0);
    if (img.pixels)
    {
        img.stride = img.size.x * img.nchannels;
    }
    else
    {
        ELOG("Could not open file %s", filename);
    }
    return img;
}

void FreeImage(Image image)
{
    stbi_image_free(image.pixels);
}

GLuint CreateTexture2DFromImage(Image image)
{
    GLenum internalFormat = GL_RGB8;
    GLenum dataFormat     = GL_RGB;
    GLenum dataType       = GL_UNSIGNED_BYTE;

    switch (image.nchannels)
    {
        case 3: dataFormat = GL_RGB; internalFormat = GL_RGB8; break;
        case 4: dataFormat = GL_RGBA; internalFormat = GL_RGBA8; break;
        default: ELOG("LoadTexture2D() - Unsupported number of channels");
    }

    GLuint texHandle;
    glGenTextures(1, &texHandle);
    glBindTexture(GL_TEXTURE_2D, texHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.size.x, image.size.y, 0, dataFormat, dataType, image.pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texHandle;
}

u32 LoadTexture2D(App* app, const char* filepath)
{
    for (u32 texIdx = 0; texIdx < app->textures.size(); ++texIdx)
        if (app->textures[texIdx].filepath == filepath)
            return texIdx;

    Image image = LoadImage(filepath);

    if (image.pixels)
    {
        Texture tex = {};
        tex.handle = CreateTexture2DFromImage(image);
        tex.filepath = filepath;

        u32 texIdx = app->textures.size();
        app->textures.push_back(tex);

        FreeImage(image);
        return texIdx;
    }
    else
    {
        return UINT32_MAX;
    }
}

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program)
{
    Submesh& submesh = mesh.submeshes[submeshIndex];

    //Try finding a vao for this submesh/program
    for (u32 i = 0; i < (u32)submesh.vaos.size(); ++i)
        if (submesh.vaos[i].programHandle == program.handle)
            return submesh.vaos[i].handle;

    GLuint vaoHandle = 0;

    //Create a new vao for this submesh program
    glGenVertexArrays(1, &vaoHandle);
    glBindVertexArray(vaoHandle);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);

    //we have to link all vertex inputs attributes to attributes in the vertex buffer
    for (u32 i = 0; i < program.vertexInputLayout.attributes.size(); ++i)
    {
        bool attributeWasLinked = false;

        for (u32 j = 0; j < submesh.vertexBufferLayout.attributes.size(); ++j)
        {
            if (program.vertexInputLayout.attributes[i].location == submesh.vertexBufferLayout.attributes[j].location)
            {
                const u32 index = submesh.vertexBufferLayout.attributes[j].location;
                const u32 ncomp = submesh.vertexBufferLayout.attributes[j].componentCount;
                const u32 offset = submesh.vertexBufferLayout.attributes[j].offset + submesh.vertexOffset;;
                const u32 stride = submesh.vertexBufferLayout.stride;
                glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)(u64)offset);
                glEnableVertexAttribArray(index);

                attributeWasLinked = true;
                break;
            }
        }
        assert(attributeWasLinked); //The submesh should provide an attribute for each vertex inputs
    }

    glBindVertexArray(0);

    //Store it in the list of vaos for this submesh
    Vao vao{ vaoHandle,program.handle };
    submesh.vaos.push_back(vao);

    return vaoHandle;

}

void Init(App* app)
{
    //Get OpenGL info
    //const unsigned char* a = ;
    app->openGLInfo.glVersion = *glGetString(GL_VERSION);
    app->openGLInfo.glRenderer = *glGetString(GL_RENDERER);
    app->openGLInfo.glVendor = *glGetString(GL_VENDOR);
    app->openGLInfo.glShadingLenguageVersion = *glGetString(GL_SHADING_LANGUAGE_VERSION);

    GLint numExtensions;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

    glEnable(GL_DEPTH_TEST);

    for (int i = 0; i < numExtensions; ++i)
    {   
        app->openGLInfo.glExtensions.push_back((const char*)glGetStringi(GL_EXTENSIONS, GLuint(i)));
    }

    //initiate view matrix

    app->camera.target = vec3(0.0f);
    app->camera.position = vec3(20.0f, 20.0f, 20.0f);

    app->aspectRatio = (float)app->displaySize.x/(float)app->displaySize.y;
    app->zNear = 0.1f;
    app->zFar = 1000.0f;
     
    app->projection = glm::perspective(glm::radians(60.0f), app->aspectRatio, app->zNear, app->zFar);
   


    //This is per object
    app->world = TransformPositionScale(vec3(0.f,1.f,0.f),vec3(1.f)); //arbitrary position of the model, later should take th entitie's position
    app->worldViewProjection = app->projection * app->view * app->world;

    app->gameObjects[0].transform.matrix = TransformPositionScale(vec3(0.f, 1.f, 0.f), vec3(1.f));
    app->gameObjects[1].transform.matrix = TransformPositionScale(vec3(10.f, 0.f, 0.f), vec3(1.f));
    app->gameObjects[2].transform.matrix = TransformPositionScale(vec3(0.f, 0.f, 10.f), vec3(1.f));
    app->activeGameObjects = 3;

    AddLight(app, LIGHT_DIRECTIONAL, vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 0));
    AddLight(app, LIGHT_POINT, vec3(1, 0, 0), vec3(0, 1, 0), vec3(10, 10, 0));

    app->mode = Mode::Mode_TexturedMesh; //Define what mode of draw we use


    InitQuad(app);

    //Init FrameBuffer
    InitFramebuffer(app);

    

    switch (app->mode)
    {
        case Mode_TexturedQuad:
        {
            InitQuad(app);
            break;
        }
        case Mode_TexturedMesh:
        {
            app->patrickTexIdx = LoadModel(app, "Patrick/Patrick.obj");

            app->texturedMeshProgramIdx = LoadProgram(app, "shaders.glsl", "TEXTURED_GEOMETRY");
            Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
            
            int attributeCount = 0;
            glGetProgramiv(texturedMeshProgram.handle, GL_ACTIVE_ATTRIBUTES, &attributeCount);

            for (int i = 0; i < attributeCount; ++i)
            {
                GLchar attribName[100];
                int attribNameLength = 0, attribSize = 0;
                GLenum attribType;

                glGetActiveAttrib(texturedMeshProgram.handle, i, ARRAY_COUNT(attribName)
                ,&attribNameLength,&attribSize,&attribType,attribName);

                int attributeLocation = glGetAttribLocation(texturedMeshProgram.handle, attribName);

                texturedMeshProgram.vertexInputLayout.attributes.push_back({(u8)attributeLocation,(u8)attribSize});
            }

            //Uniforms initialization
            
            glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxUniformBufferSize);
            glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBlockAlignment);

            app->cbuffer = CreateConstantBuffer(app->maxUniformBufferSize);

            break;
        }
    }

    app->diceTexIdx = LoadTexture2D(app, "dice.png");
    app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
    app->blackTexIdx = LoadTexture2D(app, "color_black.png");
    app->normalTexIdx = LoadTexture2D(app, "color_normal.png");
    app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");

    
}

void InitQuad(App* app)
{
    //Init verts and indices to draw quad
    VertexV3V2 vertices[] = {
        {glm::vec3(-1.0,-1.0,0.0),glm::vec2(0.0,0.0)},
        {glm::vec3(1.0,-1.0,0.0),glm::vec2(1.0,0.0) },
        {glm::vec3(1.0,1.0,0.0),glm::vec2(1.0,1.0) },
        {glm::vec3(-1.0,1.0,0.0),glm::vec2(0.0,1.0) },
    };

    u16 indices[] = {
        0,1,2,
        0,2,3
    };

    //Geometry
    glGenBuffers(1, &app->embeddedVertices);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &app->embeddedElements);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    //Attribute state
    glGenVertexArrays(1, &app->vaoQuad);
    glBindVertexArray(app->vaoQuad);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)12);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    glBindVertexArray(0);

    
    app->drawFramebufferProgramIdx = LoadProgram(app, "shaders.glsl", "DRAW_FRAMEBUFFER");

    app->drawFramebufferProgram = app->programs[app->drawFramebufferProgramIdx];


    app->programUniformTexture = glGetUniformLocation(app->drawFramebufferProgram.handle, "uTexture"); //This right here does wacky stuff

    if (app->programUniformTexture == GL_INVALID_VALUE || app->programUniformTexture == GL_INVALID_OPERATION)
    {
        //log("Fucky stuff wit da program texture");
        ILOG("ProgramUniformTexture loaded incorrectly");
    }
}

void InitFramebuffer(App* app)
{
    app->colorAttachmentHandle;
    glGenTextures(1, &app->colorAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->colorAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,app->displaySize.x,app->displaySize.y,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    app->depthAttachmentHandle;
    glGenTextures(1, &app->depthAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->depthAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    app->framebufferHandle;
    glGenFramebuffers(1, &app->framebufferHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, app->framebufferHandle);
    glFramebufferTexture(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0, app->colorAttachmentHandle,0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, app->depthAttachmentHandle, 0);

    GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        switch (framebufferStatus)
        {
            case GL_FRAMEBUFFER_UNDEFINED: ELOG("GL_FRAMEBUFFER_UNDEFINED"); break;
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: ELOG("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: ELOG("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: ELOG("GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: ELOG("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
            case GL_FRAMEBUFFER_UNSUPPORTED: ELOG("GL_FRAMEBUFFER_UNSUPPORTED"); break;
            case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: ELOG("GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;
            case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: ELOG("GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); break;
            default: ELOG("Unknown framebuffer status error");
        }
    }

    glDrawBuffers(1, &app->colorAttachmentHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Gui(App* app)
{
    

    if (ImGui::BeginMainMenuBar())
    {

        if (ImGui::BeginMenu( std::to_string(1.0f / app->deltaTime).insert(0,"FPS: ").c_str()))
        {

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("GLInfo"))
        {

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    if (ImGui::Begin("Camera"))
    {
        if (ImGui::DragFloat("X", (float*)&app->camera.position.x, 0.25))
        {
            
        };
        if (ImGui::DragFloat("Y", (float*)&app->camera.position.y, 0.25))
        {

        };
        if (ImGui::DragFloat("Z", (float*)&app->camera.position.z, 0.25))
        {

        };
        if (ImGui::Checkbox("Display Rotate", &app->displayRotate));

        ImGui::End();
    }
}

void Update(App* app)
{
    UpdateInput(app);

    app->view = lookAt(app->camera.position, app->camera.target, vec3(0.f, 1.f, 0.f));

    //Buffer globalsBuffer = CreateBuffer(app->maxUniformBufferSize, GL_UNIFORM_BUFFER, GL_STREAM_DRAW);

    //BindBuffer(app->cbuffer);

    //MapBuffer(app->cbuffer, GL_WRITE_ONLY);

    //Set Up FrameBuffer Before Rendering
    glBindFramebuffer(GL_FRAMEBUFFER, app->framebufferHandle);

    GLuint drawBuffers[] = { app->colorAttachmentHandle };
    glDrawBuffers(ARRAY_COUNT(drawBuffers), drawBuffers);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    MapBuffer(app->cbuffer, GL_WRITE_ONLY);

    //Initialize global uniforms
    app->globalParamsOffset = app->cbuffer.head;

    PushVec3(app->cbuffer, app->camera.position);

    PushUInt(app->cbuffer, app->activeLights);

    for (u32 i=0; i< app->activeLights;++i)
    {
        AlignHead(app->cbuffer, sizeof(vec4));

        Light& light = app->lights[i];
        PushUInt(app->cbuffer, light.type);
        PushVec3(app->cbuffer, light.color);
        PushVec3(app->cbuffer, light.direction);
        PushVec3(app->cbuffer, light.position);
    }

    app->globalParamsSize = app->cbuffer.head - app->globalParamsOffset;


    for (int i = 0; i < app->activeGameObjects; ++i)
    {
        //bufferHead = Align(bufferHead, app->uniformBlockAlignment);

        AlignHead(app->cbuffer, app->uniformBlockAlignment);

        app->gameObjects[i].blockOffset = app->cbuffer.head;

        PushMat4(app->cbuffer, app->gameObjects[i].transform.matrix);
        PushMat4(app->cbuffer, app->projection * app->view * app->gameObjects[i].transform.matrix);
        
        //Here I could save the local params size if the uniforms of each entity needs to store any extra specific data

    }

    glUnmapBuffer(GL_UNIFORM_BUFFER);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    
}

void UpdateInput(App* app)
{
    float moveSpeed = app->cameraSpeed;

    if (app->input.keys[K_LSHIFT] == BUTTON_PRESSED)
    {
        moveSpeed = app->cameraSprintSpeed;
    }

    if (app->input.keys[K_W] == BUTTON_PRESSED)
    {
        app->camera.position.z += moveSpeed * app->deltaTime;
    }
    if (app->input.keys[K_S] == BUTTON_PRESSED)
    {
        app->camera.position.z -= moveSpeed * app->deltaTime;
    }

    if (app->input.keys[K_D] == BUTTON_PRESSED)
    {
        app->camera.position.x += moveSpeed * app->deltaTime;
    }

    if (app->input.keys[K_A] == BUTTON_PRESSED)
    {
        app->camera.position.x -= moveSpeed * app->deltaTime;
    }


    if (app->displayRotate)
    {
        float r = 20;
        float alpha;
        float beta;

        alpha = (app->runTime / 15.0) * 3.14 * 2.0;
        beta = 0.1 * 3.14 / 2.0;


        app->camera.position = r * vec3(cos(alpha) * cos(beta), sin(beta), sin(alpha) * cos(beta));
    }
    

}

void Render(App* app)
{
    switch (app->mode)
    {
        case Mode_TexturedQuad:
        {
            // TODO: Draw your textured quad here!
            // - clear the framebuffer
            // - set the viewport
            // - set the blending state
            // - bind the texture into unit 0
            // - bind the program 
            //   (...and make its texture sample from unit 0)
            // - bind the vao
            // - glDrawElements() !!!

            glViewport(0, 0, app->displaySize.x, app->displaySize.y);

            Program& programTextureGeometry = app->programs[app->texturedGeometryProgramIdx];
            glUseProgram(programTextureGeometry.handle);
            glBindVertexArray(app->vaoQuad);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glUniform1i(app->programUniformTexture, 0);
            glActiveTexture(GL_TEXTURE0);
            GLuint textureHandle = app->textures[app->diceTexIdx].handle;
            glBindTexture(GL_TEXTURE_2D, textureHandle);

            glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,0);

            glBindVertexArray(0);
            glUseProgram(0);

            break;
        }
        case Mode::Mode_TexturedMesh:
        {

            //Bind buffer range with binding 0 for global params (light)
            glBindBufferRange(GL_UNIFORM_BUFFER, 0, app->cbuffer.handle, app->globalParamsOffset, app->globalParamsSize);

            for (int i = 0; i < app->activeGameObjects; ++i)
            {

                u32 blockSize = sizeof(glm::mat4) * 2;
                glBindBufferRange(GL_UNIFORM_BUFFER, 1, app->cbuffer.handle, app->gameObjects[i].blockOffset, blockSize); //Here the offset should be saved for each entity defining where their information is stored in the uniform buffer
                

                Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
                glUseProgram(texturedMeshProgram.handle);

                Model& model = app->models[0];
                Mesh& mesh = app->meshes[model.meshIdx];

                for (u32 i = 0; i < mesh.submeshes.size(); ++i)
                {
                    GLuint vao = FindVAO(mesh, i, texturedMeshProgram);
                    glBindVertexArray(vao);

                    u32 submeshMaterialIdx = model.materialIdx[i];
                    Material& submeshMaterial = app->materials[submeshMaterialIdx];

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);
                    glUniform1i(0, 0); //Here missing a variable app->texturedMeshProgram_uTexture

                    Submesh& submesh = mesh.submeshes[i];
                    glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);

                }


            }
            break;
        }

        default:;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //glBindFramebuffer(0,NULL);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //Draw framebuffer to screen (Using a quad)
    glViewport(0, 0, app->displaySize.x, app->displaySize.y);

    glUseProgram(app->programs[app->drawFramebufferProgramIdx].handle);
    glBindVertexArray(app->vaoQuad);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUniform1i(app->programUniformTexture, 0);
    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_2D, app->colorAttachmentHandle);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

    glBindVertexArray(0);
    glUseProgram(0);
}

glm::mat4 TransformScale(const vec3& scaleFactors)
{
    glm::mat4 transform = scale(scaleFactors);
    return transform;
}

glm::mat4 TransformPositionScale(const vec3& pos, const vec3& scaleFactor)
{
    glm::mat4 transform = translate(pos);
    transform = scale(transform, scaleFactor);
    return transform;
}

Light AddLight(App* app, LightType type, vec3 color, vec3 direction, vec3 position)
{
    app->lights[app->activeLights].type = type;
    app->lights[app->activeLights].color = color;
    app->lights[app->activeLights].direction = direction;
    app->lights[app->activeLights].position = position;
    app->activeLights++;

    return app->lights[app->activeLights - 1];
}