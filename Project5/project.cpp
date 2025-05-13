#define STB_IMAGE_IMPLEMENTATION
#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES 

#include <GL/stb_image.h>
#include <GL/glut.h>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <string>

// Game constants
const float ROAD_WIDTH = 10.0f;
const float ROAD_LENGTH = 500.0f;
const float EDGE_WIDTH = 1.0f;
const float PLAYER_SPEED = 0.2f;
const float BOUNCE_FACTOR = 0.8f;
const float OBSTACLE_SPEED = 0.4f;
const float SPAWN_TIME = 2.0f;
const float CAM_SPEED = 0.2f;
const int MAX_OBSTACLES = 10;

// Game state
struct Game {
    // Player state
    float playerX = 0.0f;
    float playerVelX = 0.0f;

    // Camera position
    float camX = 0.0f;
    float camY = 3.0f;
    float camZ = 5.0f;
    float camAngle = 0.0f;

    // Game status
    bool isOver = false;
    bool isPaused = false;
    bool isFullscreen = false;
    float spawnTimer = 0.0f;
    int score = 0;  // Player's score

    // Obstacles
    struct Obstacle {
        float x;          // X position
        float z;          // Z position
        bool active;      // Is active
        bool counted;     // Has been counted for score
    };
    std::vector<Obstacle> obstacles;
};

// Textures
struct TextureSet {
    GLuint road = 0;
    GLuint gameOver = 0;
    GLuint sky = 0;
};

// Global instances
Game game;
TextureSet textures;

// Creates a new obstacle at random position
void spawnObstacle() {
    if (game.obstacles.size() < MAX_OBSTACLES) {
        Game::Obstacle newObstacle;
        float driveWidth = ROAD_WIDTH - (2 * EDGE_WIDTH);

        // Random position across road
        newObstacle.x = -driveWidth / 2 + static_cast<float>(rand()) /
            (static_cast<float>(RAND_MAX / driveWidth));

        // Random distance
        newObstacle.z = -20.0f - static_cast<float>(rand()) /
            (static_cast<float>(RAND_MAX / (ROAD_LENGTH - 20.0f)));

        newObstacle.active = true;
        newObstacle.counted = false;  // Initialize counted status
        game.obstacles.push_back(newObstacle);
    }
}

// Check for collisions with obstacles
bool checkCollision() {
    for (auto& obstacle : game.obstacles) {
        if (obstacle.active) {
            bool collision = fabs(game.playerX - obstacle.x) < 1.0 &&
                obstacle.z > -6 &&
                obstacle.z < -4;

            if (collision) return true;
        }
    }
    return false;
}

// Handle road edge collisions with bounce effect
bool handleEdgeCollision() {
    float leftEdge = -ROAD_WIDTH / 2;
    float rightEdge = ROAD_WIDTH / 2;
    bool isBouncing = false;

    // Left edge collision
    if (game.playerX <= leftEdge + EDGE_WIDTH) {
        game.playerVelX = BOUNCE_FACTOR * PLAYER_SPEED;
        game.playerX = leftEdge + EDGE_WIDTH; // Prevent embedding
        isBouncing = true;
    }
    // Right edge collision
    else if (game.playerX >= rightEdge - EDGE_WIDTH) {
        game.playerVelX = -BOUNCE_FACTOR * PLAYER_SPEED;
        game.playerX = rightEdge - EDGE_WIDTH; // Prevent embedding
        isBouncing = true;
    }

    return isBouncing;
}

// Process texture data and generate OpenGL texture
GLuint processTexture(unsigned char* data, int width, int height, int channels, bool isSky = false) {
    if (!data) {
        std::cout << "Failed to load image!" << std::endl;
        return 0;
    }

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    // Set texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
        (channels == 4) ? GL_RGBA : GL_RGB,
        GL_UNSIGNED_BYTE, data);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Wrapping mode
    if (isSky) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_LINEAR);
    }
    else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    stbi_image_free(data);
    return texID;
}

// Load texture from file
GLuint loadTexture(const char* filename, bool isSky = false) {
    int width, height, channels;

    if (isSky) {
        stbi_set_flip_vertically_on_load(true);
    }

    unsigned char* data = stbi_load(filename, &width, &height, &channels, 0);
    return processTexture(data, width, height, channels, isSky);
}

