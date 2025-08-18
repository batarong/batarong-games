#include <SDL.h>
#include <SDL_ttf.h> 
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>


/* Layout and physics constants */
#define MAX_PLATFORMS 12
#define PLATFORM_WIDTH 100
#define PLATFORM_HEIGHT 20
#define GRAVITY 1
#define JUMP_FORCE -15
#define WINDOW_HEIGHT 600
#define MAX_PIWO 10
#define SPRINT_SPEED 2.0
#define BASE_SPEED 5

/* Sprint mechanics */
#define MAX_SPRINT_ENERGY 100.0f
#define SPRINT_DRAIN_RATE 1.0f
#define SPRINT_REGEN_RATE 0.2f
#define SPRINT_BAR_WIDTH 200
#define SPRINT_BAR_HEIGHT 20

/* UI / object sizes */
#define GAMBLING_MACHINE_WIDTH 64
#define GAMBLING_MACHINE_HEIGHT 64

/* Timings (milliseconds) */
#define SPIN_TIME 2000
#define RESULT_DISPLAY_TIME 2000
#define ERROR_DISPLAY_TIME 2000


/* Fonts and NPC/shop constants */
#define SMALL_FONT_SIZE 18
#define REGULAR_FONT_SIZE 24

#define MAX_RAY 3
#define RAY_WIDTH 64
#define RAY_HEIGHT 64
#define SHOP_ITEM_COUNT 3
#define SHOP_ITEM_NAME_MAX 64

/* Bullets */
#define MAX_BULLETS 10
#define BULLET_SPEED 10
#define BULLET_WIDTH 8
#define BULLET_HEIGHT 4

/* Dialog system limits */
#define DIALOG_MAX_LINES 16
#define DIALOG_LINE_MAX 160

/* Character config limits */
#define MAX_CHARACTER_DEF 16
#define CHARACTER_NAME_MAX 32
#define CHARACTER_IMAGE_MAX 128

typedef struct {
    char name[CHARACTER_NAME_MAX];
    char imagePath[CHARACTER_IMAGE_MAX];
} CharacterDefinition;

static CharacterDefinition characterDefinitions[MAX_CHARACTER_DEF];
static int characterDefinitionCount = 0;

typedef struct {
    void* data;
    size_t size;
} MemoryFile;

static int loadFileToMemory(const char* filePath, MemoryFile* output) {
    if (!output) return -1;
    memset(output, 0, sizeof(*output));
    FILE* file = fopen(filePath, "rb");
    if (!file) {
        fprintf(stderr, "Error opening file: %s\n", filePath);
        return -1;
    }
    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return -1; }
    long fileSize = ftell(file);
    if (fileSize < 0) { fclose(file); return -1; }
    rewind(file);
    void* buffer = malloc((size_t)fileSize);
    if (!buffer) { fclose(file); return -1; }
    size_t bytesRead = fread(buffer, 1, (size_t)fileSize, file);
    fclose(file);
    if (bytesRead != (size_t)fileSize) { free(buffer); return -1; }
    output->data = buffer;
    output->size = (size_t)fileSize;
    return 0;
}

// Keep font buffer alive for lifetime of program (TTF might stream)
static MemoryFile mainFontMem = {0};

// Loading screen state
static void renderLoadingScreen(SDL_Renderer* renderer, TTF_Font* font, const char* status, int step, int total) {
    SDL_SetRenderDrawColor(renderer, 10, 10, 30, 255);
    SDL_RenderClear(renderer);
    int barWidth = (int)((total > 0) ? (float)step / total * 600 : 0);
    SDL_Rect background = {100, 280, 600, 40};
    SDL_SetRenderDrawColor(renderer, 60, 60, 90, 255);
    SDL_RenderFillRect(renderer, &background);
    SDL_Rect foreground = {100, 280, barWidth, 40};
    SDL_SetRenderDrawColor(renderer, 120, 180, 255, 255);
    SDL_RenderFillRect(renderer, &foreground);
    if (font && status) {
        SDL_Color white = {255, 255, 255, 255};
        char line[256];
        snprintf(line, sizeof(line), "%s (%d/%d)", status, step, total);
        SDL_Surface* surface = TTF_RenderText_Solid(font, line, white);
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect textRect = { (800 - surface->w) / 2, 220, surface->w, surface->h };
            SDL_RenderCopy(renderer, texture, NULL, &textRect);
            SDL_DestroyTexture(texture);
            SDL_FreeSurface(surface);
        }
    }
    SDL_RenderPresent(renderer);
}

static char* ltrim(char* s) {
    while (*s && (*s==' '||*s=='\t'||*s=='\r' )) s++;
    return s;
}

static void rtrim(char* s) {
    size_t len = strlen(s);
    while (len>0 && (s[len-1]=='\n'||s[len-1]=='\r'||s[len-1]==' '||s[len-1]=='\t')) { s[--len]='\0'; }
}

static void addCharacterDefinition(const char* name, const char* imagePath) {
    if (!name || !*name || !imagePath || !*imagePath) return;
    for (int i = 0; i < characterDefinitionCount; i++) {
        if (strcmp(characterDefinitions[i].name, name) == 0) {
            snprintf(characterDefinitions[i].imagePath, CHARACTER_IMAGE_MAX, "%s", imagePath);
            return;
        }
    }
    if (characterDefinitionCount < MAX_CHARACTER_DEF) {
        snprintf(characterDefinitions[characterDefinitionCount].name, CHARACTER_NAME_MAX, "%s", name);
        snprintf(characterDefinitions[characterDefinitionCount].imagePath, CHARACTER_IMAGE_MAX, "%s", imagePath);
        characterDefinitionCount++;
    }
}

static const char* getCharacterImagePath(const char* name, const char* fallback) {
    for (int i = 0; i < characterDefinitionCount; i++) {
        if (strcmp(characterDefinitions[i].name, name) == 0) return characterDefinitions[i].imagePath;
    }
    return fallback;
}

