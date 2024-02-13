#include <stdio.h>

#include "tv_SDL.h"
#include "tv_types.h"
#include "tv_math.h"

void rgen_m4(tv_mat4x4* a){
    for (int i=0;i<16;i++){
        a->x[i] = tv_float(rand()) / tv_float(RAND_MAX) - 0.5;
    }
}

void mmt(tv_mat4x4* out, tv_mat4x4* a, tv_mat4x4* b)
{
    tv_mat4x4 c = *b;
    c = tv_transpose(c);
    for(int i=0;i<4;i++) {
        for(int j=0;j<4;j++){
            tv_float sum = 0.0;
            for(int k=0;k<4;k++){
                sum += a->x[i*4+k] * c.x[j*4+k];
            }
            out->x[i*4+j] = sum;
        }
    }
}

void bench()
{
    srand(169);
    tv_mat4x4 a[1024], b[1024], c;
    for (int i =0; i<1024;i++) {rgen_m4(&a[i]); rgen_m4(&b[i]);}
    const int tests = 100000 * 100;

    {
        tv_float V = 0.0;
        tv_timespec tobj;
        tv_float dt = 0.0;
        for (int i=0;i<tests;i++){
            rgen_m4(&a[0]);
            rgen_m4(&b[0]);
            tv_time_record_start(&tobj);
            c = a[0] & b[0];
            V += c.i11 + c.i44;
            dt += tv_time_record_end(&tobj);
        }
        printf("%.4f ms | V=%.4f\n", dt * 1000.0, V);
    }

}

int main()
{
    bench();

    SDL_SetMainReady();

    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO)<0) exit(1);

    /* Create window */
    SDL_Window* pWindow = SDL_CreateWindow( "SDL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN);
    if(pWindow==NULL) exit(1);

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
