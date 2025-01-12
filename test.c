#include <SDL.h>
#include <SDL_ttf.h> // Include SDL_ttf for text rendering
#include <stdbool.h>
#include <stdio.h>

#define MAX_PLATFORMS 12
#define PLATFORM_WIDTH 100
#define PLATFORM_HEIGHT 20
#define GRAVITY 1 // Gravity constant
#define JUMP_FORCE -15 // Jump force
#define WINDOW_HEIGHT 600 // Window height
#define MAX_PIWO 5 // Maximum number of piwo collectibles
#define SPRINT_SPEED 2.0 // Sprint speed multiplier
#define BASE_SPEED 5 // Base movement speed

#define MAX_SPRINT_ENERGY 100.0f
#define SPRINT_DRAIN_RATE 1.0f
#define SPRINT_REGEN_RATE 0.5f
#define SPRINT_BAR_WIDTH 200
#define SPRINT_BAR_HEIGHT 20

// Global camera offset
int cameraX = 0;

// Define batarong properties
typedef struct {
    int x, y;
    int width, height;
    SDL_Texture* texture; // Texture for the player
    int velocityY; // Vertical velocity for gravity
    bool onGround; // Check if the player is on the ground
    bool isSprinting; // New sprint state
    float sprintEnergy;  // New sprint energy property
} Batarong;

// Define platform properties
typedef struct {
    int x, y;
    SDL_Rect rect; // Rectangle for the platform
} Platform;

// Define piwo properties
typedef struct {
    int x, y;
    SDL_Texture* texture; // Texture for the piwo
    bool collected; // Check if the piwo has been collected
} Piwo;

