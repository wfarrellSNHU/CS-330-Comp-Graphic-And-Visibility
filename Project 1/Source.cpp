#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "camera.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"     // Image loading Utility functions

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Project 7-1"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao;         // Handle for the vertex array object
        GLuint vbo;        // Handles for the vertex buffer objects
        GLuint nVertices;   // Number of indices of the mesh
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;

    // Triangle mesh data
    GLMesh bottleMesh; 
    GLMesh capMesh; 
    GLMesh groundMesh;
    GLMesh wiperBack1;
    GLMesh wiperBox1;
    GLMesh wiperBack2;
    GLMesh wiperBox2;
    GLMesh screwDriverHandle;
    GLMesh screwDriverRod;
    GLMesh screwDriverTip;

    // Texture
    GLuint groundTextureId;
    GLuint bottleTextureId;
    GLuint capTextureId;
    GLuint wiperBackTextureId;
    GLuint wiperBoxTextureId;
    GLuint screwDriverHandleTextureId;
    GLuint screwDriverTextureId;

    glm::vec2 gUVScale(5.0f, 5.0f);
    GLint gTexWrapMode = GL_REPEAT;

    // Shader programs
    GLuint gProgramId;
    GLuint gLampProgramId;

    // camera
    Camera gCamera(glm::vec3(0.0f, 1.0f, 3.0f));
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

    float y = 0.0f;
    int cameraMode = 1;  // perspective = 1, ortho = 2

    // Cube and light color
    glm::vec3 gObjectColor(1.f, 0.2f, 0.0f);
    glm::vec3 gLightColor(0.8f,0.8f, 0.8f);

    // Light position and scale
    glm::vec3 gLightPosition(-1.5f, 20.5f, 0.0f);
    glm::vec3 gLightScale(0.3f);
}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UCreateMeshBottle(GLMesh& mesh);
void UCreateMeshCap(GLMesh& mesh);
void UCreateMeshGround(GLMesh& mesh);
void UCreateMeshWiperBack(GLMesh& mesh);
void UCreateMeshWiperBox(GLMesh& mesh);
void UCreateMeshScrewDriverHandle(GLMesh& mesh);
void UCreateMeshScrewDriverRod(GLMesh& mesh);
void UCreateMeshScrewDriverTip(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);
bool UCreateTexture(const char* filename, GLuint& textureId, GLint param);
void UDestroyTexture(GLuint textureId);


/* Vertex Shader Source Code*/
const GLchar* vertexShaderSource = GLSL(440,
layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
layout(location = 1) in vec3 normal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate;

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

    vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

    vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
    vertexTextureCoordinate = textureCoordinate;
}
);


/* Fragment Shader Source Code*/
const GLchar* fragmentShaderSource = GLSL(440,
in vec3 vertexNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate;

out vec4 fragmentColor; // For outgoing cube color to the GPU

// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPosition;
uniform sampler2D uTexture; // Useful when working with multiple textures
uniform vec2 uvScale;

void main()
{
    /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

    //Calculate Ambient lighting*/
    float ambientStrength = 0.8f; // Set ambient or global lighting strength
    vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

    //Calculate Diffuse lighting*/
    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    vec3 diffuse = impact * lightColor; // Generate diffuse light color

    //Calculate Specular lighting*/
    float specularIntensity = 0.5f; // Set specular light strength
    float highlightSize = 0.8f; // Set specular highlight size
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
    //Calculate specular component
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    vec3 specular = specularIntensity * specularComponent * lightColor;

    // Texture holds the color to be used for all three components
    vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

    // Calculate phong result
    vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

    fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
}
);

