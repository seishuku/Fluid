#ifndef __FLUID_H__
#define __FLUID_H__

typedef struct
{
	int w, h;

	float diff;
	float visc;

	//float *d0, *d1;
	//float *u0, *u1;
	//float *v0, *v1;

	OpenCLContext_t Context;

	cl_program Program;

	cl_kernel AddDensityVelocity;
	cl_kernel SetDensityVelocity;
	cl_kernel Diffuse;
	cl_kernel Advect;
	cl_kernel Project1;
	cl_kernel Project2;
	cl_kernel Project3;

	cl_mem d0, d1;
	cl_mem u0, u1;
	cl_mem v0, v1;

	float *den;
} Fluid2D_t;

bool Fluid2D_Init(Fluid2D_t *Fluid, int width, int height, float diffusion, float viscosity);
void Fluid2D_Destroy(Fluid2D_t *Fluid);
void Fluid2D_Step(Fluid2D_t *Fluid, float dt);
float Fluid2D_GetDensity(Fluid2D_t *Fluid, int x, int y);
void Fluid2D_AddDensityVelocity(Fluid2D_t *Fluid, int x, int y, float u, float v, float d);
void Fluid2D_SetDensityVelocity(Fluid2D_t *Fluid, int x, int y, float u, float v, float d);

#endif
