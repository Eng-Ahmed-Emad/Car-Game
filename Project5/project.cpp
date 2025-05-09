#define STB_IMAGE_IMPLEMENTATION
#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES 

#include <GL/stb_image.h>
#include <GL/glut.h>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <vector>

// ---- Constants ----
const float ROAD_WIDTH = 10.0f;        // Width of the road
const float ROAD_LENGTH = 500.0f;      // Length of the road along Z-axis (much longer now)
const float EDGE_WIDTH = 1.0f;         // Width of the green edges
const float CAR_SPEED = 0.2f;          // Movement speed of player's car
const float BOUNCE_FACTOR = 0.8f;      // How much the car bounces off edges (0-1)
const float OBSTACLE_SPEED = 0.4f;     // Movement speed of obstacles
const float SPAWN_INTERVAL = 2.0f;     // Time between obstacle spawns (seconds)
const float CAMERA_SPEED = 0.2f;       // Camera rotation/movement speed
const int MAX_OBSTACLES = 10;          // Maximum number of obstacles on the road at once

// ---- Game State Variables ----
struct GameState {
    // Car position and movement
    float carX = 0.0f;                 // Player car X position
    float carVelocityX = 0.0f;         // Car's X velocity for bouncing effect

    // Multiple obstacles
    struct Obstacle {
        float x;                      // Obstacle X position
        float z;                      // Obstacle Z position (distance from player)
        bool active;                  // Is this obstacle currently active
    };
    std::vector<Obstacle> obstacles;  // List of active obstacles

    // Camera settings
    float camX = 0.0f;                 // Camera X position
    float camY = 3.0f;                 // Camera Y position (height)
    float camZ = 5.0f;                 // Camera Z position (distance behind car)
    float camYaw = 0.0f;               // Camera rotation angle

    // Game status flags
    bool gameOver = false;             // Flag for game over state
    bool isPaused = false;             // Flag for paused state
    bool isFullScreen = false;         // Flag for fullscreen mode

    // Timing
    float timeSinceLastSpawn = 0.0f;   // Timer for obstacle spawning
};

// ---- Textures ----
struct Textures {
    GLuint road = 0;                   // Road texture
    GLuint gameOver = 0;               // Game over screen texture
    GLuint sky = 0;                    // Sky texture
};

// Global instances
GameState game;
Textures textures;

/**
 * Creates a new obstacle at a random position on the road
 */
void spawnObstacle() {
    if (game.obstacles.size() < MAX_OBSTACLES) {
        GameState::Obstacle newObstacle;

        // Random X position across the road width (excluding edge areas)
        float roadDrivableWidth = ROAD_WIDTH - (2 * EDGE_WIDTH);
        newObstacle.x = -roadDrivableWidth / 2 + static_cast<float>(rand()) /
            (static_cast<float>(RAND_MAX / roadDrivableWidth));

        // Random Z position along the road (farther away than player)
        newObstacle.z = -20.0f - static_cast<float>(rand()) /
            (static_cast<float>(RAND_MAX / (ROAD_LENGTH - 20.0f)));

        newObstacle.active = true;

        game.obstacles.push_back(newObstacle);
    }
}

/**
 * Checks for collisions between player car and obstacles or road edges
 * @return true if collision detected, false otherwise
 */
bool checkCollision() {
    // Check collisions only with obstacles (road edges are handled separately)
    for (auto& obstacle : game.obstacles) {
        if (obstacle.active) {
            bool obstacleCollision = fabs(game.carX - obstacle.x) < 1.0 &&
                obstacle.z > -6 &&
                obstacle.z < -4;

            if (obstacleCollision) return true;
        }
    }

    return false;
}

/**
 * Checks if car is hitting road edges and applies bounce effect
 * @return true if car is bouncing, false otherwise
 */
bool checkEdgeCollisionAndBounce() {
    float leftEdgePosition = -ROAD_WIDTH / 2;
    float rightEdgePosition = ROAD_WIDTH / 2;
    float edgeThreshold = EDGE_WIDTH;
    bool isBouncing = false;

    // Check collision with left edge
    if (game.carX <= leftEdgePosition + edgeThreshold) {
        // Apply bounce force in positive X direction
        game.carVelocityX = BOUNCE_FACTOR * CAR_SPEED;
        game.carX = leftEdgePosition + edgeThreshold; // Prevent embedding in edge
        isBouncing = true;
    }
    // Check collision with right edge
    else if (game.carX >= rightEdgePosition - edgeThreshold) {
        // Apply bounce force in negative X direction
        game.carVelocityX = -BOUNCE_FACTOR * CAR_SPEED;
        game.carX = rightEdgePosition - edgeThreshold; // Prevent embedding in edge
        isBouncing = true;
    }

    return isBouncing;
}

