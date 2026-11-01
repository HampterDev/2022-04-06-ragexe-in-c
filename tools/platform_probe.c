#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <string.h>

#ifdef SDL_PLATFORM_MACOS
#include "metal_probe.h"
#endif

int main(int argc, char **argv)
{
    SDL_Window *window;
    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    SDL_Rect input_area = {16, 16, 600, 32};
    Uint64 start;
    int smoke = argc == 2 && !strcmp(argv[1], "--smoke");
    int running = 1, frames = 0, status = 1;
#ifdef SDL_PLATFORM_MACOS
    SDL_MetalView view = NULL;
    struct MetalProbe *metal = NULL;
    flags |= SDL_WINDOW_METAL;
#endif
    if (argc > 1 && !smoke) {
        fputs("usage: platform_probe [--smoke]\n", stderr);
        return 2;
    }
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    window = SDL_CreateWindow("Client platform probe - type Hangul; Escape closes", 800, 600, flags);
    if (!window) goto done;
#ifdef SDL_PLATFORM_MACOS
    view = SDL_Metal_CreateView(window);
    if (!view) goto done;
    metal = metal_probe_create(SDL_Metal_GetLayer(view));
    if (!metal) goto done;
#endif
    if (!SDL_SetTextInputArea(window, &input_area, 0) || !SDL_StartTextInput(window))
        goto done;
    printf("video=%s; composition and committed text are UTF-8\n", SDL_GetCurrentVideoDriver());
    start = SDL_GetTicks();
    while (running) {
        SDL_Event event;
        int width, height;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT ||
                (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE))
                running = 0;
            else if (event.type == SDL_EVENT_TEXT_EDITING)
                printf("editing: %s (start=%d, length=%d)\n", event.edit.text, event.edit.start, event.edit.length);
            else if (event.type == SDL_EVENT_TEXT_INPUT)
                printf("committed: %s\n", event.text.text);
            else if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
                printf("drawable: %d x %d\n", event.window.data1, event.window.data2);
        }
        if (!SDL_GetWindowSizeInPixels(window, &width, &height)) goto done;
        if (width > 0 && height > 0) {
#ifdef SDL_PLATFORM_MACOS
            frames += metal_probe_draw(metal, width, height);
#else
            SDL_Surface *surface = SDL_GetWindowSurface(window);
            if (!surface || !SDL_FillSurfaceRect(surface, NULL, SDL_MapSurfaceRGB(surface, 20, 40, 60)) ||
                !SDL_UpdateWindowSurface(window)) goto done;
            ++frames;
#endif
        }
        if (smoke && SDL_GetTicks() - start >= 300) running = 0;
        SDL_Delay(10);
    }
    status = frames > 0 ? 0 : 1;
    printf("presented %d frames\n", frames);
done:
    if (status) fprintf(stderr, "Platform probe failed: %s\n", SDL_GetError());
    if (window) SDL_StopTextInput(window);
#ifdef SDL_PLATFORM_MACOS
    metal_probe_destroy(metal);
    if (view) SDL_Metal_DestroyView(view);
#endif
    SDL_DestroyWindow(window);
    SDL_Quit();
    return status;
}