/* Convenience wrappers (original code references these names). */
static int readFileToMemory(const char* filePath, MemoryFile* output) { return loadFileToMemory(filePath, output); }
static const char* getCharacterImage(const char* name, const char* fallback) { return getCharacterImagePath(name, fallback); }

static void loadCharacterConfig(const char* filePath) {
    FILE* file = fopen(filePath, "r");
    if (!file) {
        fprintf(stderr, "Error opening config file: %s\n", filePath);
        return;
    }
    char line[256];
    char currentName[CHARACTER_NAME_MAX] = "";
    while (fgets(line, sizeof(line), file)) {
        char* trimmedLine = ltrim(line);
        if (!*trimmedLine) continue;
        if (trimmedLine[0] == '#') {
            int hashCount = 0;
            while (trimmedLine[hashCount] == '#') hashCount++;
            char* sectionName = ltrim(trimmedLine + hashCount);
            rtrim(sectionName);
            if (hashCount == 2) {
                snprintf(currentName, CHARACTER_NAME_MAX, "%s", sectionName);
            }
            continue;
        }
        if (*currentName && strncmp(trimmedLine, "image", 5) == 0) {
            char* equalsSign = strchr(trimmedLine, '=');
            if (equalsSign) {
                char* imagePath = ltrim(equalsSign + 1);
                rtrim(imagePath);
                // Strip optional surrounding quotes (e.g., "images/foo.bmp")
                size_t len = strlen(imagePath);
                if (len >= 2 && imagePath[0] == '"' && imagePath[len - 1] == '"') {
                    imagePath[len - 1] = '\0';
                    imagePath++; // move past opening quote for this call
                }
                addCharacterDefinition(currentName, imagePath);
            }
        }
    }
    fclose(file);
}

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
    bool facingLeft;  // New direction property
    bool sprintKeyReleased;  // New member to track if sprint key was released
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

// Add gambling machine state
typedef struct {
    int x, y;
    SDL_Texture* texture;
} GamblingMachine;

// Add after other struct definitions
typedef struct {
    char text[32];
    int length;
    int maxLength;
} TextInput;

typedef struct {
    int x, y;
    SDL_Texture* texture;
} Ray;

typedef struct {
    char name[SHOP_ITEM_NAME_MAX];
    int price;
    bool purchased;
} ShopItem;

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(SHOP_ITEM_NAME_MAX + 40 < 128, "weird ass error in code, proceed with caution.");
#endif

// Add after other struct definitions
typedef struct {
    int x, y;
    bool active;
    bool direction;  // true = left, false = right
} Bullet;


typedef struct {
    char lines[DIALOG_MAX_LINES][DIALOG_LINE_MAX];
    int totalLines;
    int currentIndex;
    bool active;
    bool freeze_movement;
    bool portrait_visible;
    bool speaker_visible;
    SDL_Texture* portrait_tex;
    char speaker[CHARACTER_NAME_MAX];
} DialogState;

static DialogState dialogState = {0};

void renderText(SDL_Renderer* renderer, TTF_Font* font, const char* text, SDL_Color color, int x, int y);

void dialog_start(const char** lines, int lineCount,
                  const char* speakerName,
                  const char* portraitKey,
                  bool freeze_movement,
                  bool portrait_visible,
                  bool speaker_visible,
                  SDL_Renderer* renderer) {
    if (!lines || lineCount <= 0) return;
    if (lineCount > DIALOG_MAX_LINES) lineCount = DIALOG_MAX_LINES;
    if (dialogState.portrait_tex) { SDL_DestroyTexture(dialogState.portrait_tex); dialogState.portrait_tex = NULL; }
    memset(&dialogState, 0, sizeof(dialogState));
    for (int i = 0; i < lineCount; i++) if (lines[i]) snprintf(dialogState.lines[i], DIALOG_LINE_MAX, "%.*s", DIALOG_LINE_MAX - 1, lines[i]);
    dialogState.totalLines = lineCount;
    dialogState.currentIndex = 0;
    dialogState.active = true;
    dialogState.freeze_movement = freeze_movement;
    dialogState.portrait_visible = portrait_visible;
    dialogState.speaker_visible = speaker_visible && speakerName && *speakerName;
    if (dialogState.speaker_visible) snprintf(dialogState.speaker, CHARACTER_NAME_MAX, "%.*s", CHARACTER_NAME_MAX - 1, speakerName);
    if (portrait_visible && portraitKey && *portraitKey && renderer) {
        const char* path = getCharacterImage(portraitKey, "images/batarong.bmp");
        MemoryFile mem = {0};
        if (readFileToMemory(path, &mem) == 0) {
            SDL_RWops* rw = SDL_RWFromConstMem(mem.data, (int)mem.size);
            SDL_Surface* surf = rw ? SDL_LoadBMP_RW(rw, 1) : NULL;
            if (surf) {
                dialogState.portrait_tex = SDL_CreateTextureFromSurface(renderer, surf);
                SDL_FreeSurface(surf);
            }
            free(mem.data);
        }
    }
}

void dialog_start_simple(const char** lines, int lineCount) { dialog_start(lines, lineCount, NULL, NULL, false, false, false, NULL); }

void dialog_next(void) {
    if (!dialogState.active) return;
    dialogState.currentIndex++;
    if (dialogState.currentIndex >= dialogState.totalLines) {
        dialogState.active = false;
        if (dialogState.portrait_tex) { SDL_DestroyTexture(dialogState.portrait_tex); dialogState.portrait_tex = NULL; }
    }
}

void dialog_close(void) {
    if (dialogState.portrait_tex) { SDL_DestroyTexture(dialogState.portrait_tex); dialogState.portrait_tex = NULL; }
    dialogState.active = false;
}