/**
 * Processes texture data and generates OpenGL texture
 * @param data Image data from stb_image
 * @param width Image width
 * @param height Image height
 * @param nrChannels Number of color channels
 * @param isSkyTexture Special handling for sky texture
 * @return Generated texture ID
 */
GLuint processTexture(unsigned char* data, int width, int height, int nrChannels, bool isSkyTexture = false) {
    if (!data) {
        std::cout << "Failed to load image!" << std::endl;
        return 0;
    }

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    // Set texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
        (nrChannels == 4) ? GL_RGBA : GL_RGB,
        GL_UNSIGNED_BYTE, data);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Different wrapping for sky vs other textures
    if (isSkyTexture) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_LINEAR);
    }
    else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    // Free image data after texture is created
    stbi_image_free(data);
    return texID;
}

/**
 * Loads a texture from file
 * @param filename Path to the image file
 * @param isSkyTexture Special handling for sky texture
 * @return Generated texture ID
 */
GLuint loadTexture(const char* filename, bool isSkyTexture = false) {
    int width, height, nrChannels;

    if (isSkyTexture) {
        stbi_set_flip_vertically_on_load(true);
    }

    unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
    GLuint textureID = processTexture(data, width, height, nrChannels, isSkyTexture);

    return textureID;
}

/**
 * Initializes lighting for the scene
 */
void initLighting() {
    GLfloat lightPos[] = { 0.0, 10.0, 10.0, 1.0 };  // Position of light source
    GLfloat ambient[] = { 0.2, 0.2, 0.2, 1.0 };     // Ambient light intensity
    GLfloat diffuse[] = { 0.8, 0.8, 0.8, 1.0 };     // Diffuse light intensity

    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
}

/**
 * Draws the road with texture and green edges
 */
void drawGround() {
    // Road dimensions
    float roadHalfWidth = ROAD_WIDTH / 2.0f;
    float roadDepthNear = 10.0f;
    float roadDepthFar = -ROAD_LENGTH;

    // Draw textured center of road
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textures.road);

    glBegin(GL_QUADS);
    glColor3f(1, 1, 1);  // White to preserve texture colors
    // Use more texture repetitions for the longer road
    // Draw main road (slightly narrower to make room for edges)
    glTexCoord2f(0, 0); glVertex3f(-roadHalfWidth + EDGE_WIDTH, -1, roadDepthFar);
    glTexCoord2f(10, 0); glVertex3f(roadHalfWidth - EDGE_WIDTH, -1, roadDepthFar);
    glTexCoord2f(10, 50); glVertex3f(roadHalfWidth - EDGE_WIDTH, -1, roadDepthNear);
    glTexCoord2f(0, 50); glVertex3f(-roadHalfWidth + EDGE_WIDTH, -1, roadDepthNear);
    glEnd();

    glDisable(GL_TEXTURE_2D);

    // Draw green edges
    glColor3f(0.0, 0.8, 0.0);  // Bright green color

    // Left edge
    glBegin(GL_QUADS);
    glVertex3f(-roadHalfWidth, -0.9, roadDepthFar);
    glVertex3f(-roadHalfWidth + EDGE_WIDTH, -0.9, roadDepthFar);
    glVertex3f(-roadHalfWidth + EDGE_WIDTH, -0.9, roadDepthNear);
    glVertex3f(-roadHalfWidth, -0.9, roadDepthNear);
    glEnd();

    // Right edge
    glBegin(GL_QUADS);
    glVertex3f(roadHalfWidth - EDGE_WIDTH, -0.9, roadDepthFar);
    glVertex3f(roadHalfWidth, -0.9, roadDepthFar);
    glVertex3f(roadHalfWidth, -0.9, roadDepthNear);
    glVertex3f(roadHalfWidth - EDGE_WIDTH, -0.9, roadDepthNear);
    glEnd();

    // Add some depth to the edges (vertical sides)
    glColor3f(0.0, 0.6, 0.0);  // Slightly darker green for sides

    // Left edge side
    glBegin(GL_QUADS);
    glVertex3f(-roadHalfWidth + EDGE_WIDTH, -1.0, roadDepthFar);
    glVertex3f(-roadHalfWidth + EDGE_WIDTH, -0.9, roadDepthFar);
    glVertex3f(-roadHalfWidth + EDGE_WIDTH, -0.9, roadDepthNear);
    glVertex3f(-roadHalfWidth + EDGE_WIDTH, -1.0, roadDepthNear);
    glEnd();

    // Right edge side
    glBegin(GL_QUADS);
    glVertex3f(roadHalfWidth - EDGE_WIDTH, -1.0, roadDepthFar);
    glVertex3f(roadHalfWidth - EDGE_WIDTH, -0.9, roadDepthFar);
    glVertex3f(roadHalfWidth - EDGE_WIDTH, -0.9, roadDepthNear);
    glVertex3f(roadHalfWidth - EDGE_WIDTH, -1.0, roadDepthNear);
    glEnd();
}

