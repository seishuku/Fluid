#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "opencl.h"
#include "fluid2d.h"

#ifdef WIN32
#include <Windows.h>
#define DBGPRINTF(...) { char buf[512]; snprintf(buf, sizeof(buf), __VA_ARGS__); OutputDebugString(buf); }
#else
#define DBGPRINTF(...) { fprintf(stderr, __VA_ARGS__); }
#endif

#define IX(x, y, sx, sy) (min(sx-1, max(1, (x)))+min(sy-1, max(1, (y)))*sx)

bool Fluid2D_Init(Fluid2D_t *Fluid, int width, int height, float diffusion, float viscosity)
{
	Fluid->w=width;
	Fluid->h=height;
	Fluid->diff=diffusion;
	Fluid->visc=viscosity;

	if(!OpenCL_Init(&Fluid->Context))
		return false;
	else
	{
		cl_int Error=0;

		Fluid->Program=OpenCL_LoadProgam(&Fluid->Context, "fluid2d.cl");

		Fluid->SetDensityVelocity=clCreateKernel(Fluid->Program, "set_density_velocity", &Error);
		DBGPRINTF("%s\n", clGetErrorString(Error));
		Fluid->AddDensityVelocity=clCreateKernel(Fluid->Program, "add_density_velocity", &Error);
		DBGPRINTF("%s\n", clGetErrorString(Error));
		Fluid->Diffuse=clCreateKernel(Fluid->Program, "diffuse", &Error);
		DBGPRINTF("%s\n", clGetErrorString(Error));
		Fluid->Advect=clCreateKernel(Fluid->Program, "advect", &Error);
		DBGPRINTF("%s\n", clGetErrorString(Error));
		Fluid->Project1=clCreateKernel(Fluid->Program, "project1", &Error);
		DBGPRINTF("%s\n", clGetErrorString(Error));
		Fluid->Project2=clCreateKernel(Fluid->Program, "project2", &Error);
		DBGPRINTF("%s\n", clGetErrorString(Error));
		Fluid->Project3=clCreateKernel(Fluid->Program, "project3", &Error);
		DBGPRINTF("%s\n", clGetErrorString(Error));

		Fluid->d0=clCreateBuffer(Fluid->Context.Context, CL_MEM_READ_WRITE, sizeof(float)*Fluid->w*Fluid->h, NULL, &Error);
		Fluid->d1=clCreateBuffer(Fluid->Context.Context, CL_MEM_READ_WRITE, sizeof(float)*Fluid->w*Fluid->h, NULL, &Error);
		Fluid->u0=clCreateBuffer(Fluid->Context.Context, CL_MEM_READ_WRITE, sizeof(float)*Fluid->w*Fluid->h, NULL, &Error);
		Fluid->u1=clCreateBuffer(Fluid->Context.Context, CL_MEM_READ_WRITE, sizeof(float)*Fluid->w*Fluid->h, NULL, &Error);
		Fluid->v0=clCreateBuffer(Fluid->Context.Context, CL_MEM_READ_WRITE, sizeof(float)*Fluid->w*Fluid->h, NULL, &Error);
		Fluid->v1=clCreateBuffer(Fluid->Context.Context, CL_MEM_READ_WRITE, sizeof(float)*Fluid->w*Fluid->h, NULL, &Error);
	}

	Fluid->den=calloc(width*height, sizeof(float));

	if(Fluid->den==NULL)
		return false;

	//Fluid->d0=calloc(width*height, sizeof(float));

	//if(Fluid->d0==NULL)
	//	return false;

	//Fluid->d1=calloc(width*height, sizeof(float));

	//if(Fluid->d1==NULL)
	//	return false;

	//Fluid->u0=calloc(width*height, sizeof(float));

	//if(Fluid->u0==NULL)
	//	return false;

	//Fluid->u1=calloc(width*height, sizeof(float));

	//if(Fluid->u1==NULL)
	//	return false;

	//Fluid->v0=calloc(width*height, sizeof(float));

	//if(Fluid->v0==NULL)
	//	return false;

	//Fluid->v1=calloc(width*height, sizeof(float));

	//if(Fluid->v1==NULL)
	//	return false;

	return true;
}

void Fluid2D_Destroy(Fluid2D_t *Fluid)
{
	free(Fluid->den);

	//free(Fluid->d0);
	//free(Fluid->d1);
	//free(Fluid->v0);
	//free(Fluid->v1);
	//free(Fluid->u0);
	//free(Fluid->u1);
}

