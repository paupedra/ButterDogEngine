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

    for (int i = 0; i < numExtensions; ++i)
    {   
        app->openGLInfo.glExtensions.push_back((const char*)glGetStringi(GL_EXTENSIONS, GLuint(i)));
    }

    //initiate view matrix

    app->camera.target = vec3(0.0f);
    app->camera.position = vec3(1.0f, 1.0f, 1.0f);

    app->aspectRatio = (float)app->displaySize.x/(float)app->displaySize.y;
    app->zNear = 0.1f;
    app->zFar = 1000.0f;

    app->projection = glm::perspective(glm::radians(60.0f), app->aspectRatio, app->zNear, app->zFar);
    app->view = lookAt(app->camera.position,app->camera.target,vec3(0,1.f,0));

    app->world = TransformPositionScale(vec3(2.5f,1.5f,-2.0f),vec3(0.45f)); //arbitrary position of the model, later should take th entitie's position
    app->worldViewProjection = app->projection * app->view * app->world;

    // TODO: Initialize your resources here!
    // - vertex buffers
    // - element/index buffers
    // - vaos
    // - programs (and retrieve uniform indices)
    // - textures
    //Temporary vertices and indices for drawing Quad

    
   

    app->mode = Mode::Mode_TexturedMesh; //Define what mode of draw we use

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

            GLint maxUniformBufferSize,uniformBlockAlignment;
            glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBufferSize);
            glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniformBlockAlignment);

            GLuint bufferHandle;
            glGenBuffers(1, &bufferHandle);
            glBindBuffer(GL_UNIFORM_BUFFER, bufferHandle);
            glBufferData(GL_UNIFORM_BUFFER, maxUniformBufferSize, NULL, GL_STREAM_DRAW);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);

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
        {glm::vec3(-0.5,-0.5,0.0),glm::vec2(0.0,0.0)},
        {glm::vec3(0.5,-0.5,0.0),glm::vec2(1.0,0.0) },
        {glm::vec3(0.5,0.5,0.0),glm::vec2(1.0,1.0) },
        {glm::vec3(-0.5,0.5,0.0),glm::vec2(0.0,1.0) },
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
    glGenVertexArrays(1, &app->vao);
    glBindVertexArray(app->vao);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)12);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    glBindVertexArray(0);

    app->texturedGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "TEXTURED_GEOMETRY");

    Program& texturedGeometryProgram = app->programs[app->texturedGeometryProgramIdx];

    app->programUniformTexture = glGetUniformLocation(texturedGeometryProgram.handle, "uTexture"); //This right here does wacky stuff

    if (app->programUniformTexture == GL_INVALID_VALUE || app->programUniformTexture == GL_INVALID_OPERATION)
    {
        //log("Fucky stuff wit da program texture");
        ILOG("ProgramUniformTexture loaded incorrectly");
    }
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
}

void Update(App* app)
{
    // You can handle app->input keyboard/mouse here

    glBindBuffer(GL_UNIFORM_BUFFER, app->bufferHandle);
    //copy all entitie's matrices (matrix uniform in shader)
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

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glViewport(0, 0, app->displaySize.x, app->displaySize.y);

            Program& programTextureGeometry = app->programs[app->texturedGeometryProgramIdx];
            glUseProgram(programTextureGeometry.handle);
            glBindVertexArray(app->vao);

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
            u32 blockOffset = 0;
            u32 blockSize = sizeof(glm::mat4) * 2;
            glBindBufferRange(GL_UNIFORM_BUFFER, 1, app->bufferHandle, blockOffset, blockSize);

            Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
            glUseProgram(texturedMeshProgram.handle);

            Model& model = app->models[0];
            Mesh& mesh = app->meshes[model.meshIdx];

            for (u32 i = 0; i < mesh.submeshes.size();++i)
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

            break;
        }

        default:;
    }
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


/* //create the vertex format
            VertexBufferLayout vertexBufferLayout = {};
            vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 0,3,0 });
            vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 2,2,3 * sizeof(float) });
            vertexBufferLayout.stride = 5 * sizeof(float);

            //add the submesh into the mesh
            Submesh submesh = {};
            submesh.vertexBufferLayout = vertexBufferLayout;
            submesh.vertices.swap(vertices);
            submesh.indices.swap(indices);
            myMesh->submeshes.push_back(submesh);*/