void dialog_draw(SDL_Renderer* renderer, TTF_Font* font) {
    if (!dialogState.active || !font) return;
    const int boxPadding = 10;
    const int boxHeight = 140;
    SDL_Rect box = {20, 600 - boxHeight - 20, 800 - 40, boxHeight};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_RenderFillRect(renderer, &box);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_Color textColor = {255, 255, 255, 255};
    if (dialogState.portrait_visible && dialogState.portrait_tex) {
        SDL_Rect pRect = { box.x + 20, box.y - 100, 96, 96 };
        SDL_RenderCopy(renderer, dialogState.portrait_tex, NULL, &pRect);
    }
    if (dialogState.speaker_visible) renderText(renderer, font, dialogState.speaker, textColor, box.x + boxPadding, box.y + 6);
    if (dialogState.currentIndex < dialogState.totalLines) {
        int textTop = box.y + boxPadding + (dialogState.speaker_visible ? 26 : 0);
        renderText(renderer, font, dialogState.lines[dialogState.currentIndex], textColor, box.x + boxPadding, textTop);
        char progress[32];
        snprintf(progress, sizeof(progress), "%d/%d", dialogState.currentIndex + 1, dialogState.totalLines);
        renderText(renderer, font, progress, textColor, box.x + box.w - 60, box.y + box.h - 30);
    }
}

// Add to global variables
bool isGambling = false;
GamblingMachine gamblingMachine = {600, 430, NULL}; // Position the machine somewhere accessible
bool aKeyPressed = false; // Track if A key was pressed last frame
bool bKeyPressed = false; // Track if B key was pressed last frame
bool escKeyPressed = false; // Track if ESC key was pressed last frame
TextInput betInput = {"", 0, 10};  // Max 10 digits

bool isSpinning = false;
Uint32 spinStartTime = 0;
int spinResult = 0;
bool resultDisplayed = false;
Uint32 resultStartTime = 0;

int currentBet = 0;  // Store the current bet amount

bool showError = false;
Uint32 errorStartTime = 0;

Ray rayList[MAX_RAY] = {
    {200, 430, NULL},  // First Ray
    {800, 430, NULL},  // Second Ray
    {1200, 430, NULL}  // Third Ray
};

bool isShoppingOpen = false;
Ray* currentRay = NULL;
bool isPaused = false; // Pause state

ShopItem shopItems[SHOP_ITEM_COUNT] = {
    {"A pistol", 5, false},
    {"The America", 50, false},
    {"nuke", 1000, false}
};