// Initialize lighting
void setupLighting() {
    GLfloat lightPos[] = { 0.0, 10.0, 10.0, 1.0 };
    GLfloat ambient[] = { 0.2, 0.2, 0.2, 1.0 };
    GLfloat diffuse[] = { 0.8, 0.8, 0.8, 1.0 };

    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
}

// Draw road with texture and edges
void drawRoad() {
    float halfWidth = ROAD_WIDTH / 2.0f;
    float nearZ = 10.0f;
    float farZ = -ROAD_LENGTH;

    // Draw textured road surface
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textures.road);

    glBegin(GL_QUADS);
    glColor3f(1, 1, 1);  // White for texture
    glTexCoord2f(0, 0); glVertex3f(-halfWidth + EDGE_WIDTH, -1, farZ);
    glTexCoord2f(10, 0); glVertex3f(halfWidth - EDGE_WIDTH, -1, farZ);
    glTexCoord2f(10, 50); glVertex3f(halfWidth - EDGE_WIDTH, -1, nearZ);
    glTexCoord2f(0, 50); glVertex3f(-halfWidth + EDGE_WIDTH, -1, nearZ);
    glEnd();

    glDisable(GL_TEXTURE_2D);

    // Draw green edges
    glColor3f(0.0, 0.8, 0.0);

    // Left edge
    glBegin(GL_QUADS);
    glVertex3f(-halfWidth, -0.9, farZ);
    glVertex3f(-halfWidth + EDGE_WIDTH, -0.9, farZ);
    glVertex3f(-halfWidth + EDGE_WIDTH, -0.9, nearZ);
    glVertex3f(-halfWidth, -0.9, nearZ);
    glEnd();

    // Right edge
    glBegin(GL_QUADS);
    glVertex3f(halfWidth - EDGE_WIDTH, -0.9, farZ);
    glVertex3f(halfWidth, -0.9, farZ);
    glVertex3f(halfWidth, -0.9, nearZ);
    glVertex3f(halfWidth - EDGE_WIDTH, -0.9, nearZ);
    glEnd();

    // Edge sides (for depth)
    glColor3f(0.0, 0.6, 0.0);

    // Left edge side
    glBegin(GL_QUADS);
    glVertex3f(-halfWidth + EDGE_WIDTH, -1.0, farZ);
    glVertex3f(-halfWidth + EDGE_WIDTH, -0.9, farZ);
    glVertex3f(-halfWidth + EDGE_WIDTH, -0.9, nearZ);
    glVertex3f(-halfWidth + EDGE_WIDTH, -1.0, nearZ);
    glEnd();

    // Right edge side
    glBegin(GL_QUADS);
    glVertex3f(halfWidth - EDGE_WIDTH, -1.0, farZ);
    glVertex3f(halfWidth - EDGE_WIDTH, -0.9, farZ);
    glVertex3f(halfWidth - EDGE_WIDTH, -0.9, nearZ);
    glVertex3f(halfWidth - EDGE_WIDTH, -1.0, nearZ);
    glEnd();
}

// Draw skybox
void drawSkybox() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textures.sky);
    glDisable(GL_LIGHTING);

    float skySize = ROAD_LENGTH * 1.5f;
    float halfSize = skySize / 2.0f;

    // Draw 5 faces of the skybox
    glColor3f(1, 1, 1);

    // Back face
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-halfSize, -halfSize, -halfSize);
    glTexCoord2f(1, 0); glVertex3f(halfSize, -halfSize, -halfSize);
    glTexCoord2f(1, 1); glVertex3f(halfSize, halfSize, -halfSize);
    glTexCoord2f(0, 1); glVertex3f(-halfSize, halfSize, -halfSize);
    glEnd();

    // Left face
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-halfSize, -halfSize, halfSize);
    glTexCoord2f(1, 0); glVertex3f(-halfSize, -halfSize, -halfSize);
    glTexCoord2f(1, 1); glVertex3f(-halfSize, halfSize, -halfSize);
    glTexCoord2f(0, 1); glVertex3f(-halfSize, halfSize, halfSize);
    glEnd();

    // Right face
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(halfSize, -halfSize, -halfSize);
    glTexCoord2f(1, 0); glVertex3f(halfSize, -halfSize, halfSize);
    glTexCoord2f(1, 1); glVertex3f(halfSize, halfSize, halfSize);
    glTexCoord2f(0, 1); glVertex3f(halfSize, halfSize, -halfSize);
    glEnd();

    // Top face
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-halfSize, halfSize, -halfSize);
    glTexCoord2f(1, 0); glVertex3f(halfSize, halfSize, -halfSize);
    glTexCoord2f(1, 1); glVertex3f(halfSize, halfSize, halfSize);
    glTexCoord2f(0, 1); glVertex3f(-halfSize, halfSize, halfSize);
    glEnd();

    // Bottom face
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-halfSize, -halfSize, halfSize);
    glTexCoord2f(1, 0); glVertex3f(halfSize, -halfSize, halfSize);
    glTexCoord2f(1, 1); glVertex3f(halfSize, -halfSize, -halfSize);
    glTexCoord2f(0, 1); glVertex3f(-halfSize, -halfSize, -halfSize);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
}

