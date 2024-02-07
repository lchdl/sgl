#include <iostream>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

int main()
{
    SDL_SetMainReady();

    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO)<0) {
        printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
        exit(1);
    }

    /* Create window */
    SDL_Window* pWindow = SDL_CreateWindow( "SDL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 320, 240, SDL_WINDOW_SHOWN);
    if(pWindow==NULL) {
        printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
        exit(1);
    }
    
    /* Get window surface */
    SDL_Surface* pWindowSurface = SDL_GetWindowSurface(pWindow);


    /* Fill the surface gray */
    SDL_FillRect(pWindowSurface, NULL, SDL_MapRGB(pWindowSurface->format, 0x80, 0x80, 0x80));
    SDL_UpdateWindowSurface(pWindow);

    //Hack to get window to stay up
    SDL_Event e; 
    bool quit = false; 
    while(!quit){
        while(SDL_PollEvent(&e)) {
            if(e.type==SDL_QUIT) quit = true; 
        }
    }

    return 0;
}
