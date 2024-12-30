#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <math.h>

/**********************/
/* SDL Graphics stuff */
/**********************/
#include <SDL2/SDL.h>
#define WINDOW_WIDTH (320)
#define WINDOW_HEIGHT 240

/********************/
/* ALSA Sound Stuff */
/********************/
#include <alsa/asoundlib.h>

#define AUDIO_DEVICE "default"
#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_CHANNELS 2
#define AUDIO_BUFFER_LENGTH (AUDIO_SAMPLE_RATE /50) //20ms

int frame = 0;
int fps;

float x1zoom = 1.0;
float x2zoom = 1.0;
float y1zoom = 1.0;
float y2zoom = 1.0;

Uint32 start = 1;
Uint32 end = 1;

void get_fps()
{
	frame++;
	if (frame % 100 == 0)
	{
		frame = 0;
		end = start;
		start = SDL_GetTicks();
		fps = 100000.0 / (start - end);
	}
}

/***************/
/* Scope Stuff */
/***************/
int colors[] = {0x00ff00, 0xffff00, 0xff00ff, 0x00ffff};

int main(int argc, char **argv) {
    int err, i, j;

    float audio_buffer[(AUDIO_BUFFER_LENGTH * AUDIO_CHANNELS)];
    snd_pcm_t *audio_handle;
    snd_pcm_format_t format = SND_PCM_FORMAT_FLOAT;

    /* Prepare SDL Graphics */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    SDL_Window *window = SDL_CreateWindow("ALSA Oscilloscope", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (window == NULL) {
        fprintf(stderr, "Could not create window.");
        return EXIT_FAILURE;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

    /* Prepare Audio Device */
    if ((err = snd_pcm_open(&audio_handle, AUDIO_DEVICE, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        fprintf(stderr, "Error opening audio device: %s\n", snd_strerror(err));
        return EXIT_FAILURE;
    }

    if ((err = snd_pcm_set_params(audio_handle, format, SND_PCM_ACCESS_RW_INTERLEAVED, AUDIO_CHANNELS, AUDIO_SAMPLE_RATE, 1, 500000)) < 0) {
        fprintf(stderr, "Error setting audio device parameters: %s\n", snd_strerror(err));
        return EXIT_FAILURE;
    }

    bool quit = false;
    uint32_t ret = 0;
    SDL_Event event;

    float spp = (float)WINDOW_WIDTH / (float)AUDIO_BUFFER_LENGTH;
    int halfY = WINDOW_HEIGHT / 2;
    int quarterY = WINDOW_HEIGHT / 4;

    while(!quit) {
        while ((ret = SDL_PollEvent(&event))) {
            switch(event.type) {
                case SDL_QUIT:
                    quit = SDL_TRUE;
                    break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_d) {
                        x1zoom *= 1.1; // Expand view
                        x2zoom *= 1.1; // Expand view
                    } else if (event.key.keysym.sym == SDLK_g) {
                        x1zoom /= 1.1; // Compress view
                        x2zoom /= 1.1; // Compress view
                    } else if (event.key.keysym.sym == SDLK_r) {
                        y1zoom *= 1.1; // Expand y-axis
                        y2zoom *= 1.1; // Expand y-axis
                    } else if (event.key.keysym.sym == SDLK_v) {
                        y1zoom /= 1.1; // Compress y-axis
                        y2zoom /= 1.1; // Compress y-axis
                    }
                    break;
            }
        }

        // Read audio data
        int rc = snd_pcm_readi(audio_handle, audio_buffer, AUDIO_BUFFER_LENGTH);
        if (rc == -EPIPE) {
            /* EPIPE means overrun */
            fprintf(stderr, "overrun occurred\n");
            snd_pcm_prepare(audio_handle);
        }

        // Render scope
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
        SDL_RenderDrawLine(renderer, 0, WINDOW_HEIGHT / 2, WINDOW_WIDTH, WINDOW_HEIGHT / 2);
        SDL_RenderDrawLine(renderer, WINDOW_WIDTH / 2, 0, WINDOW_WIDTH / 2, WINDOW_HEIGHT);

        for(i = 1; i < AUDIO_BUFFER_LENGTH; i++) {
            int x1 = (int)((i - 1) * spp * x1zoom);
            int x2 = (int)(i * spp * x2zoom);
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_RenderDrawLine(renderer, x1, halfY - (quarterY * audio_buffer[(i - 1) * 2] * y1zoom), x2, halfY - (quarterY * audio_buffer[i * 2] * y1zoom));

            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_RenderDrawLine(renderer, x1, halfY - (quarterY * audio_buffer[((i - 1) * 2) + 1] * y2zoom), x2, halfY - (quarterY * audio_buffer[(i * 2) + 1] * y2zoom));
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
