# Batarong Game - AI Coding Assistant Guide

## Project Overview
This is a 2D platformer game written in C using SDL2. The player controls Batarong through multiple worlds to save Bringus, featuring platforming, combat, gambling mechanics, and an NPC shop system.

## Architecture & Core Components

### Single File Structure
- `game.c`: Monolithic game implementation (~1200 lines)
- All game logic, rendering, input handling in one file
- Uses struct-based entities with global state management

### Key Systems
- **Config System**: Custom markdown parser loads assets from `config/config.md`
- **Camera System**: Side-scrolling with `cameraX` offset following player
- **Entity Management**: Static arrays for platforms, piwo (collectibles), NPCs, bullets
- **Game States**: Gambling machine interactions, pause menu, shop interface

## Critical Development Patterns

### Asset Loading Pattern
```c
// Always use getCharacterImage() with fallback
SDL_Surface* surface = SDL_LoadBMP(getCharacterImage("entity_name", "images/fallback.bmp"));
```
Assets defined in `config/config.md` using markdown headers:
- `# category` (characters, backgrounds, objects, etc.)
- `## entity_name` 
- `image="path/to/file.bmp"`

### Entity Definition Pattern
All entities follow this struct pattern:
```c
typedef struct {
    int x, y;           // Position
    SDL_Texture* texture;
    bool state_flags;   // collected, active, etc.
} EntityType;
```

### Coordinate System
- Screen coordinates: player position adjusted by `cameraX` for rendering
- World coordinates: absolute positions for collision detection
- Camera follows player with boundaries

## Build & Development Workflow

### Essential Commands
```bash
make              # Build to output-directory/main-game
make run          # Build and run game
make debug        # Build with debug symbols (-g -O0)
make clean        # Remove output directory
```

### Dependencies
- SDL2 + SDL2_ttf (auto-detected via pkg-config)
- Requires `COMIC.TTF` font file in root directory
- BMP image assets in `images/` directory

## Game-Specific Constants
Key defines for gameplay tuning:
- `GRAVITY 1`, `JUMP_FORCE -15`: Physics constants
- `MAX_PLATFORMS 12`, `MAX_PIWO 10`, `MAX_RAY 3`: Entity limits  
- `SPRINT_ENERGY` system: 100 max, 1.0 drain rate, 0.2 regen rate
- Timing: `SPIN_TIME 2000ms`, `RESULT_DISPLAY_TIME 2000ms`

## Integration Points

### Config Loading
- Must call `loadCharacterConfig("config/config.md")` before SDL initialization
- Parser handles markdown format with custom key=value sections
- Fallback images used when config entries missing

### Collision Detection
- Platform collision: Rectangle-based with `onGround` state management
- Entity interaction: Distance-based proximity checks
- Bullet collision: Simple bounds checking against entities

### Rendering Order
1. Background texture (bliss.bmp)
2. Platforms (procedurally drawn rectangles)  
3. Static entities (gambling machine, NPCs)
4. Collectibles (piwo)
5. Player (with horizontal flip based on `facingLeft`)
6. Held items (gun rendering offset from player)
7. Projectiles (bullets)
8. UI elements (piwo counter, sprint bar)

## Common Gotchas
- All BMP files must be in `images/` directory and copied by Makefile
- Camera offset must be applied to all world-to-screen coordinate conversions
- Entity arrays are fixed-size with manual index management
- Config parser expects exact markdown header format (`#` and `##` only)
- SDL cleanup order matters: textures before renderer before window