/**
 * Draws the sky box with texture
 */
void drawSky() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textures.sky);

    // Disable lighting for sky (constant brightness)
    glDisable(GL_LIGHTING);

    // Make the sky box larger to match the longer road
    float skySize = ROAD_LENGTH * 1.5f;
    float skyHalfSize = skySize / 2.0f;

    // Draw sky box (5 visible faces)
    // Back face
    glBegin(GL_QUADS);
    glColor3f(1, 1, 1);
    glTexCoord2f(0, 0); glVertex3f(-skyHalfSize, -skyHalfSize, -skyHalfSize);
    glTexCoord2f(1, 0); glVertex3f(skyHalfSize, -skyHalfSize, -skyHalfSize);
    glTexCoord2f(1, 1); glVertex3f(skyHalfSize, skyHalfSize, -skyHalfSize);
    glTexCoord2f(0, 1); glVertex3f(-skyHalfSize, skyHalfSize, -skyHalfSize);
    glEnd();

    // Left face
    glBegin(GL_QUADS);
    glColor3f(1, 1, 1);
    glTexCoord2f(0, 0); glVertex3f(-skyHalfSize, -skyHalfSize, skyHalfSize);
    glTexCoord2f(1, 0); glVertex3f(-skyHalfSize, -skyHalfSize, -skyHalfSize);
    glTexCoord2f(1, 1); glVertex3f(-skyHalfSize, skyHalfSize, -skyHalfSize);
    glTexCoord2f(0, 1); glVertex3f(-skyHalfSize, skyHalfSize, skyHalfSize);
    glEnd();

    // Right face
    glBegin(GL_QUADS);
    glColor3f(1, 1, 1);
    glTexCoord2f(0, 0); glVertex3f(skyHalfSize, -skyHalfSize, -skyHalfSize);
    glTexCoord2f(1, 0); glVertex3f(skyHalfSize, -skyHalfSize, skyHalfSize);
    glTexCoord2f(1, 1); glVertex3f(skyHalfSize, skyHalfSize, skyHalfSize);
    glTexCoord2f(0, 1); glVertex3f(skyHalfSize, skyHalfSize, -skyHalfSize);
    glEnd();

    // Top face
    glBegin(GL_QUADS);
    glColor3f(1, 1, 1);
    glTexCoord2f(0, 0); glVertex3f(-skyHalfSize, skyHalfSize, -skyHalfSize);
    glTexCoord2f(1, 0); glVertex3f(skyHalfSize, skyHalfSize, -skyHalfSize);
    glTexCoord2f(1, 1); glVertex3f(skyHalfSize, skyHalfSize, skyHalfSize);
    glTexCoord2f(0, 1); glVertex3f(-skyHalfSize, skyHalfSize, skyHalfSize);
    glEnd();

    // Bottom face
    glBegin(GL_QUADS);
    glColor3f(1, 1, 1);
    glTexCoord2f(0, 0); glVertex3f(-skyHalfSize, -skyHalfSize, skyHalfSize);
    glTexCoord2f(1, 0); glVertex3f(skyHalfSize, -skyHalfSize, skyHalfSize);
    glTexCoord2f(1, 1); glVertex3f(skyHalfSize, -skyHalfSize, -skyHalfSize);
    glTexCoord2f(0, 1); glVertex3f(-skyHalfSize, -skyHalfSize, -skyHalfSize);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
}