//static void lin_solve(Fluid2D_t *Fluid, float *x, float *x0, float a, float c, int iter)
//{
//	float cRecip=1.0f/c;
//
//	for(int j=1;j<Fluid->h-1;j++)
//	{
//		for(int i=1;i<Fluid->w-1;i++)
//		{
//			for(int l=0;l<iter;l++)
//			{
//				x[IX(i, j, Fluid->w, Fluid->h)]=
//				(
//					x0[IX(i, j, Fluid->w, Fluid->h)]+
//					a*
//					(
//						x[IX(i+1, j, Fluid->w, Fluid->h)]+
//						x[IX(i-1, j, Fluid->w, Fluid->h)]+
//						x[IX(i, j+1, Fluid->w, Fluid->h)]+
//						x[IX(i, j-1, Fluid->w, Fluid->h)]
//					)
//				)*cRecip;
//			}
//		}
//	}
//}

//static void diffuse(Fluid2D_t *Fluid, float *x, float *x0, float diff, float dt, int iter)
//{
//	float a=dt*diff*(Fluid->w-2)*(Fluid->h-2);
//
//	lin_solve(Fluid, x, x0, a, 1.0f+6.0f*a, iter);
//}

//static void advect(Fluid2D_t *Fluid, float *d, float *d0, float *velocX, float *velocY, float dt)
//{
//	float dtx=dt*(Fluid->w-2);
//	float dty=dt*(Fluid->h-2);
//
//	for(int j=1;j<Fluid->h-1;j++)
//	{
//		for(int i=1;i<Fluid->w-1;i++)
//		{
//			float x=(float)i-dtx*velocX[IX(i, j, Fluid->w, Fluid->h)];
//			float y=(float)j-dty*velocY[IX(i, j, Fluid->w, Fluid->h)];
//
//			if(x<0.5f)
//				x=0.5f;
//
//			if(x>(float)Fluid->w+0.5f)
//				x=(float)Fluid->w+0.5f;
//
//			int i0=(int)floorf(x);
//			int i1=i0+1;
//
//			if(y<0.5f)
//				y=0.5f;
//
//			if(y>(float)Fluid->h+0.5f)
//				y=(float)Fluid->h+0.5f;
//
//			int j0=(int)floorf(y);
//			int j1=j0+1;
//
//			float s1=x-i0;
//			float s0=1.0f-s1;
//			float t1=y-j0;
//			float t0=1.0f-t1;
//
//			d[IX(i, j, Fluid->w, Fluid->h)]=
//			s0*
//			(
//				t0*d0[IX(i0, j0, Fluid->w, Fluid->h)]+
//				t1*d0[IX(i0, j1, Fluid->w, Fluid->h)]
//			)+
//			s1*
//			(
//				t0*d0[IX(i1, j0, Fluid->w, Fluid->h)]+
//				t1*d0[IX(i1, j1, Fluid->w, Fluid->h)]
//			);
//		}
//	}
//}

//static void project(Fluid2D_t *Fluid, float *velocX, float *velocY, float *p, float *div, int iter)
//{
//	for(int j=1;j<Fluid->h-1;j++)
//	{
//		for(int i=1;i<Fluid->w-1;i++)
//		{
//			div[IX(i, j, Fluid->w, Fluid->h)]=
//			-0.5f*
//			(
//				velocX[IX(i+1, j, Fluid->w, Fluid->h)]-
//				velocX[IX(i-1, j, Fluid->w, Fluid->h)]+
//				velocY[IX(i, j+1, Fluid->w, Fluid->h)]-
//				velocY[IX(i, j-1, Fluid->w, Fluid->h)]
//			)/Fluid->w;
//
//			p[IX(i, j, Fluid->w, Fluid->h)]=0.0f;
//		}
//	}
//
//	lin_solve(Fluid, p, div, 1, 6, iter);
//
//	for(int j=1;j<Fluid->h-1;j++)
//	{
//		for(int i=1;i<Fluid->w-1;i++)
//		{
//			velocX[IX(i, j, Fluid->w, Fluid->h)]-=0.5f*(p[IX(i+1, j, Fluid->w, Fluid->h)]-p[IX(i-1, j, Fluid->w, Fluid->h)])*Fluid->w;
//			velocY[IX(i, j, Fluid->w, Fluid->h)]-=0.5f*(p[IX(i, j+1, Fluid->w, Fluid->h)]-p[IX(i, j-1, Fluid->w, Fluid->h)])*Fluid->h;
//		}
//	}
//}