Platform platforms[MAX_PLATFORMS] = {
    {100, 500, {100, 500, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, 
    {300, 400, {300, 400, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, 
    {500, 300, {500, 300, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, 
    {200, 200, {200, 200, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, 
    {300, 500, {300, 500, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, 
    {400, 500, {400, 500, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, 
    {500, 500, {500, 500, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, 
    {500, 600, {500, 600, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, 
    {500, 700, {500, 700, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, 
    {600, 500, {600, 500, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, 
    {700, 500, {700, 500, PLATFORM_WIDTH, PLATFORM_HEIGHT}}, 
    {400, 100, {400, 100, PLATFORM_WIDTH, PLATFORM_HEIGHT}}  
};

int platformCount = MAX_PLATFORMS;

// Piwo collectibles
Piwo piwoList[MAX_PIWO] = {
    {150, 450, NULL, false}, 
    {350, 350, NULL, false}, 
    {550, 250, NULL, false}, 
    {250, 150, NULL, false}, 
    {450, 50, NULL, false},  
    {450, 51, NULL, false},  
    {450, 52, NULL, false},  
    {450, 53, NULL, false},  
    {450, 54, NULL, false},  
    {450, 55, NULL, false}   
};

int piwoCount = 0; // Counter for collected piwo

// Add near other global variables
SDL_Texture* gunTexture = NULL;
bool hasGun = false;

// Add to global variables
Bullet bullets[MAX_BULLETS] = {0};
Uint32 lastShotTime = 0;
const int SHOOT_COOLDOWN = 250;  // 250ms cooldown between shots
// Maintain a compact list of active bullet indices to avoid scanning all slots
static int activeBulletIndices[MAX_BULLETS];
static int activeBulletCount = 0;

// Function prototypes
void handleInput(bool* running, Batarong* batarong, bool* gameOver);
void applyGravity(Batarong* batarong);
bool checkCollision(Batarong* batarong, bool* gameOver);
void renderPlatforms(SDL_Renderer* renderer);
void renderGameOver(SDL_Renderer* renderer, TTF_Font* font);
void renderText(SDL_Renderer* renderer, TTF_Font* font, const char* text, SDL_Color color, int x, int y);
void renderPiwo(SDL_Renderer* renderer);
void renderSprintBar(SDL_Renderer* renderer, float sprintEnergy, Batarong* batarong, TTF_Font* font);
void renderGamblingScreen(SDL_Renderer* renderer, TTF_Font* font, TTF_Font* smallFont);
bool isNearGamblingMachine(Batarong* batarong);
void handleTextInput(SDL_Event* event);
void startGambling();
bool hasEnoughPiwoToPlay(void);
bool isNearRay(Batarong* batarong, Ray* ray);
void renderShopScreen(SDL_Renderer* renderer, TTF_Font* font);

// New pause screen renderer
void renderPauseScreen(SDL_Renderer* renderer, TTF_Font* font);

// Add these new function prototypes after existing ones
void shootBullet(Batarong* batarong);
void updateBullets(void);
void renderBullets(SDL_Renderer* renderer);
// Dialog system prototypes (scaffold)
void dialog_start_simple(const char** lines, int lineCount);
void dialog_start(const char** lines, int lineCount, const char* speakerName, const char* portraitKey, bool freeze_movement, bool portrait_visible, bool speaker_visible, SDL_Renderer* renderer);
void dialog_next(void);
void dialog_close(void);
void dialog_draw(SDL_Renderer* renderer, TTF_Font* font);

bool isNearGamblingMachine(Batarong* batarong) {
    int dx = abs((batarong->x + batarong->width/2) - (gamblingMachine.x + GAMBLING_MACHINE_WIDTH/2));
    int dy = abs((batarong->y + batarong->height/2) - (gamblingMachine.y + GAMBLING_MACHINE_HEIGHT/2));
    return dx < 50 && dy < 50; // Within 50 pixels of the machine
}

bool hasEnoughPiwoToPlay(void) {
    return piwoCount >= 10;
}

bool isNearRay(Batarong* batarong, Ray* ray) {
    int dx = abs((batarong->x + batarong->width/2) - (ray->x + RAY_WIDTH/2));
    int dy = abs((batarong->y + batarong->height/2) - (ray->y + RAY_HEIGHT/2));
    return dx < 50 && dy < 50;
}

void renderGamblingScreen(SDL_Renderer* renderer, TTF_Font* font, TTF_Font* smallFont) {
    // Fill screen with a different color for gambling screen
    SDL_SetRenderDrawColor(renderer, 50, 0, 100, 255);
    SDL_RenderClear(renderer);

    // Render title at the top
    SDL_Color textColor = {255, 255, 255};
    renderText(renderer, font, "Gambling Screen (Press B to exit)", textColor, 250, 50);

    // Show current piwo count
    char piwoText[32];
    sprintf(piwoText, "Current Piwo: %d", piwoCount);
    renderText(renderer, font, piwoText, textColor, 250, 100);

    if (isSpinning) {
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - spinStartTime >= SPIN_TIME) {
            isSpinning = false;
            spinResult = (rand() % 4) + 1;  // Random number between 1-4
            resultStartTime = currentTime;
            resultDisplayed = true;
        } else {
            renderText(renderer, font, "Spinning...", textColor, 350, 250);
        }
    } else if (resultDisplayed) {
        char resultText[64];
        
        if (spinResult == 1) {
            int winnings = currentBet * 2;
            piwoCount += winnings;
            sprintf(resultText, "You won! 2x! Bet: %d, Won: %d", currentBet, winnings);
        } else if (spinResult == 2) {
            // Fix: Use floating-point arithmetic for accurate calculation
            int winnings = (int)(currentBet * 1.25f + 0.5f);  // Multiply by 1.25 and round
            piwoCount += winnings;
            sprintf(resultText, "You won! 1.25x! Bet: %d, Won: %d", currentBet, winnings);
        } else {
            sprintf(resultText, "You lost! Bet: %d", currentBet);
        }
        renderText(renderer, font, resultText, textColor, 250, 250);

        // Clear result after display time
        if (SDL_GetTicks() - resultStartTime >= RESULT_DISPLAY_TIME) {
            resultDisplayed = false;
            currentBet = 0;  // Reset the stored bet amount
        }
    } else {
        // Render text input box background near bottom left
        SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255);
        SDL_Rect inputBox = {20, 500, 250, 40}; // Changed width from 200 to 250
        SDL_RenderFillRect(renderer, &inputBox);

        // Check if player has enough piwo to play
        if (!hasEnoughPiwoToPlay()) {
            SDL_Color errorColor = {255, 0, 0};  // Red color for error
            renderText(renderer, font, "Need at least 10 piwo to play!", errorColor, 250, 300);
        } else if (showError) {
            SDL_Color errorColor = {255, 0, 0};  // Red color for error
            renderText(renderer, font, "Not enough piwo!", errorColor, 250, 300);
            
            if (SDL_GetTicks() - errorStartTime >= ERROR_DISPLAY_TIME) {
                showError = false;
            }
        }

        // Render input text or placeholder in new position
        if (betInput.length > 0) {
            renderText(renderer, font, betInput.text, textColor, 30, 505); // Adjusted text position
        } else {
            SDL_Color placeholderColor = {128, 128, 128};
            renderText(renderer, smallFont, "Enter bet amount (min: 10)", placeholderColor, 30, 508);
        }

        // Only show spin instruction if they have enough piwo
        if (hasEnoughPiwoToPlay()) {
            renderText(renderer, font, "Press A to spin!", textColor, 300, 500);
        }
    }
}

void renderShopScreen(SDL_Renderer* renderer, TTF_Font* font) {
    // Fill screen with shop background
    SDL_SetRenderDrawColor(renderer, 0, 100, 100, 255);
    SDL_RenderClear(renderer);

    // Render title
    SDL_Color textColor = {255, 255, 255};
    renderText(renderer, font, "Ray's Shop (Press B to exit)", textColor, 250, 50);

    // Show current piwo count
    char piwoText[32];
    sprintf(piwoText, "Your Piwo: %d", piwoCount);
    renderText(renderer, font, piwoText, textColor, 250, 100);

    // Render shop items
    for (int i = 0; i < SHOP_ITEM_COUNT; i++) {
        SDL_Rect itemRect = {200, 150 + (i * 80), 400, 60};

        // Item background
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderFillRect(renderer, &itemRect);

        // Item text
        char itemText[256];
        const int maxNameLen = 80; // Hard cap to avoid overly long lines
        char safeName[81];
        snprintf(safeName, sizeof(safeName), "%.*s", maxNameLen, shopItems[i].name);
        if (shopItems[i].purchased) {
            snprintf(itemText, sizeof(itemText), "%s (Purchased)", safeName);
        } else {
            snprintf(itemText, sizeof(itemText), "%s - %d piwo (Press %d)", safeName, shopItems[i].price, i + 1);
        }
        renderText(renderer, font, itemText, textColor, 220, 165 + (i * 80));
    }
}

// Modify handleTextInput function to properly handle numeric input
void handleTextInput(SDL_Event* event) {
    if (event->type == SDL_KEYDOWN) {
        // Handle backspace
        if (event->key.keysym.sym == SDLK_BACKSPACE && betInput.length > 0) {
            betInput.text[--betInput.length] = '\0';
        }
        // Handle number keys (both numeric keypad and regular numbers)
        else if ((event->key.keysym.sym >= SDLK_0 && event->key.keysym.sym <= SDLK_9) ||
                 (event->key.keysym.sym >= SDLK_KP_0 && event->key.keysym.sym <= SDLK_KP_9)) {
            if (betInput.length < betInput.maxLength) {
                char numChar;
                if (event->key.keysym.sym >= SDLK_KP_0) {
                    numChar = '0' + (event->key.keysym.sym - SDLK_KP_0);
                } else {
                    numChar = '0' + (event->key.keysym.sym - SDLK_0);
                }
                betInput.text[betInput.length++] = numChar;
                betInput.text[betInput.length] = '\0';
            }
        }
    }
}

void startGambling() {
    if (!hasEnoughPiwoToPlay()) {
        return;  // Don't allow gambling if not enough piwo
    }
    if (betInput.length > 0) {
        currentBet = atoi(betInput.text);  // Store the bet amount
        if (currentBet >= 10) {  // Check minimum bet first
            if (currentBet <= piwoCount) {
                isSpinning = true;
                spinStartTime = SDL_GetTicks();
                piwoCount -= currentBet;  // Deduct the bet amount
                betInput.length = 0;  // Clear input
                betInput.text[0] = '\0';
                resultDisplayed = false;
            } else {
                // Show error for insufficient piwo
                showError = true;
                errorStartTime = SDL_GetTicks();
            }
        }
    }
}

// Only call SDL_GetKeyboardState once per frame
void handleInput(bool* running, Batarong* batarong, bool* gameOver) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            *running = false; // Exit the loop if the window is closed
        }
        if (isGambling) {
            handleTextInput(&event);
        }
    }
    // Get the current state of the keyboard (single fetch)
    const Uint8* state = SDL_GetKeyboardState(NULL);

    // Handle keyboard input for movement
    if (!*gameOver) {
        // ESC handling (single press)
        if (state[SDL_SCANCODE_ESCAPE]) {
            if (!escKeyPressed) {
                if (isGambling) {
                    isGambling = false;
                } else if (isShoppingOpen) {
                    isShoppingOpen = false;
                } else {
                    isPaused = !isPaused; // Toggle pause
                }
                escKeyPressed = true;
            }
        } else {
            escKeyPressed = false;
        }

        // When paused, ignore rest of gameplay input (except ESC already handled)
        if (isPaused) return;

        // Add gambling interaction with key press check
        if (state[SDL_SCANCODE_A]) {
            if (!aKeyPressed) {  // Only trigger once when key is first pressed
                if (!isGambling && !isShoppingOpen) {
                    // Check all Ray NPCs
                    for (int i = 0; i < MAX_RAY; i++) {
                        if (isNearRay(batarong, &rayList[i])) {
                            isShoppingOpen = true;
                            currentRay = &rayList[i];
                            break;
                        }
                    }
                    if (!isShoppingOpen) {  // If not near Ray, check gambling machine
                        if (isNearGamblingMachine(batarong)) {
                            isGambling = true;
                        }
                    }
                } else if (isGambling && !isSpinning && !resultDisplayed) {
                    startGambling();  // Start gambling when A is pressed again
                }
                aKeyPressed = true;
            }
        } else {
            aKeyPressed = false;  // Reset when key is released
        }

        // Add B key for exiting gambling menu
        if (state[SDL_SCANCODE_B]) {
            if (!bKeyPressed) {  // Only trigger once when key is first pressed
                if (isShoppingOpen) {
                    isShoppingOpen = false;
                    currentRay = NULL;
                } else if (isGambling) {
                    isGambling = false;
                }
                bKeyPressed = true;
            }
        } else {
            bKeyPressed = false;  // Reset when key is released
        }

    if (!isGambling) {
            // Check if sprint key was released
            if (!state[SDL_SCANCODE_LSHIFT]) {
                batarong->sprintKeyReleased = true;
                batarong->isSprinting = false;  // Stop sprinting when key is released
                // Only regenerate sprint when shift is not held
                batarong->sprintEnergy = fminf(batarong->sprintEnergy + SPRINT_REGEN_RATE, MAX_SPRINT_ENERGY);
            }

            // Only allow sprinting if we have energy and key was released after depleting energy
            if (batarong->sprintEnergy <= 0) {
                batarong->isSprinting = false;
            } else if (state[SDL_SCANCODE_LSHIFT] && batarong->sprintKeyReleased) {
                batarong->isSprinting = true;
                batarong->sprintKeyReleased = false;
            }
            
            // Handle sprint energy drain
            if (batarong->isSprinting && (state[SDL_SCANCODE_LEFT] || state[SDL_SCANCODE_RIGHT])) {
                batarong->sprintEnergy = fmaxf(batarong->sprintEnergy - SPRINT_DRAIN_RATE, 0);
            }

            float currentSpeed = BASE_SPEED * (batarong->isSprinting ? SPRINT_SPEED : 1.0);

            // Block movement when dialog requests freeze
            if (!dialogState.active || !dialogState.freeze_movement) {
                if (state[SDL_SCANCODE_UP]) {
                    if (batarong->onGround) {
                        batarong->velocityY = JUMP_FORCE; // Jump if on the ground
                        batarong->onGround = false;
                    }
                }
                if (state[SDL_SCANCODE_LEFT]) {
                    batarong->x -= currentSpeed; // Move left
                    batarong->facingLeft = true;  // Update direction
                }
                if (state[SDL_SCANCODE_RIGHT]) {
                    batarong->x += currentSpeed; // Move right
                    batarong->facingLeft = false;  // Update direction
                }
            }
        }

        // Add number key handling for shop purchases
        if (isShoppingOpen) {
            if (state[SDL_SCANCODE_1] || state[SDL_SCANCODE_2] || state[SDL_SCANCODE_3]) {
                int itemIndex = -1;
                if (state[SDL_SCANCODE_1]) itemIndex = 0;
                if (state[SDL_SCANCODE_2]) itemIndex = 1;
                if (state[SDL_SCANCODE_3]) itemIndex = 2;

                if (itemIndex >= 0 && !shopItems[itemIndex].purchased) {
                    if (piwoCount >= shopItems[itemIndex].price) {
                        piwoCount -= shopItems[itemIndex].price;
                        shopItems[itemIndex].purchased = true;
                        // Give player the gun when purchasing first item (pistol)
                        if (itemIndex == 0) {
                            hasGun = true;
                        }
                    }
                }
            }
        }

    } else {
        // Update the restart logic in handleInput function
        if (state[SDL_SCANCODE_R]) {
            *gameOver = false; // Reset game over state
            batarong->x = 300; // Reset player position
            batarong->y = 400; // Reset player position
            batarong->velocityY = 0; // Reset vertical velocity
            batarong->onGround = true; // Reset on ground status
            batarong->sprintEnergy = MAX_SPRINT_ENERGY;  // Reset sprint energy to full
            batarong->isSprinting = false;  // Reset sprint state
            batarong->sprintKeyReleased = true;  // Reset sprint key state
            // Remove piwo reset
            // piwoCount = 0; // Remove this line
            // Remove piwo collectibles reset
            for (int i = 0; i < MAX_PIWO; i++) {
                if (!piwoList[i].collected) {
                    piwoList[i].collected = false; // Only reset uncollected piwo
                }
            }
            // Don't reset gun status
            // hasGun = false; // Remove this line if it exists
        }
    }

    // Shooting control (reuses fetched state)
    if (!*gameOver && !isGambling && !isShoppingOpen && !isPaused) {
        if (state[SDL_SCANCODE_SPACE] && hasGun) {
            shootBullet(batarong);
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
    // Precompute predicted next Y once per frame (saves repeated arithmetic inside loop)
    int nextYPred = batarong->y + batarong->velocityY + GRAVITY;
    for (int i = 0; i < platformCount; i++) {
        if (batarong->x < platforms[i].x + PLATFORM_WIDTH &&
            batarong->x + batarong->width > platforms[i].x &&
            nextYPred + batarong->height >= platforms[i].y &&
            nextYPred <= platforms[i].y + PLATFORM_HEIGHT) {
            batarong->y = platforms[i].y - batarong->height;
            batarong->onGround = true;
            batarong->velocityY = 0;
            break; // Early out after landing
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
    // Fill the entire screen with black
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_Rect fullscreenRect = { 0, 0, 800, 600 };
    SDL_RenderFillRect(renderer, &fullscreenRect);

    // Center the "Game Over" text
    SDL_Color textColor = { 255, 255, 255 }; // White color for text
    renderText(renderer, font, "Game Over", textColor, 300, 250);

    // Center the "Press R to Restart" text
    renderText(renderer, font, "Press R to Restart", textColor, 270, 300);

    // Add piwo count to the game over screen
    char scoreText[32];
    sprintf(scoreText, "piwo count: %d", piwoCount);
    renderText(renderer, font, scoreText, textColor, 300, 350);
}

void renderPauseScreen(SDL_Renderer* renderer, TTF_Font* font) {
    // Dim screen with semi-transparent overlay (simulate by drawing a dark rectangle)
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
    SDL_Rect overlay = {0, 0, 800, 600};
    SDL_RenderFillRect(renderer, &overlay);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    SDL_Color textColor = {255, 255, 255};
    renderText(renderer, font, "Paused", textColor, 360, 240);
    renderText(renderer, font, "Press ESC to Resume", textColor, 300, 280);
}

// Modify renderSprintBar function to show shop prompt when near Ray
void renderSprintBar(SDL_Renderer* renderer, float sprintEnergy, Batarong* batarong, TTF_Font* font) {
    // Draw sprint bar background
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_Rect bgRect = { 10, 560, SPRINT_BAR_WIDTH, SPRINT_BAR_HEIGHT };
    SDL_RenderFillRect(renderer, &bgRect);

    // Draw sprint energy level
    SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
    SDL_Rect energyRect = { 10, 560, (int)(SPRINT_BAR_WIDTH * (sprintEnergy / MAX_SPRINT_ENERGY)), SPRINT_BAR_HEIGHT };
    SDL_RenderFillRect(renderer, &energyRect);

    // Show prompts next to sprint bar
    SDL_Color promptColor = {255, 255, 255};
    if (isNearGamblingMachine(batarong)) {
        renderText(renderer, font, "Press A to gamble", promptColor, SPRINT_BAR_WIDTH + 30, 560);
    } else {
        // Check if near any Ray NPC
        for (int i = 0; i < MAX_RAY; i++) {
            if (isNearRay(batarong, &rayList[i])) {
                renderText(renderer, font, "Press A to enter shop", promptColor, SPRINT_BAR_WIDTH + 30, 560);
                break;
            }
        }
    }
}

// Add these new functions before main()
void shootBullet(Batarong* batarong) {
    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - lastShotTime < SHOOT_COOLDOWN) {
        return;  // Don't shoot if cooldown hasn't elapsed
    }

    // Find first inactive bullet
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].active = true;
            bullets[i].direction = batarong->facingLeft;
            bullets[i].x = batarong->x + (batarong->facingLeft ? 0 : batarong->width);
            bullets[i].y = batarong->y + (batarong->height / 2);
            lastShotTime = currentTime;
            // Register in active list
            if (activeBulletCount < MAX_BULLETS) {
                activeBulletIndices[activeBulletCount++] = i;
            }
            break;
        }
    }
}

void updateBullets(void) {
    for (int i = 0; i < activeBulletCount; ) {
        int idx = activeBulletIndices[i];
        Bullet* b = &bullets[idx];
        if (!b->active) {
            // Remove stale entry (should rarely happen)
            activeBulletIndices[i] = activeBulletIndices[--activeBulletCount];
            continue;
        }
        b->x += b->direction ? -BULLET_SPEED : BULLET_SPEED;
        if (b->x < cameraX - 100 || b->x > cameraX + 900) {
            b->active = false;
            activeBulletIndices[i] = activeBulletIndices[--activeBulletCount];
            continue; // Don't increment i; swapped element needs processing
        }
        i++; // Only advance if bullet remains active
    }
}

void renderBullets(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);  // Yellow bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            SDL_Rect bulletRect = {
                bullets[i].x - cameraX,
                bullets[i].y,
                BULLET_WIDTH,
                BULLET_HEIGHT
            };
            SDL_RenderFillRect(renderer, &bulletRect);
        }
    }
}

int main(void) {
    // Load character config prior to SDL image loads
    loadCharacterConfig("config/config.md");

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

    // Load font file fully into memory then use RWops
    if (readFileToMemory("COMIC.TTF", &mainFontMem) != 0) {
        printf("Failed to read font file into memory.\n");
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_RWops* fontRW = SDL_RWFromConstMem(mainFontMem.data, (int)mainFontMem.size);
    SDL_RWops* smallFontRW = SDL_RWFromConstMem(mainFontMem.data, (int)mainFontMem.size); // reuse buffer
    TTF_Font* font = TTF_OpenFontRW(fontRW, 1, REGULAR_FONT_SIZE);
    if (!font) {
        printf("Failed to open main font from memory: %s\n", TTF_GetError());
        SDL_RWclose(fontRW);
        TTF_Quit(); SDL_Quit(); return 1;
    }
    TTF_Font* smallFont = TTF_OpenFontRW(smallFontRW, 1, SMALL_FONT_SIZE);
    if (!smallFont) {
        printf("Failed to open small font from memory: %s\n", TTF_GetError());
        TTF_CloseFont(font);
        TTF_Quit(); SDL_Quit(); return 1;
    }

    // Create a window and check for errors
    SDL_Window* window = SDL_CreateWindow("2D Game", 
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
        800, 600, SDL_WINDOW_SHOWN);

    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        TTF_CloseFont(font);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Create a renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0); // Create a renderer
    (void)renderer;

    if (renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_CloseFont(font);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Total steps for loading screen (adjust if adding more assets)
    const int totalSteps = 7;
    int step = 0;
    renderLoadingScreen(renderer, font, "Loading background", ++step, totalSteps);
    // Load background into memory first
    MemoryFile bgMem = {0};
    readFileToMemory(getCharacterImage("default", "images/bliss.bmp"), &bgMem);
    SDL_RWops* bgRW = SDL_RWFromConstMem(bgMem.data, (int)bgMem.size);
    SDL_Surface* bgSurface = bgRW ? SDL_LoadBMP_RW(bgRW, 1) : NULL;
    SDL_Texture* bgTexture = bgSurface ? SDL_CreateTextureFromSurface(renderer, bgSurface) : NULL;
    if (bgSurface) SDL_FreeSurface(bgSurface);

    if (bgTexture == NULL) {
        printf("Unable to create background texture! SDL Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_CloseFont(font);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    renderLoadingScreen(renderer, font, "Loading player", ++step, totalSteps);
    MemoryFile playerMem = {0};
    readFileToMemory(getCharacterImage("player", "images/batarong.bmp"), &playerMem);
    SDL_RWops* playerRW = SDL_RWFromConstMem(playerMem.data, (int)playerMem.size);
    SDL_Surface* tempSurface = playerRW ? SDL_LoadBMP_RW(playerRW, 1) : NULL;
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
    Batarong batarong = {300, 400, tempSurface->w, tempSurface->h, 
                         SDL_CreateTextureFromSurface(renderer, tempSurface), 
                         0, true, false, MAX_SPRINT_ENERGY, false, true}; // Add true for sprintKeyReleased
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

    renderLoadingScreen(renderer, font, "Loading piwo", ++step, totalSteps);
    MemoryFile piwoMem = {0};
    readFileToMemory(getCharacterImage("piwo", "images/piwo.bmp"), &piwoMem);
    SDL_RWops* piwoRW = SDL_RWFromConstMem(piwoMem.data, (int)piwoMem.size);
    SDL_Surface* piwoSurface = piwoRW ? SDL_LoadBMP_RW(piwoRW, 1) : NULL; // From config
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

    renderLoadingScreen(renderer, font, "Loading gambling machine", ++step, totalSteps);
    MemoryFile gamblingMem = {0};
    readFileToMemory(getCharacterImage("gambling_machine", "images/gambling.bmp"), &gamblingMem);
    SDL_RWops* gamblingRW = SDL_RWFromConstMem(gamblingMem.data, (int)gamblingMem.size);
    SDL_Surface* gamblingMachineSurface = gamblingRW ? SDL_LoadBMP_RW(gamblingRW, 1) : NULL;
    if (gamblingMachineSurface == NULL) {
        printf("Unable to load gambling machine image! SDL Error: %s\n", SDL_GetError());
        // ... handle error ...
    }
    gamblingMachine.texture = SDL_CreateTextureFromSurface(renderer, gamblingMachineSurface);
    SDL_FreeSurface(gamblingMachineSurface);

    renderLoadingScreen(renderer, font, "Loading ray", ++step, totalSteps);
    MemoryFile rayMem = {0};
    readFileToMemory(getCharacterImage("ray", "images/ray.bmp"), &rayMem);
    SDL_RWops* rayRW = SDL_RWFromConstMem(rayMem.data, (int)rayMem.size);
    SDL_Surface* raySurface = rayRW ? SDL_LoadBMP_RW(rayRW, 1) : NULL;
    if (raySurface == NULL) {
        printf("Unable to load Ray image! SDL Error: %s\n", SDL_GetError());
        // ... handle error ...
    }

    // Create texture for each Ray NPC
    for (int i = 0; i < MAX_RAY; i++) {
        rayList[i].texture = SDL_CreateTextureFromSurface(renderer, raySurface);
    }
    SDL_FreeSurface(raySurface);

    renderLoadingScreen(renderer, font, "Loading gun", ++step, totalSteps);
    MemoryFile gunMem = {0};
    readFileToMemory(getCharacterImage("gun", "images/gun.bmp"), &gunMem);
    SDL_RWops* gunRW = SDL_RWFromConstMem(gunMem.data, (int)gunMem.size);
    SDL_Surface* gunSurface = gunRW ? SDL_LoadBMP_RW(gunRW, 1) : NULL;
    if (gunSurface == NULL) {
        printf("Unable to load gun image! SDL Error: %s\n", SDL_GetError());
    } else {
        gunTexture = SDL_CreateTextureFromSurface(renderer, gunSurface);
        SDL_FreeSurface(gunSurface);
    }

    renderLoadingScreen(renderer, font, "Finishing", ++step, totalSteps);
    // Game loop
    bool running = true;
    bool gameOver = false; // Game over state

    // Frame rate control
    const int targetFPS = 30;
    const int frameDelay = 1000 / targetFPS; // Delay in milliseconds

    // renderer is available in local scope

    while (running) {
        Uint32 frameStart = SDL_GetTicks(); // Get the start time of the frame

        // Handle input
        handleInput(&running, &batarong, &gameOver);

    if (!gameOver && !isPaused) {
            // Apply gravity
            applyGravity(&batarong);

            // Check for collisions with platforms and piwo
            checkCollision(&batarong, &gameOver);

            // Add bullet updates here
            updateBullets();
        }

        // Update camera position to follow the player
        cameraX = batarong.x - (800 / 2); // Center the camera on the player

        // Clear the screen
        SDL_RenderClear(renderer);

        // Draw background
        SDL_Rect bgRect = { 0, 0, 800, 600 };
        SDL_RenderCopy(renderer, bgTexture, NULL, &bgRect);

        // Render the platforms
        renderPlatforms(renderer);

        // Render gambling machine before player
        SDL_Rect machineRect = {
            gamblingMachine.x - cameraX,
            gamblingMachine.y,
            GAMBLING_MACHINE_WIDTH,
            GAMBLING_MACHINE_HEIGHT
        };
        SDL_RenderCopy(renderer, gamblingMachine.texture, NULL, &machineRect);

        // Render the piwo collectibles
        renderPiwo(renderer);

        // Render Ray NPCs
        for (int i = 0; i < MAX_RAY; i++) {
            SDL_Rect rayRect = {
                rayList[i].x - cameraX,
                rayList[i].y,
                RAY_WIDTH,
                RAY_HEIGHT
            };
            SDL_RenderCopy(renderer, rayList[i].texture, NULL, &rayRect);
        }

        if (gameOver) {
            // Render the game over screen
            renderGameOver(renderer, font);
        } else if (isGambling) {
            renderGamblingScreen(renderer, font, smallFont);
        } else if (isShoppingOpen) {
            renderShopScreen(renderer, font);
        } else if (isPaused) {
            // Render gameplay elements behind pause (player, HUD already drawn below)
            SDL_Rect batarongRect = { batarong.x - cameraX, batarong.y, batarong.width, batarong.height };
            SDL_RenderCopyEx(renderer, batarong.texture, NULL, &batarongRect,
                           0, NULL, batarong.facingLeft ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
            if (hasGun) {
                SDL_Rect gunRect = { 
                    batarong.x - cameraX + (batarong.facingLeft ? -32 : batarong.width), 
                    batarong.y + 20, 
                    32, 32 
                };
                SDL_RenderCopyEx(renderer, gunTexture, NULL, &gunRect,
                               0, NULL, batarong.facingLeft ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
            }
            SDL_Color textColor = { 255, 255, 255 };
            char counterText[20];
            sprintf(counterText, "Piwo: %d", piwoCount);
            renderText(renderer, font, counterText, textColor, 650, 10);
            renderSprintBar(renderer, batarong.sprintEnergy, &batarong, font);
            renderPauseScreen(renderer, font);
        } else {
            // Render the player texture (now after gambling machine)
            SDL_Rect batarongRect = { batarong.x - cameraX, batarong.y, batarong.width, batarong.height }; // Adjust player position
            SDL_RenderCopyEx(renderer, batarong.texture, NULL, &batarongRect, 
                           0, NULL, batarong.facingLeft ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);

            // After rendering the player but before the HUD
            if (hasGun) {
                SDL_Rect gunRect = { 
                    batarong.x - cameraX + (batarong.facingLeft ? -32 : batarong.width), 
                    batarong.y + 20, 
                    32, 32 
                };
                SDL_RenderCopyEx(renderer, gunTexture, NULL, &gunRect, 
                               0, NULL, batarong.facingLeft ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
            }

            // Render the piwo counter
            SDL_Color textColor = { 255, 255, 255 }; // White color for text
            char counterText[20];
            sprintf(counterText, "Piwo: %d", piwoCount); // Create the counter text
            renderText(renderer, font, counterText, textColor, 650, 10); // Position the counter at the top right

            // Render sprint bar
            renderSprintBar(renderer, batarong.sprintEnergy, &batarong, font);
        }

        // Add bullet rendering here
        renderBullets(renderer);

    // Always render dialog last so overlay appears above HUD
    dialog_draw(renderer, font);

        // Present the back buffer
        SDL_RenderPresent(renderer); 

        // Cap frame rate
        Uint32 frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }
    }

    // Clean up resources
    SDL_DestroyTexture(batarong.texture); // Destroy the player texture
    SDL_DestroyTexture(bgTexture); // Destroy the background texture
    for (int i = 0; i < MAX_PIWO; i++) {
        SDL_DestroyTexture(piwoList[i].texture); // Destroy each piwo texture
    }
    SDL_DestroyTexture(gamblingMachine.texture);
    for (int i = 0; i < MAX_RAY; i++) {
        SDL_DestroyTexture(rayList[i].texture);
    }
    SDL_DestroyTexture(gunTexture);
    SDL_DestroyRenderer(renderer); // Destroy the renderer
    SDL_DestroyWindow(window); // Destroy the window
    TTF_CloseFont(smallFont);
    TTF_CloseFont(font); // Close the font
    TTF_Quit(); // Quit SDL_ttf
    SDL_Quit(); // Quit SDL

    return 0;
}
