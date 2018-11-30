#include <stdio.h>
#include <stdlib.h>
// #include <gst/app/gstappsink.h>

#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>

#include <gst/gst.h>
// #include "GLES2/gl2.h"
// #include "GLES2/gl2ext.h"
// #include "EGL/egl.h"
// #include "FSL/fsl_egl.h"
// #include "FSL/fslutil.h"

/** Globally used data **/
typedef struct _ApplicationData {
    GMainLoop *loop;
    GstBuffer *buffer;
    GStaticRWLock rwlock;
    int width;
    int height;
    unsigned int texture_id;
} ApplicationData;

static void
handoff_handler (GstElement *fakesink,
                 GstBuffer  *buffer,
                 GstPad     *pad,
                 gpointer    data);

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
GstMapInfo map; 
int isFirst = 1;
static void draw(float w, float h, ApplicationData *app) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, w, h);

    // unsigned char texDat[(int)(w*h)];
    // int i;
    // for (i = 0; i < (int)256*256; ++i)
    //     texDat[i] = i%600;//((i/(int)256)%2==0 ? 0: 255)
    
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 256, 256, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, texDat);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // // glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, GL_REPEAT);
    // glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

    // glClearColor (0.0, 0.0, 0.0, 0.0);
  
    // glGenTextures (1, &app->texture_id);
    // glBindTexture (GL_TEXTURE_2D, app->texture_id);
    // g_static_rw_lock_reader_lock (&app->rwlock);
    if (gst_buffer_map (app->buffer, &map, GST_MAP_READ)) { 
        // gst_util_dump_mem (map.data, map.size);
        if (isFirst) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                    app->width, app->height, 0,
                    GL_RGB, GL_UNSIGNED_BYTE, map.data );
        } else {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        app->width, app->height,
                        GL_RGB, GL_UNSIGNED_BYTE, map.data );
        }
        
        isFirst = 0;
        gst_buffer_unmap (app->buffer, &map); 
    }
    // g_static_rw_lock_reader_unlock (&app->rwlock);
    // glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    // glBindTexture(GL_TEXTURE_2D, 0);
    glVertexAttribPointer( g_hVertexLoc, 3, GL_FLOAT, 0, 0, vertices );
	glEnableVertexAttribArray( g_hVertexLoc );
    glVertexAttribPointer( g_hVertexTexLoc, 2, GL_FLOAT, 0, 0, VertexTexCoords );
	glEnableVertexAttribArray( g_hVertexTexLoc );

    glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, texture[0]);
    glDrawArrays( GL_TRIANGLE_STRIP, 0, sizeof(vertices) );

    glDisableVertexAttribArray( g_hVertexLoc );
    glDisableVertexAttribArray( g_hVertexTexLoc );

}

/** GStreamer related ****************************************/
static gboolean
bus_handler (GstBus     *bus,
             GstMessage *msg,
             gpointer    data)
{
    GMainLoop *loop = (GMainLoop *)data;

    switch (GST_MESSAGE_TYPE (msg)) {
        case GST_MESSAGE_ERROR: {
            gchar  *debug;
            GError *error;

            gst_message_parse_error (msg, &error, &debug);
            // g_printerr ("Error: %s\n", error->message);
            g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), error->message);
            g_printerr ("Debugging information: %s\n", debug ? debug : "none");
            g_free (debug);
            g_error_free (error);

            g_main_loop_quit (loop);
            break;
        }
        case GST_MESSAGE_STATE_CHANGED:
            /* We are only interested in state-changed messages from the pipeline */
            g_print ("Pipeline state changed ");
        break;
        default:
            break;
    }

    return TRUE;
}

static void
handoff_handler (GstElement *fakesink,
                 GstBuffer  *buffer,
                 GstPad     *pad,
                 gpointer    data)
{
    ApplicationData *app = (ApplicationData *)data;

    // g_static_rw_lock_writer_lock (&app->rwlock);
    /* Clean up previous reference */
    if (app->buffer != NULL) {
        gst_buffer_unref (app->buffer);
        app->buffer = NULL;
    }
    /* Refer upcoming buffer */
    app->buffer = gst_buffer_ref (buffer);
    // g_static_rw_lock_writer_unlock (&app->rwlock);

    return;
}