// Draw a car model with specified color
void drawCar(float r, float g, float b) {
    // Car body
    glColor3f(r, g, b);
    glPushMatrix();
    glScalef(1.0, 0.4, 2.0);
    glutSolidCube(1.0);
    glPopMatrix();

    // Car cabin
    glColor3f(r * 0.8, g * 0.8, b * 0.8);
    glPushMatrix();
    glTranslatef(0.0, 0.3, 0.0);
    glScalef(0.8, 0.3, 1.0);
    glutSolidCube(1.0);
    glPopMatrix();

    // Wheels
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

// Draw player's car
void drawPlayer() {
    glPushMatrix();
    glTranslatef(game.playerX, -0.5, -5);
    glRotatef(180, 0, 1, 0);  // Face forward
    drawCar(1.0, 0.0, 0.0);   // Red car
    glPopMatrix();
}

// Draw all active obstacles
void drawObstacles() {
    for (auto& obstacle : game.obstacles) {
        if (obstacle.active) {
            glPushMatrix();
            glTranslatef(obstacle.x, -0.5, obstacle.z);
            // Randomize color slightly
            float g = 0.7f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) * 1.5f);
            drawCar(0.0, g, 1.0 - g);
            glPopMatrix();
        }
    }
}

// Draw score display
void drawScore() {
    // Switch to 2D projection for UI
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    // Display score text
    glColor3f(1.0, 1.0, 1.0);
    glRasterPos2f(20, 560);
    std::string scoreText = "Score: " + std::to_string(game.score);

    for (char c : scoreText) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }

    // Restore matrices
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_LIGHTING);
}

// Draw game over screen
void showGameOver() {
    // Switch to 2D projection for UI
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textures.gameOver);

    // Center of screen
    glColor3f(1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(200, 150);
    glTexCoord2f(1, 0); glVertex2f(600, 150);
    glTexCoord2f(1, 1); glVertex2f(600, 450);
    glTexCoord2f(0, 1); glVertex2f(200, 450);
    glEnd();

    glDisable(GL_TEXTURE_2D);

    // Display final score
    glColor3f(1.0, 1.0, 0.0); // Yellow for final score
    glRasterPos2f(350, 120);
    std::string finalScore = "Final Score: " + std::to_string(game.score);

    for (char c : finalScore) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }

    // Restore matrices
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// Main display function
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Camera positioning
    glRotatef(game.camAngle, 0.0f, 1.0f, 0.0f);
    gluLookAt(
        game.camX, game.camY, game.camZ,    // Camera position
        game.playerX, 0.0f, -5.0f,          // Look at player
        0.0f, 1.0f, 0.0f                    // Up vector
    );

    // Draw scene elements
    drawSkybox();
    drawRoad();
    drawPlayer();
    drawObstacles();

    // Draw score
    drawScore();

    // Check for collision if game is active
    if (!game.isOver && !game.isPaused && checkCollision()) {
        std::cout << "Collision detected!" << std::endl;
        game.isOver = true;
    }

    // Show game over screen if needed
    if (game.isOver) {
        showGameOver();
    }

    glutSwapBuffers();
}

// Handle window resize
void reshape(int w, int h) {
    if (h == 0) h = 1;
    float ratio = 1.0f * w / h;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50, ratio, 1, ROAD_LENGTH * 2);

    glMatrixMode(GL_MODELVIEW);
    glViewport(0, 0, w, h);
}

