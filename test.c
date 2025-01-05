#include <SDL.h>
#include <stdbool.h>
#include <stdio.h>

#define MAX_PLATFORMS 12
#define PLATFORM_WIDTH 100
#define PLATFORM_HEIGHT 20
#define GRAVITY 1 // Gravity constant
#define JUMP_FORCE -15 // Jump force
#define WINDOW_HEIGHT 600 // Window height

// Global camera offset
int cameraX = 0;

// Define batarong properties
typedef struct {
    int x, y;
    int width, height;
    SDL_Texture* texture; // Texture for the player
    int velocityY; // Vertical velocity for gravity
    bool onGround; // Check if the player is on the ground
} Batarong;

// Define platform properties
typedef struct {
    int x, y;
    SDL_Rect rect; // Rectangle for the platform
} Platform;

Platform platforms[MAX_PLATFORMS] = {
    {100, 500, {100, 500, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, // Platform 1
    {300, 400, {300, 400, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, // Platform 2
    {500, 300, {500, 300, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, // Platform 3
    {200, 200, {200, 200, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, // Platform 4
    {300, 500, {300, 500, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, // Platform 
    {400, 500, {400, 500, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, // Platform
    {500, 500, {500, 500, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, // Platform  
    {500, 600, {500, 600, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, // Platform  
    {500, 700, {500, 700, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, // Platform  
    {600, 500, {600, 500, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, // Platform 
    {700, 500, {700, 500, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, // Platform
    {400, 100, {400, 100, PLATFORM_WIDTH, PLATFORM_HEIGHT}}  // Platform 5
};

int platformCount = MAX_PLATFORMS;

// Function prototypes
void handleInput(bool* running, Batarong* batarong, bool* gameOver);
void applyGravity(Batarong* batarong);
bool checkCollision(Batarong* batarong, bool* gameOver);
void renderPlatforms(SDL_Renderer* renderer);
void renderGameOver(SDL_Renderer* renderer);

void handleInput(bool* running, Batarong* batarong, bool* gameOver) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            *running = false; // Exit the loop if the window is closed
        }
    }

    // Get the current state of the keyboard
    const Uint8* state = SDL_GetKeyboardState(NULL);

    // Handle keyboard input for movement
    if (!*gameOver) {
        if (state[SDL_SCANCODE_UP]) {
            if (batarong->onGround) {
                batarong->velocityY = JUMP_FORCE; // Jump if on the ground
                batarong->onGround = false; // Player is now in the air
            }
        }
        if (state[SDL_SCANCODE_LEFT]) {
            batarong->x -= 5; // Move left
        }
        if (state[SDL_SCANCODE_RIGHT]) {
            batarong->x += 5; // Move right
        }
    } else {
        // Check for restart input
        if (state[SDL_SCANCODE_R]) {
            *gameOver = false; // Reset game over state
            batarong->x = 300; // Reset player position
            batarong->y = 400; // Reset player position
            batarong->velocityY = 0; // Reset vertical velocity
            batarong->onGround = true; // Reset on ground status
        }
    }
}

void applyGravity(Batarong* batarong) {
    if (!batarong->onGround) {
        batarong->velocityY += GRAVITY; // Apply gravity
        batarong->y += batarong->velocityY; // Update player position
    }
}

bool checkCollision(Batarong* batarong, bool* gameOver) {
    // Reset onGround status
    batarong->onGround = false;

    // Check for collision with platforms
    for (int i = 0; i < platformCount; i++) {
        // Calculate the next position of the batarong
        int nextY = batarong->y + batarong->velocityY + GRAVITY;

        // Check if the player is falling onto the platform
        if (batarong->x < platforms[i].x + PLATFORM_WIDTH &&
            batarong->x + batarong->width > platforms[i].x &&
            nextY + batarong->height >= platforms[i].y &&
            nextY <= platforms[i].y + PLATFORM_HEIGHT) {
            // Collision detected
            batarong->y = platforms[i].y - batarong->height; // Place player on top of the platform
            batarong->onGround = true; // Player is on the ground
            batarong->velocityY = 0; // Reset vertical velocity
            break;
        }
    }

    // Check if the player has fallen below the bottom of the window
    if (batarong->y > WINDOW_HEIGHT) {
        *gameOver = true; // Set game over state
    }

    return batarong->onGround;
}

void renderPlatforms(SDL_Renderer* renderer) {
    for (int i = 0; i < platformCount; i++) {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green color for platforms
        // Adjust platform position based on camera
        SDL_Rect platformRect = { platforms[i].x - cameraX, platforms[i].y, PLATFORM_WIDTH, PLATFORM_HEIGHT };
        SDL_RenderFillRect(renderer, &platformRect); // Draw the platform
    }
}

void renderGameOver(SDL_Renderer* renderer) {
    // Render game over text
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red color for game over text
    SDL_Rect gameOverRect = { 200, 250, 400, 100 }; // Position and size of the game over rectangle
    SDL_RenderFillRect(renderer, &gameOverRect); // Draw the game over rectangle

    // Here you can add text rendering for "Game Over" and "Press R to Restart"
    // For simplicity, we will just draw a rectangle as a placeholder
}

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    // Create a window
    SDL_Window* window = SDL_CreateWindow("2D Game", 
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
        800, 600, SDL_WINDOW_SHOWN); // Create a window

    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create a renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0); // Create a renderer

    if (renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Load the background image
    SDL_Surface* bgSurface = SDL_LoadBMP("images/bliss.bmp");
    // Create a texture from the background surface
    SDL_Texture* bgTexture = SDL_CreateTextureFromSurface(renderer, bgSurface);
    SDL_FreeSurface(bgSurface); // Free the temporary surface

    if (bgTexture == NULL) {
        printf("Unable to create background texture! SDL Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Load the player image
    SDL_Surface* tempSurface = SDL_LoadBMP("images/batarong.bmp");
    if (tempSurface == NULL) {
        printf("Unable to load image! SDL Error: %s\n", SDL_GetError());
        SDL_DestroyTexture(bgTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Create a texture from the surface
    Batarong batarong = {300, 400, tempSurface->w, tempSurface->h, SDL_CreateTextureFromSurface(renderer, tempSurface), 0, true}; // Spawn on Platform 1
    SDL_FreeSurface(tempSurface); // Free the temporary surface

    if (batarong.texture == NULL) {
        printf("Unable to create texture! SDL Error: %s\n", SDL_GetError());
        SDL_DestroyTexture(bgTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool running = true;
    bool gameOver = false; // Game over state

    // Frame rate control
    const int targetFPS = 30;
    const int frameDelay = 1000 / targetFPS; // Delay in milliseconds

    while (running) {
        Uint32 frameStart = SDL_GetTicks(); // Get the start time of the frame

        // Handle input
        handleInput(&running, &batarong, &gameOver);

        if (!gameOver) {
            // Apply gravity
            applyGravity(&batarong);

            // Check for collisions with platforms
            checkCollision(&batarong, &gameOver);
        }

        // Update camera position to follow the player
        cameraX = batarong.x - (800 / 2); // Center the camera on the player

        // Clear the screen
        SDL_RenderClear(renderer);

        // Render the background texture (scaled to fit the window)
        SDL_Rect bgRect = { 0, 0, 800, 600 }; // Set the background rectangle to the window size
        SDL_RenderCopy(renderer, bgTexture, NULL, &bgRect); // Draw the background texture

        // Render the platforms
        renderPlatforms(renderer);

        if (gameOver) {
            // Render the game over screen
            renderGameOver(renderer);
        } else {
            // Render the player texture
            SDL_Rect batarongRect = { batarong.x - cameraX, batarong.y, batarong.width, batarong.height }; // Adjust player position
            SDL_RenderCopy(renderer, batarong.texture, NULL, &batarongRect); // Draw the player texture
        }

        // Present the back buffer
        SDL_RenderPresent(renderer); 

        // Calculate frame time and delay if necessary
        Uint32 frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime); // Delay to maintain frame rate
        }
    }

    // Clean up
    SDL_DestroyTexture(batarong.texture); // Destroy the player texture
    SDL_DestroyTexture(bgTexture); // Destroy the background texture
    SDL_DestroyRenderer(renderer); // Destroy the renderer
    SDL_DestroyWindow(window); // Destroy the window
    SDL_Quit(); // Quit SDL

    return 0;
}