static GstElement *
create_pipeline (ApplicationData *app)
{
    GstElement *pipeline, *source, *csp, *filter, *sink;
    GstElement *decoder, *freeze;
    GstBus *bus;
    GstPad *pad;
    GstCaps *caps;
    GstStructure *structure;
    GstStateChangeReturn ret;

    /* Create gstreamer elements */
    // pipeline = gst_parse_launch(" filesrc location=\"/home/ubuntu/glfw/test_0.png\" ! pngdec ! imagefreeze ! videoconvert ! fakesink", NULL);
    pipeline = gst_pipeline_new ("video-player"); // pipeline
    source   = gst_element_factory_make ("v4l2src",     "video-source"); //videotestsrc
    csp      = gst_element_factory_make ("videoconvert", "csp");
    filter   = gst_element_factory_make ("capsfilter",       "filter");
    sink     = gst_element_factory_make ("fakesink",         "fakesink"); //fakesink

    //image decoder
    decoder = gst_element_factory_make ("pngdec", "png-decoder");
    freeze  = gst_element_factory_make ("imagefreeze", "freeze");
    if(!pipeline) {
        g_printerr("pipeline err");
    }
    if (!decoder) {
        g_printerr("decoder err");
    }
    if (!freeze) {
        g_printerr("freeze err");
    }

    if (!pipeline || !source || !csp || !filter || !sink || !decoder || !freeze) {
        g_printerr ("One element could not be created. Exiting.\n");
        return NULL;
    }

    g_object_set (G_OBJECT (source), "device", "/dev/video4", NULL);
    // g_object_set (G_OBJECT (source), "location", "./test_0.png", NULL);
    /* we set a property of elements */
    g_object_set (G_OBJECT (sink), "sync", TRUE, NULL);
    g_object_set (G_OBJECT (sink), "signal-handoffs", TRUE, NULL);
    g_signal_connect (G_OBJECT (sink), "handoff",
                        G_CALLBACK(handoff_handler), app);
    caps = gst_caps_new_simple ("video/x-raw",
                                "format", G_TYPE_STRING, "RGB", //I420
                                // "format", G_TYPE_STRING, "RGB16",
                                "bpp", G_TYPE_INT, 24,
                                "depth", G_TYPE_INT, 24,
                                NULL);
    g_object_set (G_OBJECT(filter), "caps", caps, NULL);
    gst_caps_unref(caps);

    /* we add a message handler */
    bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
    gst_bus_add_watch (bus, bus_handler, app->loop);
    gst_object_unref (bus);

    /* we add all elements into the pipeline */
    gst_bin_add_many (GST_BIN (pipeline), source, csp, filter, sink, NULL);

    /* we link the elements together */
    gst_element_link_many (source, csp, filter, sink, NULL);

    /* Set the pipeline to "playing" state */
    ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
    if (gst_element_get_state (pipeline, NULL, NULL, GST_CLOCK_TIME_NONE)
            != GST_STATE_CHANGE_SUCCESS)
    {
        return NULL;
    }

    if (ret == GST_STATE_CHANGE_FAILURE) {
		g_printerr ("Unable to set the pipeline to the playing state.\n");
		gst_object_unref (pipeline);
		return NULL;
	}
    g_print("play");

    /* Get width and height of video data */
    pad = gst_element_get_static_pad (sink, "sink");
    caps = gst_pad_get_current_caps(pad);
    structure = gst_caps_get_structure(caps, 0);
    gst_structure_get_int (structure, "width", &app->width);
    gst_structure_get_int (structure, "height", &app->height);
    gst_object_unref (pad);

    return pipeline;
}

int main(int argc, char *argv[]) {
    GLuint shader_program, vbo;
    GLint pos;
    GLFWwindow* window;

    //gstreamer
    ApplicationData app;
    GstElement *pipeline;

    // gst init
    gst_init (&argc, &argv);

    if (!glfwInit())
        return -1;
    // g_main_loop_run (app.loop); // gstreamer loop
    
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);
    
    app.loop = g_main_loop_new (NULL, FALSE);
    app.buffer = NULL;
    app.width = mode->width;
    app.height = mode->height;
    app.texture_id = 0;
    
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

    pipeline = create_pipeline (&app);

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

    // texture
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        // texture
        draw(mode->width, mode->height, &app);
        g_usleep (1000/2);

        glfwSwapBuffers(window);
    }
    glDeleteBuffers(1, &vbo);
    glfwTerminate();

    /* Release objects before quit */
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);

    gst_buffer_unref (app.buffer);
    g_main_loop_unref (app.loop);
    return EXIT_SUCCESS;
}