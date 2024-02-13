#include <stdio.h>
#include "tv_SDL.h"

#include "tv_math.h"
#include "tv_model.h"
#include "tv_context.h"
#include "tv_surface.h"
#include "tv_pipeline.h"

void bench()
{
    tv_mesh* mesh = tv_mesh_create();
    int ind[3], f;
    double d = 1;
    ind[0] = tv_mesh_add_vertex(mesh, tv_vec3(0,0,-d),tv_vec3(0,0,1),tv_vec2(0,0));
    ind[1] = tv_mesh_add_vertex(mesh, tv_vec3(1,0,-d),tv_vec3(0,0,1),tv_vec2(0,1));
    ind[2] = tv_mesh_add_vertex(mesh, tv_vec3(1,2,-d),tv_vec3(0,0,1),tv_vec2(1,1));
    f = tv_mesh_add_triangle(mesh, ind[0], ind[1], ind[2]);
    tv_context* context = tv_context_create();
    context->eye = tv_vec3(0,0,0);
    context->look = tv_vec3(0,0,-1);
    context->up = tv_vec3(0,1,0);
    context->perspective.near=0.1;
    context->perspective.far=10.0;
    int w=256, h=256;
    tv_surface* surf0 = tv_surface_create(w,h,4);
    tv_surface* surf1 = tv_surface_create(w,h,4);
    tv_surface* surf2 = tv_surface_create(w,h,4);
    context->color_surface = surf0;
    context->depth_surface = surf1;
    context->stencil_surface = surf2;

    tv_draw_mesh(context, mesh, tv_mat4x4::identity());

    tv_surface_destroy(surf0);
    tv_surface_destroy(surf1);
    tv_surface_destroy(surf2);
    tv_mesh_destroy(mesh);
    tv_context_destroy(context);
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