// Update score when an obstacle passes the player
void updateScore() {
    const float playerZ = -5.0f;

    for (auto& obstacle : game.obstacles) {
        if (obstacle.active && !obstacle.counted && obstacle.z > playerZ) {
            game.score++;
            obstacle.counted = true;
            std::cout << "Score: " << game.score << std::endl;
        }
    }
}

// Timer function for game updates
void timer(int value) {
    if (!game.isOver && !game.isPaused) {
        // Update obstacle spawn timer (60 FPS)
        game.spawnTimer += 0.016f;

        // Spawn new obstacle when time interval passes
        if (game.spawnTimer >= SPAWN_TIME) {
            spawnObstacle();
            game.spawnTimer = 0.0f;
        }

        // Handle edge collisions
        handleEdgeCollision();

        // Apply velocity from bounce effect
        game.playerX += game.playerVelX;

        // Dampen velocity over time
        if (fabs(game.playerVelX) > 0.01f) {
            game.playerVelX *= 0.9f;  // Reduce by 10% each frame
        }
        else {
            game.playerVelX = 0.0f;   // Stop when very slow
        }

        // Move obstacles toward player
        for (auto& obstacle : game.obstacles) {
            if (obstacle.active) {
                obstacle.z += OBSTACLE_SPEED;

                // Remove passed obstacles
                if (obstacle.z > 10.0f) {
                    obstacle.active = false;
                }
            }
        }

        // Update score when obstacles pass
        updateScore();

        // Clean up inactive obstacles
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

    glutPostRedisplay();
    glutTimerFunc(1000/30, timer, 0);  // ~60 FPS
}

// Handle keyboard input
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case 27:  // ESC key
        exit(0);
        break;

    case 'a': case 'A':
        game.camAngle += CAM_SPEED;
        break;

    case 'd': case 'D':
        game.camAngle -= CAM_SPEED;
        break;

    case 'w': case 'W':
        game.camY += CAM_SPEED;  // Camera up
        break;

    case 's': case 'S':
        game.camY -= CAM_SPEED;  // Camera down
        break;

    case 'p': case 'P':
        game.isPaused = !game.isPaused;  // Toggle pause
        break;

    case 'r': case 'R':
        // Reset game
        game.playerX = 0.0f;
        game.playerVelX = 0.0f;
        game.obstacles.clear();
        game.isOver = false;
        game.isPaused = false;
        game.score = 0;  // Reset score
        break;

    case 'f': case 'F':
        game.isFullscreen = !game.isFullscreen;
        if (game.isFullscreen) {
            glutFullScreen();
        }
        else {
            glutReshapeWindow(800, 600);
        }
        break;
    }
}

// Handle special keys (arrows)
void specialKeys(int key, int x, int y) {
    if (!game.isOver && !game.isPaused) {
        switch (key) {
        case GLUT_KEY_LEFT:
            game.playerVelX -= PLAYER_SPEED * 0.5f;
            break;

        case GLUT_KEY_RIGHT:
            game.playerVelX += PLAYER_SPEED * 0.5f;
            break;

        case GLUT_KEY_F1:
            game.isFullscreen = !game.isFullscreen;
            if (game.isFullscreen) {
                glutFullScreen();
            }
            else {
                glutReshapeWindow(800, 600);
            }
            break;
        }
    }
}

// Handle mouse input
void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (game.isOver) {
            // Restart game
            game.playerX = 0.0f;
            game.playerVelX = 0.0f;
            game.obstacles.clear();
            game.isOver = false;
            game.score = 0;  // Reset score on restart
        }
        else {
            game.isPaused = !game.isPaused;  // Toggle pause
        }
    }
}

// Initialize game
void init() {
    glEnable(GL_DEPTH_TEST);
    setupLighting();

    // Initialize random seed
    srand(static_cast<unsigned int>(time(NULL)));

    // Load textures
    textures.road = loadTexture("road.jpg");
    textures.gameOver = loadTexture("game_over.jpg", true);
    textures.sky = loadTexture("sky.jpg", true);

    // Initialize obstacles
    game.obstacles.reserve(MAX_OBSTACLES);

    // Spawn initial obstacles
    for (int i = 0; i < 5; i++) {
        spawnObstacle();
    }
}

// Program entry point
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutCreateWindow("Racing Game");
    glutFullScreen();

    init();

    // Register callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouse);
    glutTimerFunc(0, timer, 0);

    glutMainLoop();
    return 0;
}