void Fluid2D_Step(Fluid2D_t *Fluid, float dt)
{
	int intr=4;
	size_t size[2]={ Fluid->w, Fluid->h };

	clSetKernelArg(Fluid->Diffuse, 2, sizeof(cl_float), &Fluid->visc);
	clSetKernelArg(Fluid->Diffuse, 3, sizeof(cl_float), &dt);
	clSetKernelArg(Fluid->Diffuse, 4, sizeof(cl_int), &intr);

	clSetKernelArg(Fluid->Diffuse, 0, sizeof(cl_mem), &Fluid->u1);
	clSetKernelArg(Fluid->Diffuse, 1, sizeof(cl_mem), &Fluid->u0);
	clEnqueueNDRangeKernel(Fluid->Context.CommandQueue, Fluid->Diffuse, 2, NULL, size, NULL, 0, NULL, NULL);
	//diffuse(Fluid, Fluid->u1, Fluid->u0, Fluid->visc, dt, 4);

	clSetKernelArg(Fluid->Diffuse, 0, sizeof(cl_mem), &Fluid->v1);
	clSetKernelArg(Fluid->Diffuse, 1, sizeof(cl_mem), &Fluid->v0);
	clEnqueueNDRangeKernel(Fluid->Context.CommandQueue, Fluid->Diffuse, 2, NULL, size, NULL, 0, NULL, NULL);
	//diffuse(Fluid, Fluid->v1, Fluid->v0, Fluid->visc, dt, 4);

	clSetKernelArg(Fluid->Project1, 0, sizeof(cl_mem), &Fluid->u1);
	clSetKernelArg(Fluid->Project1, 1, sizeof(cl_mem), &Fluid->v1);
	clSetKernelArg(Fluid->Project1, 2, sizeof(cl_mem), &Fluid->u0);
	clSetKernelArg(Fluid->Project1, 3, sizeof(cl_mem), &Fluid->v0);
	clSetKernelArg(Fluid->Project1, 4, sizeof(cl_int), &intr);
	clEnqueueNDRangeKernel(Fluid->Context.CommandQueue, Fluid->Project1, 2, NULL, size, NULL, 0, NULL, NULL);
	clSetKernelArg(Fluid->Project2, 0, sizeof(cl_mem), &Fluid->u1);
	clSetKernelArg(Fluid->Project2, 1, sizeof(cl_mem), &Fluid->v1);
	clSetKernelArg(Fluid->Project2, 2, sizeof(cl_mem), &Fluid->u0);
	clSetKernelArg(Fluid->Project2, 3, sizeof(cl_mem), &Fluid->v0);
	clSetKernelArg(Fluid->Project2, 4, sizeof(cl_int), &intr);
	clEnqueueNDRangeKernel(Fluid->Context.CommandQueue, Fluid->Project2, 2, NULL, size, NULL, 0, NULL, NULL);
	clSetKernelArg(Fluid->Project3, 0, sizeof(cl_mem), &Fluid->u1);
	clSetKernelArg(Fluid->Project3, 1, sizeof(cl_mem), &Fluid->v1);
	clSetKernelArg(Fluid->Project3, 2, sizeof(cl_mem), &Fluid->u0);
	clSetKernelArg(Fluid->Project3, 3, sizeof(cl_mem), &Fluid->v0);
	clSetKernelArg(Fluid->Project3, 4, sizeof(cl_int), &intr);
	clEnqueueNDRangeKernel(Fluid->Context.CommandQueue, Fluid->Project3, 2, NULL, size, NULL, 0, NULL, NULL);
	//project(Fluid, Fluid->u1, Fluid->v1, Fluid->u0, Fluid->v0, 4);

	clSetKernelArg(Fluid->Advect, 0, sizeof(cl_mem), &Fluid->u0);
	clSetKernelArg(Fluid->Advect, 1, sizeof(cl_mem), &Fluid->u1);
	clSetKernelArg(Fluid->Advect, 2, sizeof(cl_mem), &Fluid->u1);
	clSetKernelArg(Fluid->Advect, 3, sizeof(cl_mem), &Fluid->v1);
	clSetKernelArg(Fluid->Advect, 4, sizeof(cl_float), &dt);
	clEnqueueNDRangeKernel(Fluid->Context.CommandQueue, Fluid->Advect, 2, NULL, size, NULL, 0, NULL, NULL);
	//advect(Fluid, Fluid->u0, Fluid->u1, Fluid->u1, Fluid->v1, dt);

	clSetKernelArg(Fluid->Advect, 0, sizeof(cl_mem), &Fluid->v0);
	clSetKernelArg(Fluid->Advect, 1, sizeof(cl_mem), &Fluid->v1);
	clEnqueueNDRangeKernel(Fluid->Context.CommandQueue, Fluid->Advect, 2, NULL, size, NULL, 0, NULL, NULL);
	//advect(Fluid, Fluid->v0, Fluid->v1, Fluid->u1, Fluid->v1, dt);

	clSetKernelArg(Fluid->Project1, 0, sizeof(cl_mem), &Fluid->u0);
	clSetKernelArg(Fluid->Project1, 1, sizeof(cl_mem), &Fluid->v0);
	clSetKernelArg(Fluid->Project1, 2, sizeof(cl_mem), &Fluid->u1);
	clSetKernelArg(Fluid->Project1, 3, sizeof(cl_mem), &Fluid->v1);
	clSetKernelArg(Fluid->Project1, 4, sizeof(cl_int), &intr);
	clEnqueueNDRangeKernel(Fluid->Context.CommandQueue, Fluid->Project1, 2, NULL, size, NULL, 0, NULL, NULL);
	clSetKernelArg(Fluid->Project2, 0, sizeof(cl_mem), &Fluid->u0);
	clSetKernelArg(Fluid->Project2, 1, sizeof(cl_mem), &Fluid->v0);
	clSetKernelArg(Fluid->Project2, 2, sizeof(cl_mem), &Fluid->u1);
	clSetKernelArg(Fluid->Project2, 3, sizeof(cl_mem), &Fluid->v1);
	clSetKernelArg(Fluid->Project2, 4, sizeof(cl_int), &intr);
	clEnqueueNDRangeKernel(Fluid->Context.CommandQueue, Fluid->Project2, 2, NULL, size, NULL, 0, NULL, NULL);
	clSetKernelArg(Fluid->Project3, 0, sizeof(cl_mem), &Fluid->u0);
	clSetKernelArg(Fluid->Project3, 1, sizeof(cl_mem), &Fluid->v0);
	clSetKernelArg(Fluid->Project3, 2, sizeof(cl_mem), &Fluid->u1);
	clSetKernelArg(Fluid->Project3, 3, sizeof(cl_mem), &Fluid->v1);
	clSetKernelArg(Fluid->Project3, 4, sizeof(cl_int), &intr);
	clEnqueueNDRangeKernel(Fluid->Context.CommandQueue, Fluid->Project3, 2, NULL, size, NULL, 0, NULL, NULL);
	//project(Fluid, Fluid->u0, Fluid->v0, Fluid->u1, Fluid->v1, 4);

	clSetKernelArg(Fluid->Diffuse, 0, sizeof(cl_mem), &Fluid->d1);
	clSetKernelArg(Fluid->Diffuse, 1, sizeof(cl_mem), &Fluid->d0);
	clSetKernelArg(Fluid->Diffuse, 2, sizeof(cl_float), &Fluid->diff);
	clEnqueueNDRangeKernel(Fluid->Context.CommandQueue, Fluid->Diffuse, 2, NULL, size, NULL, 0, NULL, NULL);
	//diffuse(Fluid, Fluid->d1, Fluid->d0, Fluid->diff, dt, 4);

	clSetKernelArg(Fluid->Advect, 0, sizeof(cl_mem), &Fluid->d0);
	clSetKernelArg(Fluid->Advect, 1, sizeof(cl_mem), &Fluid->d1);
	clSetKernelArg(Fluid->Advect, 2, sizeof(cl_mem), &Fluid->u0);
	clSetKernelArg(Fluid->Advect, 3, sizeof(cl_mem), &Fluid->v0);
	clSetKernelArg(Fluid->Advect, 4, sizeof(cl_float), &dt);
	clEnqueueNDRangeKernel(Fluid->Context.CommandQueue, Fluid->Advect, 2, NULL, size, NULL, 0, NULL, NULL);
	//advect(Fluid, Fluid->d0, Fluid->d1, Fluid->u0, Fluid->v0, dt);

	clEnqueueReadBuffer(Fluid->Context.CommandQueue, Fluid->d0, CL_TRUE, 0, sizeof(float)*Fluid->w*Fluid->h, Fluid->den, 0, NULL, NULL);
}

