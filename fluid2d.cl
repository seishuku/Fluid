#define IX(x, y, sx, sy) (min(sx-1, max(1, (x)))+min(sy-1, max(1, (y)))*sx)

kernel void set_density_velocity(global float *u0, global float *v0, global float *d0, int x, int y, float u, float v, float d, int w, int h)
{
	u0[IX(x, y, w, h)]=u;
	v0[IX(x, y, w, h)]=v;
	d0[IX(x, y, w, h)]=d;
}

kernel void add_density_velocity(global float *u0, global float *v0, global float *d0, int x, int y, float u, float v, float d, int w, int h)
{
	u0[IX(x, y, w, h)]+=u;
	v0[IX(x, y, w, h)]+=v;
	d0[IX(x, y, w, h)]+=d;
}

void lin_solve(float *x, float *x0, float a, float c, int iter)
{
	float cRecip=1.0f/c;
	int i=get_global_id(0);
	int j=get_global_id(1);
	int w=get_global_size(0);
	int h=get_global_size(1);

	for(int l=0;l<iter;l++)
	{
		x[IX(i, j, w, h)]=
		(
			x0[IX(i, j, w, h)]+
			a*
			(
				x[IX(i+1, j, w, h)]+
				x[IX(i-1, j, w, h)]+
				x[IX(i, j+1, w, h)]+
				x[IX(i, j-1, w, h)]
			)
		)*cRecip;
	}
}

kernel void diffuse(global float *x, global float *x0, float diff, float dt, int iter)
{
	int w=get_global_size(0);
	int h=get_global_size(1);

	float a=dt*diff*(w-2)*(h-2);

	lin_solve(x, x0, a, 1.0f+6.0f*a, iter);
}

kernel void advect(global float *d, global float *d0, global float *velocX, global float *velocY, float dt)
{
	int i=get_global_id(0);
	int j=get_global_id(1);
	int w=get_global_size(0);
	int h=get_global_size(1);
	float dtx=dt*(w-2);
	float dty=dt*(h-2);

	float x=(float)i-dtx*velocX[IX(i, j, w, h)];
	float y=(float)j-dty*velocY[IX(i, j, w, h)];

	if(x<0.5f)
		x=0.5f;

	if(x>(float)w+0.5f)
		x=(float)w+0.5f;

	int i0=(int)floor(x);
	int i1=i0+1;

	if(y<0.5f)
		y=0.5f;

	if(y>(float)h+0.5f)
		y=(float)h+0.5f;

	int j0=(int)floor(y);
	int j1=j0+1;

	float s1=x-i0;
	float s0=1.0f-s1;
	float t1=y-j0;
	float t0=1.0f-t1;

	d[IX(i, j, w, h)]=
	(
		s0*
		(
			t0*d0[IX(i0, j0, w, h)]+
			t1*d0[IX(i0, j1, w, h)]
		)+
		s1*
		(
			t0*d0[IX(i1, j0, w, h)]+
			t1*d0[IX(i1, j1, w, h)]
		)
	);
}

kernel void project1(global float *velocX, global float *velocY, global float *p, global float *div)
{
	int i=get_global_id(0);
	int j=get_global_id(1);
	int w=get_global_size(0);
	int h=get_global_size(1);

	div[IX(i, j, w, h)]=
	-0.5f*
	(
		velocX[IX(i+1, j, w, h)]-
		velocX[IX(i-1, j, w, h)]+
		velocY[IX(i, j+1, w, h)]-
		velocY[IX(i, j-1, w, h)]
	)/w;

	p[IX(i, j, w, h)]=0.0f;
}

kernel void project2(global float *velocX, global float *velocY, global float *p, global float *div, int iter)
{
	lin_solve(p, div, 1, 6, iter);
}

kernel void project3(global float *velocX, global float *velocY, global float *p, global float *div)
{
	int i=get_global_id(0);
	int j=get_global_id(1);
	int w=get_global_size(0);
	int h=get_global_size(1);

	velocX[IX(i, j, w, h)]-=0.5f*(p[IX(i+1, j, w, h)]-p[IX(i-1, j, w, h)])*w;
	velocY[IX(i, j, w, h)]-=0.5f*(p[IX(i, j+1, w, h)]-p[IX(i, j-1, w, h)])*h;
}