/* Lamp Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);


/* Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

void main()
{
    fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}

int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Create the mesh
    UCreateMeshGround(groundMesh);
    UCreateMeshBottle(bottleMesh); 
    UCreateMeshCap(capMesh);
    UCreateMeshWiperBack(wiperBack1);
    UCreateMeshWiperBox(wiperBox1);
    UCreateMeshWiperBack(wiperBack2);
    UCreateMeshWiperBox(wiperBox2);
    UCreateMeshScrewDriverHandle(screwDriverHandle);
    UCreateMeshScrewDriverRod(screwDriverRod);
    UCreateMeshScrewDriverTip(screwDriverTip);
    

    // Create the shader program
    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
        return EXIT_FAILURE;

    if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampProgramId))
        return EXIT_FAILURE;

    // Load ground texture
    const char* tex0Filename = "./resources/textures/concrete.png";
    if (!UCreateTexture(tex0Filename, groundTextureId, GL_MIRRORED_REPEAT))
    {
        cout << "Failed to load texture concrete " << tex0Filename << endl;
        return EXIT_FAILURE;
    }
     
    // Load bottle texture
    const char* tex1Filename = "./resources/textures/whitePlastic.png";
    if (!UCreateTexture(tex1Filename, bottleTextureId, GL_MIRRORED_REPEAT))
    {
        cout << "Failed to load texture bottle " << tex1Filename << endl;
        return EXIT_FAILURE;
    } 
 
    // Load cap texture
    const char* tex2Filename = "./resources/textures/yellowPlastic.jpg";
    if (!UCreateTexture(tex2Filename, capTextureId, GL_MIRRORED_REPEAT))
    {
        cout << "Failed to load texture bottleCap" << tex2Filename << endl;
        return EXIT_FAILURE;
    }

    // Load wiper back texture
    const char* tex3Filename = "./resources/textures/wiperBack.png";
    if (!UCreateTexture(tex3Filename, wiperBackTextureId, GL_MIRRORED_REPEAT))
    {
        cout << "Failed to load texture wiper back " << tex3Filename << endl;
        return EXIT_FAILURE;
    }

    // Load wiper box texture
    const char* tex4Filename = "./resources/textures/wiperBox.jpg";
    if (!UCreateTexture(tex4Filename, wiperBoxTextureId, GL_MIRRORED_REPEAT))
    {
        cout << "Failed to load texture wiper box" << tex4Filename << endl;
        return EXIT_FAILURE;
    }

    // Load screw driver handle texture
    const char* tex5Filename = "./resources/textures/screwDriverHandle.jpg";
    if (!UCreateTexture(tex5Filename, screwDriverHandleTextureId, GL_MIRRORED_REPEAT))
    {
        cout << "Failed to load texture screw driver handle " << tex5Filename << endl;
        return EXIT_FAILURE;
    }

    // Load screw driver metal texture
    const char* tex6Filename = "./resources/textures/screwDriver.png";
    if (!UCreateTexture(tex6Filename, screwDriverTextureId, GL_MIRRORED_REPEAT))
    {
        cout << "Failed to load texture screw driver metal " << tex6Filename << endl;
        return EXIT_FAILURE;
    }

    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gProgramId);
    // We set the texture as texture units
    glUniform1i(glGetUniformLocation(gProgramId, "groundTexture"), 0);
    glUniform1i(glGetUniformLocation(gProgramId, "bottleTexture"), 1);
    glUniform1i(glGetUniformLocation(gProgramId, "capTexture"), 2);
    glUniform1i(glGetUniformLocation(gProgramId, "wiperBackTextureId"), 3);
    glUniform1i(glGetUniformLocation(gProgramId, "wiperBoxTextureId"), 4);
    glUniform1i(glGetUniformLocation(gProgramId, "screwDriverHandleTextureId"), 5);
    glUniform1i(glGetUniformLocation(gProgramId, "screwDriverTextureId"), 6);

     // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        // --------------------
        float currentFrame = (float)glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URender();

        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(groundMesh);
    UDestroyMesh(bottleMesh);
    UDestroyMesh(capMesh);
    UDestroyMesh(wiperBack1);
    UDestroyMesh(wiperBox1);
    UDestroyMesh(wiperBack1);
    UDestroyMesh(wiperBox1);
    UDestroyMesh(screwDriverHandle);
    UDestroyMesh(screwDriverRod);
    UDestroyMesh(screwDriverTip);
    

    // Release texture
    UDestroyTexture(groundTextureId);
    UDestroyTexture(bottleTextureId);
    UDestroyTexture(capTextureId);
    UDestroyTexture(wiperBackTextureId);
    UDestroyTexture(wiperBoxTextureId);
    UDestroyTexture(screwDriverHandleTextureId);
    UDestroyTexture(screwDriverTextureId);

    // Release shader program
    UDestroyShaderProgram(gProgramId);
    UDestroyShaderProgram(gLampProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    static const float cameraSpeed = 2.5f;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        y += gCamera.MovementSpeed * gDeltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        y -= gCamera.MovementSpeed * gDeltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        if (cameraMode == 1)
            cameraMode = 2;
        else
            cameraMode = 1;
    }
}

// glfw: Whenever the mouse moves, this callback is called.
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = (float)xpos;
        gLastY = (float)ypos;
        gFirstMouse = false;
    }

    float xoffset = (float)xpos - (float)gLastX;
    float yoffset = (float)gLastY - (float)ypos; // reversed since y-coordinates go from bottom to top

    gLastX = (float)xpos;
    gLastY = (float)ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (gCamera.MovementSpeed != 0.0f) {
        gCamera.MovementSpeed += (float)yoffset;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Functioned called to render a frame
void URender()
{
    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();
    // translate for Y model
    glm::mat4 translation = glm::translate(glm::vec3(0.0f, y, 0.0f));
    view = view * translation;


    glm::mat4 projection;
    if (cameraMode == 2) {
        // Creates a ortho projection
        projection = glm::ortho(-2.0f, +2.0f, -1.5f, +1.5f, 0.1f, 100.0f);
    }
    else {
        // Creates a perspective projection
        projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    }
    
    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");
  
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    GLint objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    GLint lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    GLint lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));


    glm::mat4  model = glm::mat4(1.0f);
    glm::mat4  scale = glm::mat4(1.0f);
    glm::mat4  rotation = glm::mat4(1.0f);
    glm::mat4  mtranslation = glm::mat4(1.0f);

    // Ground
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(groundMesh.vao);
    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    // Bind the texture
    glBindTexture(GL_TEXTURE_2D, groundTextureId);
    // Draws the vertices array
    glDrawArrays(GL_TRIANGLES, 0, groundMesh.nVertices); 

    // Bottle
    // 1. Scales the object 
    scale = glm::scale(glm::vec3(0.5f, 1.0f, 0.5f));
    // 2. Rotates shape by 15 degrees in the x axis
    rotation = glm::rotate(glm::radians(45.0f), glm::vec3(0.0f, 0.1f, 0.0f));
    // 3. Place object at the origin
    mtranslation = glm::translate(glm::vec3(0.5f, 0.5f, 0.0f));
    // Model matrix: transformations are applied right-to-left order
    model = scale * rotation * mtranslation;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(bottleMesh.vao);
    // Bind the texture
    glBindTexture(GL_TEXTURE_2D, bottleTextureId);
    // Draws the vertices array
    glDrawArrays(GL_TRIANGLES, 0, bottleMesh.nVertices);

    // Cap
    // 1. Scales the object
    scale = glm::scale(glm::vec3(0.3f, 0.3f, 0.3f));
    // 2. Rotates shape by 15 degrees in the x axis
    rotation = glm::rotate(glm::radians(25.0f), glm::vec3(0.0f, 0.1f, 0.0f));
    // 3. Place object at the origin
    mtranslation = glm::translate(glm::vec3(0.5f, 3.5f, -0.5f));
    // Model matrix: transformations are applied right-to-left order
    model = scale * rotation * mtranslation;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    // Bind the vertices
    glBindVertexArray(capMesh.vao);   
    // Bind the texture
    glBindTexture(GL_TEXTURE_2D, capTextureId);
    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, capMesh.nVertices);


    // Wiper Back Right
    model = glm::mat4(1.0f);
    // 1. Scales the object
    scale = glm::scale(glm::vec3(0.5f, 0.5f, 2.0f));
    // 2. Rotates shape by 15 degrees in the x axis
    rotation = glm::rotate(glm::radians(0.0f), glm::vec3(1.1f, 0.0f, 0.0f));
    // 3. Place object at the origin
    mtranslation = glm::translate(glm::vec3(4.0f, 0.1f, 0.0f));
    // Model matrix: transformations are applied right-to-left order
    model = scale * rotation * mtranslation;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    // Bind the vertices
    glBindVertexArray(wiperBack1.vao);
    // Bind the texture
    glBindTexture(GL_TEXTURE_2D, wiperBackTextureId);
    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, wiperBack1.nVertices);

    // Wiper Back Left
    model = glm::mat4(1.0f);
    // 1. Scales the object
    scale = glm::scale(glm::vec3(0.5f, 0.5f, 2.0f));
    // 2. Rotates shape by 15 degrees in the x axis
    rotation = glm::rotate(glm::radians(0.0f), glm::vec3(-1.1f, 0.0f, 0.0f));
    // 3. Place object at the origin
    mtranslation = glm::translate(glm::vec3(-4.0f, 0.1f, 0.0f));
    // Model matrix: transformations are applied right-to-left order
    model = scale * rotation * mtranslation;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    // Bind the vertices
    glBindVertexArray(wiperBack2.vao);
    // Bind the texture
    glBindTexture(GL_TEXTURE_2D, wiperBackTextureId);
    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, wiperBack2.nVertices);


    // Wiper Box 1
    model = glm::mat4(1.0f);
    // 1. Scales the object
    scale = glm::scale(glm::vec3(0.35f, 0.1f, 3.3f));
    // 2. Rotates shape by 15 degrees in the x axis
    rotation = glm::rotate(glm::radians(0.0f), glm::vec3(9.1f, 0.1f, 0.0f));
    // 3. Place object at the origin
    mtranslation = glm::translate(glm::vec3(-2.0f, 0.1f, 2.0f));
    // Model matrix: transformations are applied right-to-left order
    model = mtranslation * scale * rotation;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    // Bind the vertices
    glBindVertexArray(wiperBox1.vao);
    // Bind the texture
    glBindTexture(GL_TEXTURE_2D, wiperBoxTextureId);
    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, wiperBox1.nVertices);

    // Wiper Box 2
    model = glm::mat4(1.0f);
    // 1. Scales the object
    scale = glm::scale(glm::vec3(0.35f, 0.1f, 3.3f));
    // 2. Rotates shape by 15 degrees in the x axis
    rotation = glm::rotate(glm::radians(0.0f), glm::vec3(-9.1f, 0.0f, 0.0f));
    // 3. Place object at the origin
    mtranslation = glm::translate(glm::vec3(2.0f, 0.1f, 2.0f));
    // Model matrix: transformations are applied right-to-left order
    model = mtranslation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    // Bind the vertices
    glBindVertexArray(wiperBox2.vao);
    // Bind the texture
    glBindTexture(GL_TEXTURE_2D, wiperBoxTextureId);
    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, wiperBox2.nVertices);

    // Screw Driver Handle
    model = glm::mat4(1.0f);
    // 1. Scales the object
    scale = glm::scale(glm::vec3(0.3f, 0.f, 0.3f));
    // 2. Rotates shape by 15 degrees in the x axis
    rotation = glm::rotate(glm::radians(25.0f), glm::vec3(0.0f, 0.1f, 0.0f));
    // 3. Place object at the origin
    mtranslation = glm::translate(glm::vec3(1.5f, 0.0f, -2.0f));
    // Model matrix: transformations are applied right-to-left order
    model = scale * rotation * mtranslation;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    // Bind the vertices
    glBindVertexArray(screwDriverHandle.vao);
    // Bind the texture
    glBindTexture(GL_TEXTURE_2D, screwDriverHandleTextureId);
    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, screwDriverHandle.nVertices);

    // Screw Driver Rod
    model = glm::mat4(1.0f);
    // 1. Scales the object
    scale = glm::scale(glm::vec3(0.1f, 0.0f, 0.0f));
    // 2. Rotates shape by 15 degrees in the x axis
    rotation = glm::rotate(glm::radians(25.0f), glm::vec3(0.0f, 0.1f, 0.0f));
    // 3. Place object at the origin
    mtranslation = glm::translate(glm::vec3(3.5f, 3.0f, -2.5f));
    // Model matrix: transformations are applied right-to-left order
    model = scale * rotation * mtranslation;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    // Bind the vertices
    glBindVertexArray(screwDriverRod.vao);
    // Bind the texture
    glBindTexture(GL_TEXTURE_2D, screwDriverTextureId);
    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, screwDriverRod.nVertices);

    // Screw Driver Tip
    model = glm::mat4(1.0f);
    // 1. Scales the object
    scale = glm::scale(glm::vec3(0.3f, 0.3f, 0.3f));
    // 2. Rotates shape by 15 degrees in the x axis
    rotation = glm::rotate(glm::radians(25.0f), glm::vec3(0.0f, 0.1f, 0.0f));
    // 3. Place object at the origin
    mtranslation = glm::translate(glm::vec3(1.5f, 0.0f, -2.5f));
    // Model matrix: transformations are applied right-to-left order
    model = scale * rotation * mtranslation;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    // Bind the vertices
    glBindVertexArray(screwDriverTip.vao);
    // Bind the texture
    glBindTexture(GL_TEXTURE_2D, screwDriverTextureId);
    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, screwDriverTip.nVertices); 

    // LAMP: draw lamp
    glUseProgram(gLampProgramId);
    //Transform the smaller cube used as a visual que for the light source
    model = glm::translate(gLightPosition) * glm::scale(gLightScale);
    // Reference matrix uniforms from the Lamp Shader program
    modelLoc = glGetUniformLocation(gLampProgramId, "model");
    viewLoc = glGetUniformLocation(gLampProgramId, "view");
    projLoc = glGetUniformLocation(gLampProgramId, "projection");
    // Pass matrix data to the Lamp Shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glDrawArrays(GL_TRIANGLES, 0, groundMesh.nVertices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
    glUseProgram(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow); // Flips the the back buffer with the front buffer every frame.
}

void UCreateMeshBottle(GLMesh& mesh)
{

    // Position and texture data
    GLfloat verts[] = {
        // Vertex Positions   // Normals           // Texture Coordinates    
         0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 0 
         0.5f, -0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 1  
        -0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 3  
         0.5f, -0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 1 
        -0.5f, -0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 2 
        -0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 3 

         0.5f,  0.5f, 0.0f,   -1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 0 
         0.5f, -0.5f, 0.0f,   -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 1 
         0.5f, -0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 4 
         0.5f,  0.5f, 0.0f,   -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 0 
         0.5f, -0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 4 
         0.5f,  0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 5 

         0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 0 
         0.5f,  0.5f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 5 
        -0.5f,  0.5f, -1.0f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 6 
         0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 0 
        -0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 3 
        -0.5f,  0.5f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 6 

         0.5f, -0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 4 
         0.5f,  0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 5 
        -0.5f,  0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 6
         0.5f, -0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 4 
        -0.5f,  0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 6
        -0.5f, -0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 7 

        -0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // 2 
        -0.5f,  0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  1.0f, 1.0f, // 3 
        -0.5f,  0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // 6
        -0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // 2 
        -0.5f,  0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f, // 6
        -0.5f, -0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // 7

         0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // 1
         0.5f, -0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f, // 4
        -0.5f, -0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // 7 
         0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // 1
        -0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  0.0f, 0.0f, // 2
        -0.5f, -0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // 7 

    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

void UCreateMeshCap(GLMesh& mesh)
{

    // Position and texture data
    GLfloat verts[] = {
        // Vertex Positions   // Normals           // Texture Coordinates    
         0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 0 
         0.5f, -0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 1  
        -0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 3  
         0.5f, -0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 1 
        -0.5f, -0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 2 
        -0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 3 

         0.5f,  0.5f, 0.0f,   -1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 0 
         0.5f, -0.5f, 0.0f,   -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 1 
         0.5f, -0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 4 
         0.5f,  0.5f, 0.0f,   -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 0 
         0.5f, -0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 4 
         0.5f,  0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 5 

         0.5f,  0.5f, 0.0f,   0.0f,  0.0f,  1.0f,  1.0f, 0.0f, // 0 
         0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f, // 5 
        -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f, // 6 
         0.5f,  0.5f, 0.0f,   0.0f,  0.0f,  1.0f,  0.0f, 1.0f, // 0 
        -0.5f,  0.5f, 0.0f,   0.0f,  0.0f,  1.0f,  0.0f, 0.0f, // 3 
        -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f, // 6 

         0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  -1.0f,  1.0f, 0.0f, // 4 
         0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  -1.0f,  1.0f, 1.0f, // 5 
        -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  -1.0f,  0.0f, 1.0f, // 6
         0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  -1.0f,  0.0f, 1.0f, // 4 
        -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  -1.0f,  0.0f, 0.0f, // 6
        -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  -1.0f,  1.0f, 0.0f, // 7 

        -0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // 2 
        -0.5f,  0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  1.0f, 1.0f, // 3 
        -0.5f,  0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // 6
        -0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // 2 
        -0.5f,  0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f, // 6
        -0.5f, -0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // 7

         0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // 1
         0.5f, -0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f, // 4
        -0.5f, -0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // 7 
         0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // 1
        -0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  0.0f, 0.0f, // 2
        -0.5f, -0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // 7 

    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

void UCreateMeshGround(GLMesh& mesh)
{
    // Position and Texture data
    GLfloat verts[] = {
        // Vertex Positions  // Normals           //Texture Coordinates 
         5.0f,0.0f,-5.0f,    0.0f,  1.0f,  0.0f,  0.0f,  0.0f, // Bottom Right Vertex 0
        -5.0f,0.0f,-5.0f,    0.0f,  1.0f,  0.0f,  1.0f,  0.0f, // Bottom Left Vertex 1
         5.0f,0.0f, 5.0f,    0.0f,  1.0f,  0.0f,  1.0f,  1.0f, // Top Right Vertex 2

        -5.0f,0.0f,-5.0f,    0.0f,  1.0f,  0.0f,  1.0f,  0.0f, // Bottom Left Vertex 1
         5.0f,0.0f, 5.0f,    0.0f,  1.0f,  0.0f,  1.0f,  1.0f, // Top Right Vertex 2
        -5.0f,0.0f, 5.0f,    0.0f,  1.0f,  0.0f,  0.0f,  1.0f  // Top Left Vertex 3
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

void UCreateMeshWiperBack(GLMesh& mesh)
{
    // Position and Texture data
    GLfloat verts[] = {
        // Vertex Positions  // Normals           //Texture Coordinates 
         0.5f, 0.0f,-1.0f,    0.0f,  1.0f,  0.0f,  0.0f,  0.0f, // Bottom Right Vertex 0
        -0.5f, 0.0f,-1.0f,    0.0f,  1.0f,  0.0f,  1.0f,  0.0f, // Bottom Left Vertex 1
         0.5f, 0.0f, 1.0f,    0.0f,  1.0f,  0.0f,  1.0f,  1.0f, // Top Right Vertex 2

        -0.5f, 0.0f,-1.0f,    0.0f,  1.0f,  0.0f,  1.0f,  0.0f, // Bottom Left Vertex 1
         0.6f, 0.0f, 1.0f,    0.0f,  1.0f,  0.0f,  1.0f,  1.0f, // Top Right Vertex 2
        -0.5f, 0.0f, 1.0f,    0.0f,  1.0f,  0.0f,  0.0f,  1.0f  // Top Left Vertex 3
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

void UCreateMeshWiperBox(GLMesh& mesh)
{

    // Position and texture data
    GLfloat verts[] = {
        // Vertex Positions   // Normals           // Texture Coordinates    
         0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 0 
         0.5f, -0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 1  
        -0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 3  
         0.5f, -0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 1 
        -0.5f, -0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 2 
        -0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 3 

         0.5f,  0.5f, 0.0f,   -1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 0 
         0.5f, -0.5f, 0.0f,   -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 1 
         0.5f, -0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 4 
         0.5f,  0.5f, 0.0f,   -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 0 
         0.5f, -0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 4 
         0.5f,  0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 5 

         0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 0 
         0.5f,  0.5f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 5 
        -0.5f,  0.5f, -1.0f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 6 
         0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 0 
        -0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 3 
        -0.5f,  0.5f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 6 

         0.5f, -0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 4 
         0.5f,  0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 5 
        -0.5f,  0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 6
         0.5f, -0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 4 
        -0.5f,  0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 6
        -0.5f, -0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 7 

        -0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // 2 
        -0.5f,  0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  1.0f, 1.0f, // 3 
        -0.5f,  0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // 6
        -0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // 2 
        -0.5f,  0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f, // 6
        -0.5f, -0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // 7

         0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // 1
         0.5f, -0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f, // 4
        -0.5f, -0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // 7 
         0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // 1
        -0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  0.0f, 0.0f, // 2
        -0.5f, -0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // 7 

    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

void UCreateMeshScrewDriverHandle(GLMesh& mesh)
{

    // Position and texture data
    GLfloat verts[] = {
        // Vertex Positions   // Normals           // Texture Coordinates    
         0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 0 
         0.5f, -0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 1  
        -0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 3  
         0.5f, -0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 1 
        -0.5f, -0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 2 
        -0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 3 

         0.5f,  0.5f, 0.0f,   -1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 0 
         0.5f, -0.5f, 0.0f,   -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 1 
         0.5f, -0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 4 
         0.5f,  0.5f, 0.0f,   -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 0 
         0.5f, -0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 4 
         0.5f,  0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 5 

         0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 0 
         0.5f,  0.5f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 5 
        -0.5f,  0.5f, -1.0f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 6 
         0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 0 
        -0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 3 
        -0.5f,  0.5f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 6 

         0.5f, -0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 4 
         0.5f,  0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 5 
        -0.5f,  0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 6
         0.5f, -0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 4 
        -0.5f,  0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 6
        -0.5f, -0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 7 

        -0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // 2 
        -0.5f,  0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  1.0f, 1.0f, // 3 
        -0.5f,  0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // 6
        -0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // 2 
        -0.5f,  0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f, // 6
        -0.5f, -0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // 7

         0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // 1
         0.5f, -0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f, // 4
        -0.5f, -0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // 7 
         0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // 1
        -0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  0.0f, 0.0f, // 2
        -0.5f, -0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // 7 

    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

void UCreateMeshScrewDriverRod(GLMesh& mesh)
{

    // Position and texture data
    GLfloat verts[] = {
        // Vertex Positions   // Normals           // Texture Coordinates    
         0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 0 
         0.5f, -0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 1  
        -0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 3  
         0.5f, -0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 1 
        -0.5f, -0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 2 
        -0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 3 

         0.5f,  0.5f, 0.0f,   -1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 0 
         0.5f, -0.5f, 0.0f,   -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 1 
         0.5f, -0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 4 
         0.5f,  0.5f, 0.0f,   -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 0 
         0.5f, -0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 4 
         0.5f,  0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 5 

         0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 0 
         0.5f,  0.5f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 5 
        -0.5f,  0.5f, -1.0f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 6 
         0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 0 
        -0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 3 
        -0.5f,  0.5f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 6 

         0.5f, -0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 4 
         0.5f,  0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 5 
        -0.5f,  0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 6
         0.5f, -0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 4 
        -0.5f,  0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 6
        -0.5f, -0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 7 

        -0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // 2 
        -0.5f,  0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  1.0f, 1.0f, // 3 
        -0.5f,  0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // 6
        -0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // 2 
        -0.5f,  0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f, // 6
        -0.5f, -0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // 7

         0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // 1
         0.5f, -0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f, // 4
        -0.5f, -0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // 7 
         0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // 1
        -0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  0.0f, 0.0f, // 2
        -0.5f, -0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // 7 

    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

void UCreateMeshScrewDriverTip(GLMesh& mesh)
{

    // Position and texture data
    GLfloat verts[] = {
        // Vertex Positions   // Normals           // Texture Coordinates    
         0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 0 
         0.5f, -0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 1  
        -0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 3  
         0.5f, -0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 1 
        -0.5f, -0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 2 
        -0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 3 

         0.5f,  0.5f, 0.0f,   -1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 0 
         0.5f, -0.5f, 0.0f,   -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 1 
         0.5f, -0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 4 
         0.5f,  0.5f, 0.0f,   -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 0 
         0.5f, -0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 4 
         0.5f,  0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 5 

         0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 0 
         0.5f,  0.5f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 5 
        -0.5f,  0.5f, -1.0f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 6 
         0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 0 
        -0.5f,  0.5f, 0.0f,   1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 3 
        -0.5f,  0.5f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 6 

         0.5f, -0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 4 
         0.5f,  0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // 5 
        -0.5f,  0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 6
         0.5f, -0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // 4 
        -0.5f,  0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // 6
        -0.5f, -0.5f, -1.0f,  -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // 7 

        -0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // 2 
        -0.5f,  0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  1.0f, 1.0f, // 3 
        -0.5f,  0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // 6
        -0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // 2 
        -0.5f,  0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f, // 6
        -0.5f, -0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // 7

         0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // 1
         0.5f, -0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f, // 4
        -0.5f, -0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // 7 
         0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  1.0f, 0.0f, // 1
        -0.5f, -0.5f, 0.0f,   0.0f,  1.0f,  0.0f,  0.0f, 0.0f, // 2
        -0.5f, -0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // 7 

    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(1, &mesh.vbo);
}


// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}


/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId, GLint param)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, param);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, param);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}


void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}
