#include <iostream>
#include <pty.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cstring>
#include <SDL2/SDL.h>
#include <thread>

using std::string;
using std::to_string;

#include "font.h"
#include "color.h"

int row,column = 0;
#define ROW_MAX 24
#define COL_MAX 80
bool hidden = false;

// Utility macros
#define CHECK_ERROR(test, message) \
    do { \
        if((test)) { \
            fprintf(stderr, "%s\n", (message)); \
            exit(1); \
        } \
    } while(0)

int WINDOW_WIDTH 		= renderFontWidth*80;
int WINDOW_HEIGHT 		= renderFontHeight*24;
#define TICKS_FOR_NEXT_FRAME (1000 / 60)

/* -- SDL Pointers --*/
SDL_Event event;
SDL_Renderer *renderer;
SDL_Window *window;
SDL_Texture *texture;
void *texture_pixels;
int texture_pitch;

/* ---- Runtime Variables ---- */
bool frameDone = false;
bool running = true;
int lastFrameTime = 0;

/* ---- INPUT HANDLING ---- */
void PrintKeyInfo( SDL_KeyboardEvent *key ){
	/* Is it a release or a press? */
	if( key->type == SDL_KEYUP )
		printf( "Release:- " );
	else
		printf( "Press:- " );

	/* Print the hardware scancode first */
	printf( "Scancode: 0x%02X", key->keysym.scancode );
	/* Print the name of the key */
	printf( ", Name: %s", SDL_GetKeyName( key->keysym.sym ) );
	printf( "\n" );
}

int processControls() {
		const Uint8 *keys = SDL_GetKeyboardState(NULL);
		
		// Quit
        if (keys[SDL_SCANCODE_ESCAPE]) {
			running = false;
		}
		return 0;
}

void updateInputs() {
	while(running) {		
		processControls();
		
		// Update Origin
		SDL_Delay(16);
	}
}

/* ---- DISPLAYING ---- */
// Load Color as SDL Render Color
int loadColor(const Color& color) {
	SDL_SetRenderDrawColor(
		renderer,
		(int)(color.r*255),
		(int)(color.g*255),
		(int)(color.b*255),
		255
	);	
	return 0;
}

uint8_t *renderPixel(int x, int y, Color &color) {
    x = x % WINDOW_WIDTH;
    y = y % WINDOW_HEIGHT;
    uint8_t *pixel = (uint8_t *)texture_pixels + y * texture_pitch + x * 4;
    pixel[0] = (uint8_t)(color.r * 255);
    pixel[1] = (uint8_t)(color.g * 255);
    pixel[2] = (uint8_t)(color.b * 255);
    pixel[3] = 0xFF;
    return pixel;
}

// Render Character to Screen Position
void renderCharacter(int startX, int startY, Color &frontColor, Color &backColor, char c) {
	for (int y = 0; y <= renderFontHeight; y++) {
		unsigned char renderFontRow = renderFontArray[y + c*renderFontHeight];
		
		for (int x = 0; x <= renderFontWidth; x++) {
			renderFontRow = renderFontRow << 1;
			char resultPixel = renderFontRow & 0x80;
			// Render Pixel if 1
			if (resultPixel) {
				renderPixel(startX+x,startY+y,frontColor);
			} else {
				renderPixel(startX+x,startY+y,backColor);
			}
		}
	}
}

void scrollScreen(int rows) {
	int rowsHeight = rows * renderFontHeight;
	SDL_Rect srcRect = { 0, rowsHeight, WINDOW_WIDTH, WINDOW_HEIGHT - rowsHeight };
	SDL_Rect dstRect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT - rowsHeight };
	SDL_RenderCopy(renderer, texture, &srcRect, &dstRect);
	SDL_Rect clearRect = { 0, WINDOW_HEIGHT - rowsHeight, WINDOW_WIDTH, rowsHeight };
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
	SDL_RenderFillRect(renderer, &clearRect);
	row-=rows;
}

