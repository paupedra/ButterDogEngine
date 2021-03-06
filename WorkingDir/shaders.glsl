///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef TEXTURED_GEOMETRY

struct Light
{
	unsigned int type;
	vec3 color;
	vec3 direction;
	vec3 position;
};


layout(binding = 0,std140) uniform GlobalParams //Same for all game Objects
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[10];
};

#if defined(VERTEX) ///////////////////////////////////////////////////

// TODO: Write your vertex shader here

layout(location=0) in vec3 aPosition;
layout(location=2) in vec2 aTexCoord;

layout(binding = 1,std140) uniform LocalParams //Per game Object
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;
out vec3 vPosition;
//out vec3 vNormal;
//out vec3 vViewDir;

void main()
{
	vTexCoord = aTexCoord;
	vPosition = vec3(uWorldMatrix * vec4(aPosition,1.0));
	//vNormal = vec3(uWorldMatrix * vec4(aNormal,0.0));

	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition,1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

// TODO: Write your fragment shader here

in vec2 vTexCoord;
in vec3 vPosition;
//in vec3 vNormal;
//in vec3 vViewDir;

uniform sampler2D uTexture;

layout(location=0) out vec4 oColor;

void main()
{
	oColor = texture(uTexture,vTexCoord);
	oColor += vec4(uLight[0].color,1.0);
}

#endif
#endif


// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.





#ifdef DRAW_FRAMEBUFFER

#if defined(VERTEX) ///////////////////////////////////////////////////

// TODO: Write your vertex shader here

layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;
	gl_Position = vec4(aPosition,1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

// TODO: Write your fragment shader here

in vec2 vTexCoord;

uniform sampler2D uTexture;

layout(location=0) out vec4 oColor;

void main()
{
	oColor = texture(uTexture,vTexCoord);
}

#endif
#endif