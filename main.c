#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

typedef struct Button {
	SDL_Rect rect;
	const char *label;
	bool isHovered;
} Button;

static SDL_Texture *renderText(SDL_Renderer *renderer, TTF_Font *font, const char *message, SDL_Color color, int *width, int *height) {
	SDL_Surface *surface = TTF_RenderUTF8_Blended(font, message, color);
	if (!surface) {
		fprintf(stderr, "Error rendering text: %s\n", TTF_GetError());
		return NULL;
	}
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (!texture) {
		fprintf(stderr, "Error creating texture: %s\n", SDL_GetError());
	}
	if (width) *width = surface->w;
	if (height) *height = surface->h;
	SDL_FreeSurface(surface);
	return texture;
}

int main(int argc, char *argv[]) {
	(void)argc; (void)argv;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
		fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
		return 1;
	}
	if (TTF_Init() != 0) {
		fprintf(stderr, "SDL_ttf initialization failed: %s\n", TTF_GetError());
		SDL_Quit();
		return 1;
	}

	SDL_Window *window = SDL_CreateWindow(
		"Batarong Launcher",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		WINDOW_WIDTH, WINDOW_HEIGHT,
		SDL_WINDOW_SHOWN
	);
	if (!window) {
		fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
		TTF_Quit(); SDL_Quit();
		return 1;
	}
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!renderer) {
		fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
		SDL_DestroyWindow(window); TTF_Quit(); SDL_Quit();
		return 1;
	}

	const char *fontPaths[] = { "COMIC.TTF", "./output-directory/COMIC.TTF", NULL };
	TTF_Font *font = NULL;
	for (int i = 0; fontPaths[i]; ++i) {
		font = TTF_OpenFont(fontPaths[i], 24);
		if (font) break;
	}
	if (!font) {
		fprintf(stderr, "Font loading failed: %s\n", TTF_GetError());
	}

	int titleWidth = 0, titleHeight = 0;
	SDL_Texture *titleTexture = NULL;
	if (font) {
		SDL_Color white = {255, 255, 255, 255};
		titleTexture = renderText(renderer, font, "Batarong Game", white, &titleWidth, &titleHeight);
	}

	Button playButton;
	playButton.rect.w = 180;
	playButton.rect.h = 40;
	playButton.rect.x = (WINDOW_WIDTH - playButton.rect.w) / 2;
	playButton.rect.y = 40 + titleHeight + 20;
	playButton.label = "Play Game";
	playButton.isHovered = false;

	SDL_Texture *labelNormalTexture = NULL, *labelHoverTexture = NULL;
	int labelWidth = 0, labelHeight = 0;
	if (font) {
		SDL_Color white = {255,255,255,255};
		SDL_Color red = {220,30,30,255};
		labelNormalTexture = renderText(renderer, font, playButton.label, white, &labelWidth, &labelHeight);
		labelHoverTexture = renderText(renderer, font, playButton.label, red, NULL, NULL);
		if (labelWidth + 40 < playButton.rect.w) {
			// Keep existing width
		} else {
			playButton.rect.w = labelWidth + 40;
			playButton.rect.x = (WINDOW_WIDTH - playButton.rect.w) / 2;
		}
	}

	bool isRunning = true;
	while (isRunning) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				isRunning = false;
			} else if (event.type == SDL_MOUSEMOTION) {
				int mouseX = event.motion.x, mouseY = event.motion.y;
				playButton.isHovered = SDL_PointInRect(&(SDL_Point){mouseX, mouseY}, &playButton.rect);
			} else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
				int mouseX = event.button.x, mouseY = event.button.y;
				if (SDL_PointInRect(&(SDL_Point){mouseX, mouseY}, &playButton.rect)) {
					fprintf(stdout, "Play button clicked\n");
					fflush(stdout);
				}
			}
		}

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		if (titleTexture) {
			SDL_Rect dst = { (WINDOW_WIDTH - titleWidth)/2, 40, titleWidth, titleHeight };
			SDL_RenderCopy(renderer, titleTexture, NULL, &dst);
		}

		if (font && labelNormalTexture && labelHoverTexture) {
			SDL_Texture *activeTexture = playButton.isHovered ? labelHoverTexture : labelNormalTexture;
			SDL_Rect dst = { playButton.rect.x + (playButton.rect.w - labelWidth)/2, playButton.rect.y + (playButton.rect.h - labelHeight)/2, labelWidth, labelHeight };
			SDL_RenderCopy(renderer, activeTexture, NULL, &dst);
		}

		SDL_RenderPresent(renderer);
		SDL_Delay(10);
	}

	if (titleTexture) SDL_DestroyTexture(titleTexture);
	if (labelNormalTexture) SDL_DestroyTexture(labelNormalTexture);
	if (labelHoverTexture) SDL_DestroyTexture(labelHoverTexture);
	if (font) TTF_CloseFont(font);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	TTF_Quit();
	SDL_Quit();
	return 0;
}
