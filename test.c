#include <SDL.h>
#include <stdbool.h>
#include <stdio.h>

#define MAX_PLATFORMS 5
#define PLATFORM_WIDTH 100
#define PLATFORM_HEIGHT 20

// Define batarong properties
typedef struct {
    int x, y;
    int width, height;
    SDL_Texture* texture; // Texture for the player
} batarong;

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
    {400, 100, {400, 100, PLATFORM_WIDTH, PLATFORM_HEIGHT}}  // Platform 5
};

int platformCount = MAX_PLATFORMS;

void handleInput(bool* running, batarong* batarong) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            *running = false; // Exit the loop if the window is closed
        }
        // Handle keyboard input
        if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_UP:
                    batarong->y -= 5; // Move up
                    break;
                case SDLK_DOWN:
                    batarong->y += 5; // Move down
                    break;
                case SDLK_LEFT:
                    batarong->x -= 5; // Move left
                    break;
                case SDLK_RIGHT:
                    batarong->x += 5; // Move right
                    break;
            }
        }
    }
}

void renderPlatforms(SDL_Renderer* renderer, int cameraY) {
    for (int i = 0; i < platformCount; i++) {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green color for platforms
        // Adjust platform position based on camera
        SDL_Rect platformRect = { platforms[i].x, platforms[i].y, PLATFORM_WIDTH, PLATFORM_HEIGHT };
        SDL_RenderFillRect(renderer, &platformRect); // Draw the platform
    }
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
    batarong batarong = {400, 300, tempSurface->w, tempSurface->h, SDL_CreateTextureFromSurface(renderer, tempSurface)};
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

    // Frame rate control
    const int targetFPS = 30;
    const int frameDelay = 1000 / targetFPS; // Delay in milliseconds

    // Camera offset
    int cameraY = 0;

    while (running) {
        Uint32 frameStart = SDL_GetTicks(); // Get the start time of the frame

        // Handle input
        handleInput(&running, &batarong);

        // Update camera position based on player movement
        // Only move the camera when the player reaches certain thresholds
        if (batarong.y < 200) {
            cameraY += 5; // Move camera down if player moves up
            batarong.y = 200; // Prevent the player from moving above the top of the screen
        } else if (batarong.y > 500) {
            cameraY -= 5; // Move camera up if player moves down
            batarong.y = 500; // Prevent the player from moving below the bottom of the screen
        }

        // Clear the screen
        SDL_RenderClear(renderer);

        // Render the background texture (scaled to fit the window)
        SDL_Rect bgRect = { 0, 0, 800, 600 }; // Set the background rectangle to the window size
        SDL_RenderCopy(renderer, bgTexture, NULL, &bgRect); // Draw the background texture

        // Render the platforms
        renderPlatforms(renderer, cameraY);

        // Render the player texture
        SDL_Rect batarongRect = { batarong.x, batarong.y, batarong.width, batarong.height };
        SDL_RenderCopy(renderer, batarong.texture, NULL, &batarongRect); // Draw the player texture

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
