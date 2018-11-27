#include <stdio.h>
#include <stdlib.h>

#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>

// #include "GLES2/gl2.h"
// #include "GLES2/gl2ext.h"
// #include "EGL/egl.h"
// #include "FSL/fsl_egl.h"
// #include "FSL/fslutil.h"

static const GLchar* vertex_shader_source =
"uniform   mat4 g_matModelView;				\n"
"uniform   mat4 g_matProj;					\n"
"								\n"
"attribute vec4 g_vPosition;				\n"
//"attribute vec3 g_vColor;					\n"
"attribute vec2 g_vTexCoord;				\n"
"								\n"
//"varying   vec3 g_vVSColor;					\n"
"varying   vec2 g_vVSTexCoord;				\n"
"								\n"
"void main()						\n"
"{								\n"
"    vec4 vPositionES = g_vPosition;	\n"
"    gl_Position  = vPositionES;		\n"
//"    g_vVSColor = g_vColor;					\n"
"    g_vVSTexCoord = g_vTexCoord;				\n"
"}								\n";

static const GLchar* fragment_shader_source =
    // "#version 100\n"
    "#ifdef GL_FRAGMENT_PRECISION_HIGH				\n"
    "   precision highp float;					\n"
    "#else							\n"
    "   precision mediump float;				\n"
    "#endif							\n"
    "uniform sampler2D s_texture;"
    "varying   vec2 g_vVSTexCoord;				\n"
    "void main() {\n"
    // "   gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
    "   gl_FragColor = texture2D(s_texture,g_vVSTexCoord);\n"
    "}\n";
    
GLuint g_hVertexLoc = 0;
GLuint g_hVertexTexLoc = 2;
GLuint texture[1];
static const GLfloat vertices[] = {
    -1.0f, 1.0f, 0.0f, // 左上角
    -1.0f, -1.0f, 0.0f, // 左下角
    1.0f, 1.0f, 0.0f, // 右上角
    1.0f, -1.0f, 0.0f	// 右下角
};

float VertexTexCoords[] =
{
    0.0f,0.0f,
    0.0f,1.0f,
    1.0f,0.0f,
    1.0f,1.0f,
};

GLint common_get_shader_program(const char *vertex_shader_source, const char *fragment_shader_source) {
    enum Consts {INFOLOG_LEN = 512};
    GLchar infoLog[INFOLOG_LEN];
    GLint fragment_shader;
    GLint shader_program;
    GLint success;
    GLint vertex_shader;

    /* Vertex shader */
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex_shader, INFOLOG_LEN, NULL, infoLog);
        printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n", infoLog);
    }

    /* Fragment shader */
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment_shader, INFOLOG_LEN, NULL, infoLog);
        printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", infoLog);
    }

    /* Link shaders */
    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);

    glBindAttribLocation( shader_program, g_hVertexLoc,   "g_vPosition" );
    glBindAttribLocation( shader_program, g_hVertexTexLoc,   "g_vTexCoord" );

    glLinkProgram(shader_program);
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_program, INFOLOG_LEN, NULL, infoLog);
        printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return shader_program;
}

void draw(float w, float h) {
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, w, h);
    
    unsigned char texDat[10000];
    int i;
    for (i = 0; i < 10000; ++i)
        texDat[i] = ((i + (i / 8)) % 2) * 128 + 127;
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 8, 8, 0, GL_RGB, GL_UNSIGNED_BYTE, texDat);
    glBindTexture(GL_TEXTURE_2D, 0);

    glVertexAttribPointer( g_hVertexLoc, 3, GL_FLOAT, 0, 0, vertices );
	glEnableVertexAttribArray( g_hVertexLoc );
    glVertexAttribPointer( g_hVertexTexLoc, 2, GL_FLOAT, 0, 0, VertexTexCoords );
	glEnableVertexAttribArray( g_hVertexTexLoc );

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture[0]);
    glDrawArrays( GL_TRIANGLE_STRIP, 0, sizeof(vertices) );

    glDisableVertexAttribArray( g_hVertexLoc );
    glDisableVertexAttribArray( g_hVertexTexLoc );

}

int main(void) {
    GLuint shader_program, vbo;
    GLint pos;
    GLFWwindow* window;

    if (!glfwInit())
        return -1;
    
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    // glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    // window = glfwCreateWindow(WIDTH, HEIGHT, __FILE__, NULL, NULL);

    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(mode->width, mode->height, "My Title", primary, NULL); // current context
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    printf("GL_VERSION  : %s\n", glGetString(GL_VERSION) );
    printf("GL_RENDERER : %s\n", glGetString(GL_RENDERER) );

    shader_program = common_get_shader_program(vertex_shader_source, fragment_shader_source);
    pos = glGetAttribLocation(shader_program, "position");

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glViewport(0, 0, mode->width, mode->height);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    glEnableVertexAttribArray(pos);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(shader_program);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        // texture
        draw(mode->width, mode->height);

        glfwSwapBuffers(window);
    }
    glDeleteBuffers(1, &vbo);
    glfwTerminate();
    return EXIT_SUCCESS;
}