void drawCarModel(float r, float g, float b) {
    // Car body (bottom part)
    glColor3f(r, g, b);
    glPushMatrix();
    glScalef(1.0, 0.4, 2.0);
    glutSolidCube(1.0);
    glPopMatrix();

    // Car cabin (top part)
    glColor3f(r * 0.8, g * 0.8, b * 0.8);  // Slightly darker for cabin
    glPushMatrix();
    glTranslatef(0.0, 0.3, 0.0);
    glScalef(0.8, 0.3, 1.0);
    glutSolidCube(1.0);
    glPopMatrix();

    // Wheels (black)
    glColor3f(0.2, 0.2, 0.2);

    // Front-left wheel
    glPushMatrix();
    glTranslatef(-0.5, -0.3, 0.7);
    glutSolidTorus(0.1, 0.2, 8, 8);
    glPopMatrix();

    // Front-right wheel
    glPushMatrix();
    glTranslatef(0.5, -0.3, 0.7);
    glutSolidTorus(0.1, 0.2, 8, 8);
    glPopMatrix();

    // Rear-left wheel
    glPushMatrix();
    glTranslatef(-0.5, -0.3, -0.7);
    glutSolidTorus(0.1, 0.2, 8, 8);
    glPopMatrix();

    // Rear-right wheel
    glPushMatrix();
    glTranslatef(0.5, -0.3, -0.7);
    glutSolidTorus(0.1, 0.2, 8, 8);
    glPopMatrix();
}

/**
 * Draws the player's car (red)
 */
void drawPlayerCar() {
    glPushMatrix();
    glTranslatef(game.carX, -0.5, -5);
    glRotatef(180, 0, 1, 0);  // Face the car forward
    drawCarModel(1.0, 0.0, 0.0);  // Red car
    glPopMatrix();
}

/**
 * Draws all active obstacle cars
 */
void drawObstacleCars() {
    for (auto& obstacle : game.obstacles) {
        if (obstacle.active) {
            glPushMatrix();
            glTranslatef(obstacle.x, -0.5, obstacle.z);
            // Randomize obstacle car colors a bit for variety
            float g = 0.7f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) * 1.5f);
            drawCarModel(0.0, g, 1.0 - g);
            glPopMatrix();
        }
    }
}

/**
 * Draws the game over screen
 */
void drawGameOverScreen() {
    // Switch to 2D orthographic projection for UI
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Disable lighting for UI elements
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textures.gameOver);

    // Draw game over texture in the center of the screen
    glColor3f(1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(200, 150);
    glTexCoord2f(1, 0); glVertex2f(600, 150);
    glTexCoord2f(1, 1); glVertex2f(600, 450);
    glTexCoord2f(0, 1); glVertex2f(200, 450);
    glEnd();

    glDisable(GL_TEXTURE_2D);

    // Restore previous matrices
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

/**
 * Main display function called by GLUT
 */
void display() {
    // Clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Set camera position and orientation
    glRotatef(game.camYaw, 0.0f, 1.0f, 0.0f);
    gluLookAt(game.camX, game.camY, game.camZ,  // Camera position
        game.carX, 0.0f, -5.0f,           // Look at point (player car)
        0.0f, 1.0f, 0.0f);                // Up vector

    // Draw scene elements
    drawSky();
    drawGround();
    drawPlayerCar();
    drawObstacleCars();

    // Check for collision with obstacles if game is active
    if (!game.gameOver && !game.isPaused && checkCollision()) {
        std::cout << "Collision with obstacle detected!" << std::endl;
        game.gameOver = true;
    }

    // Draw game over screen if needed
    if (game.gameOver) {
        drawGameOverScreen();
    }

    // Swap buffers to display the rendered image
    glutSwapBuffers();
}

/**
 * Handle window resize events
 * @param w New window width
 * @param h New window height
 */
void reshape(int w, int h) {
    if (h == 0) h = 1;  // Prevent division by zero
    float ratio = 1.0f * w / h;

    // Set up projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Increase far clipping plane to see the longer road
    gluPerspective(60, ratio, 1, ROAD_LENGTH * 2);

    // Return to modelview matrix
    glMatrixMode(GL_MODELVIEW);

    // Adjust viewport to window dimensions
    glViewport(0, 0, w, h);
}

/**
 * Timer function for game updates
 * @param value Timer ID (unused)
 */
void timer(int value) {
    if (!game.gameOver && !game.isPaused) {
        // Update obstacle spawn timer
        game.timeSinceLastSpawn += 0.016f;  // ~60 FPS

        // Spawn new obstacle when time interval has passed
        if (game.timeSinceLastSpawn >= SPAWN_INTERVAL) {
            spawnObstacle();  // Create new random obstacle
            game.timeSinceLastSpawn = 0.0f;  // Reset timer
        }

        // Apply bounce physics if car is hitting edge
        checkEdgeCollisionAndBounce();

        // Apply car's velocity from bounce effect
        game.carX += game.carVelocityX;

        // Dampen velocity over time for realistic physics
        if (fabs(game.carVelocityX) > 0.01f) {
            game.carVelocityX *= 0.9f;  // Reduce velocity by 10% each frame
        }
        else {
            game.carVelocityX = 0.0f;  // Stop completely when very slow
        }

        // Move all obstacles toward the player
        for (auto& obstacle : game.obstacles) {
            if (obstacle.active) {
                obstacle.z += OBSTACLE_SPEED;

                // Remove obstacles that pass the player
                if (obstacle.z > 10.0f) {
                    obstacle.active = false;
                }
            }
        }

        // Clean up inactive obstacles occasionally
        if (game.obstacles.size() > MAX_OBSTACLES / 2) {
            auto it = game.obstacles.begin();
            while (it != game.obstacles.end()) {
                if (!it->active) {
                    it = game.obstacles.erase(it);
                }
                else {
                    ++it;
                }
            }
        }
    }

    // Request redisplay and schedule next update
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);  // ~60 FPS
}