float Fluid2D_GetDensity(Fluid2D_t *Fluid, int x, int y)
{
//	return Fluid->d0[IX(x, y, Fluid->w, Fluid->h)];
	return Fluid->den[IX(x, y, Fluid->w, Fluid->h)];
}

void Fluid2D_AddDensityVelocity(Fluid2D_t *Fluid, int x, int y, float u, float v, float d)
{
	//Fluid->u0[IX(x, y, Fluid->w, Fluid->h)]+=u;
	//Fluid->v0[IX(x, y, Fluid->w, Fluid->h)]+=v;
	//Fluid->d0[IX(x, y, Fluid->w, Fluid->h)]+=d;
	size_t size[1]={ 1 };

	clSetKernelArg(Fluid->AddDensityVelocity, 0, sizeof(cl_mem), (void *)&Fluid->u0);
	clSetKernelArg(Fluid->AddDensityVelocity, 1, sizeof(cl_mem), (void *)&Fluid->v0);
	clSetKernelArg(Fluid->AddDensityVelocity, 2, sizeof(cl_mem), (void *)&Fluid->d0);
	clSetKernelArg(Fluid->AddDensityVelocity, 3, sizeof(cl_int), (void *)&x);
	clSetKernelArg(Fluid->AddDensityVelocity, 4, sizeof(cl_int), (void *)&y);
	clSetKernelArg(Fluid->AddDensityVelocity, 5, sizeof(cl_float), (void *)&u);
	clSetKernelArg(Fluid->AddDensityVelocity, 6, sizeof(cl_float), (void *)&v);
	clSetKernelArg(Fluid->AddDensityVelocity, 7, sizeof(cl_float), (void *)&d);
	clSetKernelArg(Fluid->AddDensityVelocity, 8, sizeof(cl_int), (void *)&Fluid->w);
	clSetKernelArg(Fluid->AddDensityVelocity, 9, sizeof(cl_int), (void *)&Fluid->h);

	clEnqueueNDRangeKernel(Fluid->Context.CommandQueue, Fluid->AddDensityVelocity, 1, NULL, size, NULL, 0, NULL, NULL);
}

