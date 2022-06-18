#include <windows.h>
#include <ddraw.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include "opencl.h"
#include "fluid2d.h"

#pragma intrinsic(__rdtsc)

#ifdef WIN32
#include <Windows.h>
#define DBGPRINTF(...) { char buf[512]; snprintf(buf, sizeof(buf), __VA_ARGS__); OutputDebugString(buf); }
#else
#define DBGPRINTF(...) { fprintf(stderr, __VA_ARGS__); }
#endif

LPDIRECTDRAW7 lpDD=NULL;
LPDIRECTDRAWSURFACE7 lpDDSFront=NULL;
LPDIRECTDRAWSURFACE7 lpDDSBack=NULL;
HWND hWnd=NULL;

char *szAppName="DirectDraw";

RECT WindowRect;

int Width=1024, Height=1024;

BOOL Done=FALSE, Key[256];

unsigned __int64 Frequency, StartTime, EndTime;
float fTimeStep, fTime=0.0f;

int mx, my;

Fluid2D_t Fluid;

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void Render(void);
int Init(void);
int Create(void);
void Destroy(void);

unsigned __int64 rdtsc(void)
{
	return __rdtsc();
}

unsigned __int64 GetFrequency(void)
{
	unsigned __int64 TimeStart, TimeStop, TimeFreq;
	unsigned __int64 StartTicks, StopTicks;
	volatile unsigned __int64 i;

	QueryPerformanceFrequency((LARGE_INTEGER *)&TimeFreq);

	QueryPerformanceCounter((LARGE_INTEGER *)&TimeStart);
	StartTicks=rdtsc();

	for(i=0;i<1000000;i++);

	StopTicks=rdtsc();
	QueryPerformanceCounter((LARGE_INTEGER *)&TimeStop);

	return (StopTicks-StartTicks)*TimeFreq/(TimeStop-TimeStart);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int iCmdShow)
{
	WNDCLASS wc;
	MSG msg;

	wc.style=CS_VREDRAW|CS_HREDRAW|CS_OWNDC;
	wc.lpfnWndProc=WndProc;
	wc.cbClsExtra=0;
	wc.cbWndExtra=0;
	wc.hInstance=hInstance;
	wc.hIcon=LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor=LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground=GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName=NULL;
	wc.lpszClassName=szAppName;

	RegisterClass(&wc);

	WindowRect.left=0;
	WindowRect.right=Width;
	WindowRect.top=0;
	WindowRect.bottom=Height;

	AdjustWindowRect(&WindowRect, WS_OVERLAPPEDWINDOW, FALSE);

	hWnd=CreateWindow(szAppName, szAppName, WS_POPUPWINDOW|WS_CAPTION|WS_CLIPSIBLINGS, CW_USEDEFAULT, CW_USEDEFAULT, WindowRect.right-WindowRect.left, WindowRect.bottom-WindowRect.top, NULL, NULL, hInstance, NULL);

	ShowWindow(hWnd, SW_SHOW);
	SetForegroundWindow(hWnd);

	if(!Create())
	{
		DestroyWindow(hWnd);
		return -1;
	}

	if(!Init())
	{
		Destroy();
		DestroyWindow(hWnd);
		return -1;
	}

	if(!Fluid2D_Init(&Fluid, Width, Height, 0.0000005f, 0.00000005f))
	{
		Destroy();
		DestroyWindow(hWnd);
		return -1;
	}

	Frequency=GetFrequency();
	EndTime=rdtsc();

	while(!Done)
	{
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if(msg.message==WM_QUIT)
				Done=1;
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			RECT RectSrc, RectDst;
			POINT Point={ 0, 0 };

			StartTime=EndTime;
			EndTime=rdtsc();

			if(IDirectDrawSurface7_IsLost(lpDDSBack)==DDERR_SURFACELOST)
				IDirectDrawSurface7_Restore(lpDDSBack);

			if(IDirectDrawSurface7_IsLost(lpDDSFront)==DDERR_SURFACELOST)
				IDirectDrawSurface7_Restore(lpDDSFront);

			Render();

			fTimeStep=(float)(EndTime-StartTime)/Frequency;
			fTime+=fTimeStep;

			ClientToScreen(hWnd, &Point);
			GetClientRect(hWnd, &RectDst);
			OffsetRect(&RectDst, Point.x, Point.y);
			SetRect(&RectSrc, 0, 0, Width, Height);

			IDirectDrawSurface7_Blt(lpDDSFront, &RectDst, lpDDSBack, &RectSrc, DDBLT_WAIT, NULL);
		}
	}

	Fluid2D_Destroy(&Fluid);

	Destroy();
	DestroyWindow(hWnd);

	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static POINT old;
	POINT pos, delta;

	switch(uMsg)
	{
		case WM_CREATE:
			break;

		case WM_CLOSE:
			PostQuitMessage(0);
			break;

		case WM_DESTROY:
			break;

		case WM_SIZE:
			break;

		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
			SetCapture(hWnd);

			GetCursorPos(&pos);
			old.x=pos.x;
			old.y=pos.y;
			break;

		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
			ReleaseCapture();
			break;

		case WM_MOUSEMOVE:
			GetCursorPos(&pos);

			if(!wParam)
			{
				old.x=pos.x;
				old.y=pos.y;
				break;
			}

			delta.x=pos.x-old.x;
			delta.y=pos.y-old.y;

			switch(wParam)
			{
				case MK_LBUTTON:
					ScreenToClient(hWnd, &pos);
					Fluid2D_AddDensityVelocity(&Fluid, pos.x, pos.y, (float)delta.x, (float)delta.y, 4.0f);
					break;

				case MK_MBUTTON:
					break;

				case MK_RBUTTON:
					break;
			}
			break;

		case WM_KEYDOWN:
			Key[wParam]=TRUE;

			switch(wParam)
			{
				case VK_ESCAPE:
					PostQuitMessage(0);
					break;

				default:
					break;
			}
			break;

		case WM_KEYUP:
			Key[wParam]=FALSE;
			break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void Clear(LPDIRECTDRAWSURFACE7 lpDDS)
{
	DDBLTFX ddbltfx;

	ddbltfx.dwSize=sizeof(DDBLTFX);
	ddbltfx.dwFillColor=0x00000000;

	if(IDirectDrawSurface7_Blt(lpDDS, NULL, NULL, NULL, DDBLT_COLORFILL, &ddbltfx)!=DD_OK)
		return;
}

unsigned char *fb;
int depth;

void point(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
	int i=depth*(y*Width+x);

	fb[i+0]=b;
	fb[i+1]=g;
	fb[i+2]=r;
}

void line(int x, int y, int x2, int y2, unsigned char r, unsigned char g, unsigned char b)
{
	int i;
	int dx=abs(x-x2);
	int dy=abs(y-y2);
	float s=(float)(0.98f/(dx>dy?dx:dy)), t=0.0f;

	while(t<1.0)
	{
		dx=min(Width-1, max(0, (int)((1.0f-t)*x+t*x2)));
		dy=min(Height-1, max(0, (int)((1.0f-t)*y+t*y2)));

		i=depth*(dy*Width+dx);

		fb[i+0]=b;
		fb[i+1]=g;
		fb[i+2]=r;

		t+=s;
	}
}

void Render(void)
{
	DDSURFACEDESC2 ddsd;
	HRESULT ret=DDERR_WASSTILLDRAWING;

	Fluid2D_AddDensityVelocity(&Fluid, 50, (Fluid.h/2)-2, 0.5f, -0.75f, 4.0f);
	Fluid2D_AddDensityVelocity(&Fluid, 50, (Fluid.h/2),   0.8f,  0.0f, 4.0f);
	Fluid2D_AddDensityVelocity(&Fluid, 50, (Fluid.h/2)+2, 0.5f,  0.75f, 4.0f);

	Fluid2D_AddDensityVelocity(&Fluid, Fluid.w-50, (Fluid.h/2)-2, -0.5f, -0.75f, 4.0f);
	Fluid2D_AddDensityVelocity(&Fluid, Fluid.w-50, (Fluid.h/2),   -0.8f,  0.0f, 4.0f);
	Fluid2D_AddDensityVelocity(&Fluid, Fluid.w-50, (Fluid.h/2)+2, -0.5f,  0.75f, 4.0f);

	Fluid2D_Step(&Fluid, fTimeStep);

	Clear(lpDDSBack);

	memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
	ddsd.dwSize=sizeof(ddsd);

	while(ret==DDERR_WASSTILLDRAWING)
		ret=IDirectDrawSurface7_Lock(lpDDSBack, NULL, &ddsd, 0, NULL);

	fb=(unsigned char *)ddsd.lpSurface;
	depth=ddsd.ddpfPixelFormat.dwRGBBitCount/8;
	unsigned long *lfb=(unsigned long *)((unsigned char *)ddsd.lpSurface);

	for(unsigned int y=1;y<ddsd.dwHeight-1;y++)
	{
		for(unsigned int x=1;x<ddsd.dwWidth-1;x++)
		{
			unsigned long d=min(255, max(0, (unsigned long)(Fluid2D_GetDensity(&Fluid, x, y)*255.0f)));
			lfb[y*ddsd.dwWidth+x]=(d<<16)|(d<<8)|d;
		}
	}

	IDirectDrawSurface7_Unlock(lpDDSBack, NULL);
}

int Init(void)
{
	return 1;
}

BOOL Create(void)
{
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAWCLIPPER lpClipper=NULL;

	if(DirectDrawCreateEx(NULL, &lpDD, &IID_IDirectDraw7, NULL)!=DD_OK)
	{
		MessageBox(hWnd, "DirectDrawCreateEx failed.", "Error", MB_OK);
		return FALSE;
	}

	if(IDirectDraw7_SetCooperativeLevel(lpDD, hWnd, DDSCL_NORMAL)!=DD_OK)
	{
		MessageBox(hWnd, "IDirectDraw7_SetCooperativeLevel failed.", "Error", MB_OK);
		return FALSE;
	}

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize=sizeof(ddsd);
	ddsd.dwFlags=DDSD_CAPS;
	ddsd.ddsCaps.dwCaps=DDSCAPS_PRIMARYSURFACE;

	if(IDirectDraw7_CreateSurface(lpDD, &ddsd, &lpDDSFront, NULL)!=DD_OK)
	{
		MessageBox(hWnd, "IDirectDraw7_CreateSurface (Front) failed.", "Error", MB_OK);
		return FALSE;
	}

	ddsd.dwFlags=DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS;
	ddsd.dwWidth=Width;
	ddsd.dwHeight=Height;
	ddsd.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN;

	if(IDirectDraw7_CreateSurface(lpDD, &ddsd, &lpDDSBack, NULL)!=DD_OK)
	{
		MessageBox(hWnd, "IDirectDraw7_CreateSurface (Back) failed.", "Error", MB_OK);
		return FALSE;
	}

	if(IDirectDraw7_CreateClipper(lpDD, 0, &lpClipper, NULL)!=DD_OK)
	{
		MessageBox(hWnd, "IDirectDraw7_CreateClipper failed.", "Error", MB_OK);
		return FALSE;
	}

	if(IDirectDrawClipper_SetHWnd(lpClipper, 0, hWnd)!=DD_OK)
	{
		MessageBox(hWnd, "IDirectDrawClipper_SetHWnd failed.", "Error", MB_OK);
		return FALSE;
	}

	if(IDirectDrawSurface_SetClipper(lpDDSFront, lpClipper)!=DD_OK)
	{
		MessageBox(hWnd, "IDirectDrawSurface_SetClipper failed.", "Error", MB_OK);
		return FALSE;
	}

	if(lpClipper!=NULL)
	{
		IDirectDrawClipper_Release(lpClipper);
		lpClipper=NULL;
	}

	return TRUE;
}

void Destroy(void)
{
	if(lpDDSBack!=NULL)
	{
		IDirectDrawSurface7_Release(lpDDSBack);
		lpDDSBack=NULL;
	}

	if(lpDDSFront!=NULL)
	{
		IDirectDrawSurface7_Release(lpDDSFront);
		lpDDSFront=NULL;
	}

	if(lpDD!=NULL)
	{
		IDirectDraw7_Release(lpDD);
		lpDD=NULL;
	}
}