/**
 * Handle regular keyboard input
 * @param key Key pressed
 * @param x Mouse x position (unused)
 * @param y Mouse y position (unused)
 */
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case 27:  // ESC key
        exit(0);
        break;

    case 'a': case 'A':
        game.camYaw += CAMERA_SPEED;
        break;

    case 'd': case 'D':
        game.camYaw -= CAMERA_SPEED;
        break;

    case 'w': case 'W':
        game.camY += CAMERA_SPEED;  // Move camera up
        break;

    case 's': case 'S':
        game.camY -= CAMERA_SPEED;  // Move camera down
        break;

    case 'p': case 'P':
        game.isPaused = !game.isPaused;  // Toggle pause
        break;

    case 'r': case 'R':
        // Reset game state
        game.carX = 0.0f;
        game.carVelocityX = 0.0f;
        game.obstacles.clear();
        game.gameOver = false;
        game.isPaused = false;
        break;

    case 'f': case 'F':
        game.isFullScreen = !game.isFullScreen;  // Toggle fullscreen
        if (game.isFullScreen) {
            glutFullScreen();
        }
        else {
            glutReshapeWindow(800, 600);
        }
        break;
    }
}

/**
 * Handle special keyboard input (arrow keys, etc.)
 * @param key Special key pressed
 * @param x Mouse x position (unused)
 * @param y Mouse y position (unused)
 */
void specialKeys(int key, int x, int y) {
    if (!game.gameOver && !game.isPaused) {
        switch (key) {
        case GLUT_KEY_LEFT:
            // Apply leftward velocity directly to the car
            game.carVelocityX -= CAR_SPEED * 0.5f;
            break;

        case GLUT_KEY_RIGHT:
            // Apply rightward velocity directly to the car
            game.carVelocityX += CAR_SPEED * 0.5f;
            break;

        case GLUT_KEY_F1:
            game.isFullScreen = !game.isFullScreen;
            if (game.isFullScreen) {
                glutFullScreen();
            }
            else {
                glutReshapeWindow(800, 600);
            }
            break;
        }
    }
}

/**
 * Handle mouse input
 * @param button Mouse button pressed
 * @param state Button state (up/down)
 * @param x Mouse x position (unused)
 * @param y Mouse y position (unused)
 */
void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (game.gameOver) {
            // Restart game if game over
            game.carX = 0.0f;
            game.carVelocityX = 0.0f;
            game.obstacles.clear();
            game.gameOver = false;
        }
        else {
            game.isPaused = !game.isPaused;  // Toggle pause with left click
        }
    }
}

/**
 * Initialize OpenGL settings and game resources
 */
void init() {
    // Enable depth testing for 3D rendering
    glEnable(GL_DEPTH_TEST);

    // Set up lighting
    initLighting();

    // Initialize random seed for obstacle positions
    srand(static_cast<unsigned int>(time(NULL)));

    // Load textures
    textures.road = loadTexture("road.jpg");
    textures.gameOver = loadTexture("game_over.jpg", true);
    textures.sky = loadTexture("sky.jpg", true);

    // Initialize obstacles container
    game.obstacles.reserve(MAX_OBSTACLES);

    // Spawn initial obstacles
    for (int i = 0; i < 5; i++) {
        spawnObstacle();
    }
}

/**
 * Main function - program entry point
 */
int main(int argc, char** argv) {
    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
   // glutInitWindowSize(800, 600);
    
    glutCreateWindow("3omda Racing Game");
    glutFullScreen();
    
    // Initialize game
    init();

    // Register GLUT callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouse);
    glutTimerFunc(0, timer, 0);

    // Enter main loop
    glutMainLoop();
    return 0;
}