int initVariables() {
	printf("Init Default Vars...\n");
	
	white.r = 1.0f;
	white.g = 1.0f;
	white.b = 1.0f;
	black.r = 0.0f;
	black.g = 0.0f;
	black.b = 0.0f;
	
	red.r = 1.0f;
	red.g = 0.0f;
	red.b = 0.0f;
	green.r = 0.0f;
	green.g = 1.0f;
	green.b = 0.0f;
	blue.r = 0.0f;
	blue.g = 0.0f;
	blue.b = 1.0f;
	
	yellow.r = 1.0f;
	yellow.g = 1.0f;
	yellow.b = 0.0f;
	magenta.r = 1.0f;
	magenta.g = 0.0f;
	magenta.b = 1.0f;
	cyan.r = 0.0f;
	cyan.g = 1.0f;
	cyan.b = 1.0f;
	return 0;
}

/* --- MAIN ---- */
int main(int argc, char **argv) {
	int master_fd, slave_fd;
	pid_t pid;
	uint8_t *pixel;
	
    //SDL_Init(SDL_INIT_VIDEO);
	SDL_Window *window = SDL_CreateWindow(
		"pixterm",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE // | SDL_WINDOW_FULLSCREEN_DESKTOP
	);
    CHECK_ERROR(window == NULL, SDL_GetError());
	
    renderer = SDL_CreateRenderer(
		window,
		-1,
		SDL_RENDERER_ACCELERATED
	); 
	CHECK_ERROR(renderer == NULL, SDL_GetError());
	
	// Clearing of Screen and creating of Texture
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
	texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_RGBA32,
		SDL_TEXTUREACCESS_STREAMING,
		WINDOW_WIDTH,
		WINDOW_HEIGHT
	);
	initVariables();

	SDL_LockTexture(texture, NULL, &texture_pixels, &texture_pitch);
	renderCharacter(0, 0, white, black, 'A');
	SDL_UnlockTexture(texture);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);

    // Create PTY
    if (openpty(&master_fd, &slave_fd, nullptr, nullptr, nullptr) == -1) {
        perror("openpty");
        return 1;
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    } else if (pid == 0) {  // Child process: Attach to PTY and start shell
        close(master_fd);
        setsid();
        dup2(slave_fd, STDIN_FILENO);
        dup2(slave_fd, STDOUT_FILENO);
        dup2(slave_fd, STDERR_FILENO);
        close(slave_fd);
        execl("/bin/bash", "/bin/bash", nullptr);
        perror("execl");
        return 1;
    } else {
        close(slave_fd);
        char buffer[256];

        // Make stdin non-blocking
        fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

        while (true) {
			SDL_LockTexture(texture, NULL, &texture_pixels, &texture_pitch);
            struct pollfd fds[] = {
                { master_fd, POLLIN, 0 }, // Shell output
                { STDIN_FILENO, POLLIN, 0 } // User input
            };

            if (poll(fds, 2, -1) > 0) {
                // Read from shell and print to stdout
                if (fds[0].revents & POLLIN) {
                    ssize_t len = read(master_fd, buffer, sizeof(buffer) - 1);
                    if (len > 0) {
                        buffer[len] = '\0';
                        std::cout << buffer << std::flush;
						string dateTimeString = "Built on " + (string)__TIMESTAMP__;

						for (int i = 0; i < len; i++) {
							if (!hidden) {
								renderCharacter(column*renderFontWidth, row*renderFontHeight, white, black, buffer[i]);
								column++;
							}
							if (buffer[i] == '\n' || column >= COL_MAX) {
								column = 0;
								row++;
							}
							if (buffer[i] == '\e') {
								hidden = true;
							}
							if (buffer[i] == 'm') {
								hidden = false;
							}
							if (row >= ROW_MAX) {
								scrollScreen(1);
							}
						}
                    }
                }

                // Read user input and send to shell
                if (fds[1].revents & POLLIN) {
                    ssize_t len = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
                    if (len > 0) {
                        write(master_fd, buffer, len);
                    }
                }
            }
			SDL_UnlockTexture(texture);
			SDL_RenderCopy(renderer, texture, NULL, NULL);
			SDL_RenderPresent(renderer);
        }
    }
	
	// Camera Origin
	//std::thread inputThread(updateInputs);

	end:	
	SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
	return 0;
}