Platform platforms[MAX_PLATFORMS] = {
    {100, 500, {100, 500, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, // Platform 1
    {300, 400, {300, 400, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, // Platform 2
    {500, 300, {500, 300, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, // Platform 3
    {200, 200, {200, 200, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, // Platform 4
    {300, 500, {300, 500, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, // Platform 5
    {400, 500, {400, 500, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, // Platform 6
    {500, 500, {500, 500, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, // Platform 7
    {500, 600, {500, 600, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, // Platform 8
    {500, 700, {500, 700, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, // Platform 9
    {600, 500, {600, 500, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, // Platform 10
    {700, 500, {700, 500, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, // Platform 11
    {400, 100, {400, 100, PLATFORM_WIDTH, PLATFORM_HEIGHT}}  // Platform 12
};

int platformCount = MAX_PLATFORMS;

// Piwo collectibles
Piwo piwoList[MAX_PIWO] = {
    {150, 450, NULL, false}, // Piwo 1
    {350, 350, NULL, false}, // Piwo 2
    {550, 250, NULL, false}, // Piwo 3
    {250, 150, NULL, false}, // Piwo 4
    {450, 50, NULL, false}    // Piwo 5
};

int piwoCount = 0; // Counter for collected piwo

// Function prototypes
void handleInput(bool* running, Batarong* batarong, bool* gameOver);
void applyGravity(Batarong* batarong);
bool checkCollision(Batarong* batarong, bool* gameOver);
void renderPlatforms(SDL_Renderer* renderer);
void renderGameOver(SDL_Renderer* renderer, TTF_Font* font);
void renderText(SDL_Renderer* renderer, TTF_Font* font, const char* text, SDL_Color color, int x, int y);
void renderPiwo(SDL_Renderer* renderer);
void renderSprintBar(SDL_Renderer* renderer, float sprintEnergy);

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
        // Only allow sprinting if we have energy
        batarong->isSprinting = state[SDL_SCANCODE_LSHIFT] && batarong->sprintEnergy > 0;
        
        // Handle sprint energy
        if (batarong->isSprinting && (state[SDL_SCANCODE_LEFT] || state[SDL_SCANCODE_RIGHT])) {
            batarong->sprintEnergy = fmaxf(batarong->sprintEnergy - SPRINT_DRAIN_RATE, 0);
        } else {
            batarong->sprintEnergy = fminf(batarong->sprintEnergy + SPRINT_REGEN_RATE, MAX_SPRINT_ENERGY);
        }

        float currentSpeed = BASE_SPEED * (batarong->isSprinting ? SPRINT_SPEED : 1.0);

        if (state[SDL_SCANCODE_UP]) {
            if (batarong->onGround) {
                batarong->velocityY = JUMP_FORCE; // Jump if on the ground
                batarong->onGround = false;
            }
        }
        if (state[SDL_SCANCODE_LEFT]) {
            batarong->x -= currentSpeed; // Move left
        }
        if (state[SDL_SCANCODE_RIGHT]) {
            batarong->x += currentSpeed; // Move right
        }
    } else {
        // Check for restart input
        if (state[SDL_SCANCODE_R]) {
            *gameOver = false; // Reset game over state
            batarong->x = 300; // Reset player position
            batarong->y = 400; // Reset player position
            batarong->velocityY = 0; // Reset vertical velocity
            batarong->onGround = true; // Reset on ground status
            piwoCount = 0; // Reset piwo counter
            for (int i = 0; i < MAX_PIWO; i++) {
                piwoList[i].collected = false; // Reset piwo collection status
            }
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

    // Check for collision with piwo
    for (int i = 0; i < MAX_PIWO; i++) {
        if (!piwoList[i].collected && 
            batarong->x < piwoList[i].x + 32 && // Assuming piwo width is 32
            batarong->x + batarong->width > piwoList[i].x &&
            batarong->y < piwoList[i].y + 32 && // Assuming piwo height is 32
            batarong->y + batarong->height > piwoList[i].y) {
            // Collision detected with piwo
            piwoList[i].collected = true; // Mark piwo as collected
            piwoCount++; // Increment the piwo counter
        }
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

void renderPiwo(SDL_Renderer* renderer) {
    for (int i = 0; i < MAX_PIWO; i++) {
        if (!piwoList[i].collected) {
            SDL_Rect piwoRect = { piwoList[i].x - cameraX, piwoList[i].y, 32, 32 }; // Adjust position based on camera
            SDL_RenderCopy(renderer, piwoList[i].texture, NULL, &piwoRect); // Draw the piwo texture
        }
    }
}

void renderText(SDL_Renderer* renderer, TTF_Font* font, const char* text, SDL_Color color, int x, int y) {
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, text, color);
    if (textSurface) {
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_Rect textRect = { x, y, textSurface->w, textSurface->h };
        SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
        SDL_DestroyTexture(textTexture);
        SDL_FreeSurface(textSurface);
    }
}

void renderGameOver(SDL_Renderer* renderer, TTF_Font* font) {
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red color for game over background
    SDL_Rect gameOverRect = { 200, 250, 400, 100 }; // Position and size of the game over rectangle
    SDL_RenderFillRect(renderer, &gameOverRect); // Draw the game over rectangle

    // Render "Game Over" text
    SDL_Color textColor = { 255, 255, 255 }; // White color for text
    renderText(renderer, font, "Game Over", textColor, 250, 270); // Position the text

    // Render "Press R to Restart" text
    renderText(renderer, font, "Press R to Restart", textColor, 220, 320); // Position the restart text
}

void renderSprintBar(SDL_Renderer* renderer, float sprintEnergy) {
    // Draw sprint bar background
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_Rect bgRect = { 10, 560, SPRINT_BAR_WIDTH, SPRINT_BAR_HEIGHT };
    SDL_RenderFillRect(renderer, &bgRect);

    // Draw sprint energy level
    SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
    SDL_Rect energyRect = { 10, 560, (int)(SPRINT_BAR_WIDTH * (sprintEnergy / MAX_SPRINT_ENERGY)), SPRINT_BAR_HEIGHT };
    SDL_RenderFillRect(renderer, &energyRect);
}

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    // Initialize SDL_ttf
    if (TTF_Init() == -1) {
        printf("SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    // Load a font
    TTF_Font* font = TTF_OpenFont("COMIC.TTF", 24); // Replace with your font path
    if (font == NULL) {
        printf("Failed to load font! TTF_Error: %s\n", TTF_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Create a window
    SDL_Window* window = SDL_CreateWindow("2D Game", 
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
        800, 600, SDL_WINDOW_SHOWN); // Create a window

    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        TTF_CloseFont(font);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Create a renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0); // Create a renderer

    if (renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_CloseFont(font);
        TTF_Quit();
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
        TTF_CloseFont(font);
        TTF_Quit();
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
        TTF_CloseFont(font);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Create a texture from the surface
    Batarong batarong = {300, 400, tempSurface->w, tempSurface->h, SDL_CreateTextureFromSurface(renderer, tempSurface), 0, true, false, MAX_SPRINT_ENERGY}; //
    SDL_FreeSurface(tempSurface); // Free the temporary surface

    if (batarong.texture == NULL) {
        printf("Unable to create texture! SDL Error: %s\n", SDL_GetError());
        SDL_DestroyTexture(bgTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_CloseFont(font);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Load the piwo image
    SDL_Surface* piwoSurface = SDL_LoadBMP("images/piwo.bmp"); // Replace with your piwo image path
    if (piwoSurface == NULL) {
        printf("Unable to load piwo image! SDL Error: %s\n", SDL_GetError());
        SDL_DestroyTexture(batarong.texture);
        SDL_DestroyTexture(bgTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_CloseFont(font);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Create a texture from the surface for each piwo
    for (int i = 0; i < MAX_PIWO; i++) {
        piwoList[i].texture = SDL_CreateTextureFromSurface(renderer, piwoSurface);
    }
    SDL_FreeSurface(piwoSurface); // Free the temporary surface

    // Game loop
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

            // Check for collisions with platforms and piwo
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

        // Render the piwo collectibles
        renderPiwo(renderer);

        if (gameOver) {
            // Render the game over screen
            renderGameOver(renderer, font);
        } else {
            // Render the player texture
            SDL_Rect batarongRect = { batarong.x - cameraX, batarong.y, batarong.width, batarong.height }; // Adjust player position
            SDL_RenderCopy(renderer, batarong.texture, NULL, &batarongRect); // Draw the player texture

            // Render the piwo counter
            SDL_Color textColor = { 255, 255, 255 }; // White color for text
            char counterText[20];
            sprintf(counterText, "Piwo: %d", piwoCount); // Create the counter text
            renderText(renderer, font, counterText, textColor, 650, 10); // Position the counter at the top right

            // Render sprint bar
            renderSprintBar(renderer, batarong.sprintEnergy);
        }

        // Present the back buffer
        SDL_RenderPresent(renderer); 

        // Calculate frame time and delay if necessary
        Uint32 frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime); // Delay to maintain frame rate
        }
    }

    // Clean up resources
    SDL_DestroyTexture(batarong.texture); // Destroy the player texture
    SDL_DestroyTexture(bgTexture); // Destroy the background texture
    for (int i = 0; i < MAX_PIWO; i++) {
        SDL_DestroyTexture(piwoList[i].texture); // Destroy each piwo texture
    }
    SDL_DestroyRenderer(renderer); // Destroy the renderer
    SDL_DestroyWindow(window); // Destroy the window
    TTF_CloseFont(font); // Close the font
    TTF_Quit(); // Quit SDL_ttf
    SDL_Quit(); // Quit SDL

    return 0;
}
