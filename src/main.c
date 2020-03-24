#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <cglm/cglm.h>
#include <cglm/call.h>
#include <math.h>
#include <ft2build.h>
#include FT_FREETYPE_H


/**
 * Capture errors from glfw.
 */
static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

/**
 * Capture key callbacks from glfw
 */
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

typedef struct CHARACTER_STRUCT
{
    GLuint texture;   // ID handle of the glyph texture
    vec2 size;    // Size of glyph
    float width;
    float height;
    float bearing_left;
    float bearing_top;
    GLuint advance;    // Horizontal offset to advance to next glyph
} character_T;

character_T* get_character(char c)
{
    // FreeType
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
        perror("ERROR::FREETYPE: Could not init FreeType Library");

    // Load font as face
    FT_Face face;
    if (FT_New_Face(ft, "/usr/share/fonts/truetype/lato/Lato-Medium.ttf", 0, &face))
        perror("ERROR::FREETYPE: Failed to load font");

    // Set size to load glyphs as
    FT_Set_Pixel_Sizes(face, 0, 18);

    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 

    // Load character glyph 
    if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        perror("ERROR::FREETYTPE: Failed to load Glyph");

    // Generate texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RED,
        face->glyph->bitmap.width,
        face->glyph->bitmap.rows,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        face->glyph->bitmap.buffer
    );
    // Set texture options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // Now store character for later use
    character_T* character = calloc(1, sizeof(struct CHARACTER_STRUCT));
    character->texture = texture;
    character->width = face->glyph->bitmap.width;
    character->height = face->glyph->bitmap.rows;
    character->bearing_left = face->glyph->bitmap_left;
    character->bearing_top = face->glyph->bitmap_top;
    character->advance = face->glyph->advance.x;
    glBindTexture(GL_TEXTURE_2D, 0);
    // Destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft); 

    return character;
}

int main(int argc, char* argv[])
{
    glfwSetErrorCallback(error_callback);

    /**
     * Initialize glfw to be able to use it.
     */
    if (!glfwInit())
        perror("Failed to initialize glfw.\n");

    /**
     * Setting some parameters to the window,
     * using OpenGL 3.3
     */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_FLOATING, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    /**
     * Creating our window
     */
    GLFWwindow* window = glfwCreateWindow(640, 480, "My Title", NULL, NULL);
    if (!window)
        perror("Failed to create window.\n");

    glfwSetKeyCallback(window, key_callback);

    /**
     * Enable OpenGL as current context
     */
    glfwMakeContextCurrent(window);

    /** 
     * Initialize glew and check for errors
     */
    GLenum err = glewInit();
    if (GLEW_OK != err)
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));

    fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);

    unsigned int VBO;
    glGenBuffers(1, &VBO);

    GLuint vertex_shader, fragment_shader, program;
    GLint mvp_location, vertex_location;

    /**
     * Vertex Shader
     */
    static const char* vertex_shader_text =
        "#version 330 core\n"
        "uniform mat4 MVP;\n"
        "attribute vec4 thevertex;\n"
        "out vec2 TexCoord;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = MVP * vec4(thevertex.xy, 0.0, 1.0);\n"
        "    TexCoord = thevertex.zw;"
        "}\n";
    
    /**
     * Fragment Shader
     */    
    static const char* fragment_shader_text =
        "#version 330 core\n"
        "varying vec3 color;\n"
        "in vec2 TexCoord;\n"
        "uniform sampler2D ourTexture;\n"
        "void main()\n"
        "{\n"
        "    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(ourTexture, TexCoord).r);\n"
        "    gl_FragColor = vec4(vec3(1, 1, 1), 1.0) * sampled;\n"
        "}\n"; 

    int success;
    char infoLog[512];
 
    /**
     * Compile vertex shader and check for errors
     */
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        printf("Vertex Shader Error\n");
        glGetShaderInfoLog(vertex_shader, 512, NULL, infoLog);
        perror(infoLog);
    }

    /**
     * Compile fragment shader and check for errors
     */ 
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        printf("Fragment Shader Error\n");
        glGetShaderInfoLog(fragment_shader, 512, NULL, infoLog);
        perror(infoLog);
    }

    /**
     * Create shader program and check for errors
     */ 
    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success)
    {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        perror(infoLog);
    }

    /**
     * Grab locations from shader
     */
    vertex_location = glGetAttribLocation(program, "thevertex");
    mvp_location = glGetUniformLocation(program, "MVP");

    glBindVertexArray(VAO); 

    /**
     * Create and bind texture
     */
    character_T* character = get_character('H');
    unsigned int texture = character->texture;

    float scale = 0.1f;
    GLfloat xpos = -(character->width*scale) / 2;
    GLfloat ypos = -(character->height*scale) / 2; 

    GLfloat w = character->width * scale;
    GLfloat h = character->height * scale;

    GLfloat vertices[6][4] = {
        { xpos,     ypos + h,   0.0, 0.0 },            
        { xpos,     ypos,       0.0, 1.0 },
        { xpos + w, ypos,       1.0, 1.0 },

        { xpos,     ypos + h,   0.0, 0.0 },
        { xpos + w, ypos,       1.0, 1.0 },
        { xpos + w, ypos + h,   1.0, 0.0 }           
    }; 

    /**
     * Main loop
     */
    while (!glfwWindowShouldClose(window))
    {
        int width, height;
        mat4 p, mvp;

        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.2f, 0.4f, 0.2f, 1.0f);
        
        mat4 m = GLM_MAT4_IDENTITY_INIT; 

        glm_translate(m, (vec3){ 0, 0, 0 });
        
        glm_ortho_default(width / (float) height, p);
        glm_mat4_mul(p, m, mvp);

        glUseProgram(program);
        glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*) mvp);

        /**
         * Draw texture
         */
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(VAO);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(vertex_location);
        glVertexAttribPointer(vertex_location, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
   
    glfwDestroyWindow(window); 
    glfwTerminate();
    return 0;
}