void Fluid2D_SetDensityVelocity(Fluid2D_t *Fluid, int x, int y, float u, float v, float d)
{
	//Fluid->u0[IX(x, y, Fluid->w, Fluid->h)]=u;
	//Fluid->v0[IX(x, y, Fluid->w, Fluid->h)]=v;
	//Fluid->d0[IX(x, y, Fluid->w, Fluid->h)]=d;
	size_t size[1]={ 1 };

	clSetKernelArg(Fluid->SetDensityVelocity, 0, sizeof(cl_mem), (void *)&Fluid->u0);
	clSetKernelArg(Fluid->SetDensityVelocity, 1, sizeof(cl_mem), (void *)&Fluid->v0);
	clSetKernelArg(Fluid->SetDensityVelocity, 2, sizeof(cl_mem), (void *)&Fluid->d0);
	clSetKernelArg(Fluid->SetDensityVelocity, 3, sizeof(cl_int), (void *)&x);
	clSetKernelArg(Fluid->SetDensityVelocity, 4, sizeof(cl_int), (void *)&y);
	clSetKernelArg(Fluid->SetDensityVelocity, 5, sizeof(cl_float), (void *)&u);
	clSetKernelArg(Fluid->SetDensityVelocity, 6, sizeof(cl_float), (void *)&v);
	clSetKernelArg(Fluid->SetDensityVelocity, 7, sizeof(cl_float), (void *)&d);
	clSetKernelArg(Fluid->SetDensityVelocity, 8, sizeof(cl_int), (void *)&Fluid->w);
	clSetKernelArg(Fluid->SetDensityVelocity, 9, sizeof(cl_int), (void *)&Fluid->h);

	clEnqueueNDRangeKernel(Fluid->Context.CommandQueue, Fluid->SetDensityVelocity, 1, NULL, size, NULL, 0, NULL, NULL);
}
