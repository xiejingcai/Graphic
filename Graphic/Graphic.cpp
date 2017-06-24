#include "stdafx.h"
#include <stdio.h>
#include "Graphic.h"
#include <math.h>
#include <windows.h>

#define M_PI 3.1415926
#define MAX_LOADSTRING 100
#define width 720
#define height 480
#define cube_side 200.00

// 全局变量: 
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名
HDC hdc = NULL;					//保存窗口 hdc 的全局变量
HDC buffer_hdc = NULL;			//后备窗口 hdc buffer
UINT_PTR TimerID;
PAINTSTRUCT ps;
HBITMAP bufferbmp;
HWND hWinHandle = NULL; //保存窗口句柄的全局变量

// 顶点和3D物体外形描述
HBITMAP hbmp; 
BYTE color[200 * 1200 * 3];
wchar_t *texture = TEXT("dice.bmp");


int delay = 20;
int ZBuffer[width*height];

typedef struct VertexPoint{
	double x;
	double y;
	double z;
} VertexP;
typedef struct AngleAxis {
	double dx;
	double dy;
	double dz;
} AngleA;
typedef struct ObjectCube {
	AngleA aa;
	VertexP vp[8];
	VertexP vp_run[8];
	int sf[6][4];
} OCube;

OCube cube = {
	{0,0,0},
	{
		{ 0.0,				0.0,			0.0 },
		{ cube_side - 1.0,	0.0,			0.0 },
		{ 0.0,				cube_side - 1.0,	0.0 },
		{ 0.0,				0.0,			cube_side - 1.0},
		{ cube_side - 1.0,	cube_side - 1.0,	0.0 },
		{ 0.0,				cube_side - 1.0,	cube_side - 1.0 },
		{ cube_side - 1.0,	0.0,			cube_side - 1.0 },
		{ cube_side - 1.0,	cube_side - 1.0,	cube_side - 1.0 }
	},
	{
		{ 0.0,				0.0,			 0.0 },
		{ cube_side - 1.0,	0.0,			 0.0 },
		{ 0.0,				cube_side - 1.0, 0.0 },
		{ 0.0,				0.0,			 cube_side - 1.0 },
		{ cube_side - 1.0,	cube_side - 1.0, 0.0 },
		{ 0.0,				cube_side - 1.0, cube_side - 1.0 },
		{ cube_side - 1.0,	0.0,			 cube_side - 1.0 },
		{ cube_side - 1.0,	cube_side - 1.0, cube_side - 1.0 }
	},
	{
		{0,2,4,1},
		{1,4,7,6},
		{6,7,5,3},
		{3,5,2,0},
		{2,5,7,4},
		{0,1,6,3}
	}
};
OCube tmp;
/**
*	判断是否显示此平面
*/
bool Present(VertexP &v1, VertexP &v2, VertexP &v3)
{
	double z = 0.0;
	double a[] = { 0,0,0 }, b[] = { 0,0,0 };
	a[0] = v2.x - v1.x;
	a[1] = v2.y - v1.y;
	a[2] = v2.z - v1.z;
	b[0] = v3.x - v1.x;
	b[1] = v3.y - v1.y;
	b[2] = v3.z - v1.z;
	z = a[0] * b[1] - a[1] * b[0];

	if (z < 0.0)
		return false;
	else
		return true;
}
/**
*	fragement shader + render，选择3D面片的投影显示面片，
*	对区域内每一个点着色，根据深度信息选择是否绘制当前点
*/
void ScanShaderRender(HDC& hdc, VertexP p[], int textureoffset)
{
	double x, y;
	double minx, maxx, miny, maxy;

	int offset = 0;
	COLORREF c;
	int zpos;
	double ratio_i, ratio_j;
	VertexP o = { 0,0,0 };
	VertexP o1 = { p[1].x - p[0].x, p[1].y - p[0].y ,p[1].z - p[0].z };
	VertexP o2 = { p[3].x - p[0].x, p[3].y - p[0].y ,p[3].z - p[0].z };
	
	minx = p[0].x; maxx = p[0].x; miny = p[0].y; maxy = p[0].y;
	for (int i = 1; i < 4; i++) {
		minx = minx < p[i].x ? minx : p[i].x;
		miny = miny < p[i].y ? miny : p[i].y;
		maxx = maxx > p[i].x ? maxx : p[i].x;
		maxy = maxy > p[i].y ? maxy : p[i].y;
	}
	for (x = minx; x <= maxx; x+=1) {
		for (y = miny; y <= maxy; y+=1) {
			o.x = x - p[0].x;
			o.y = y - p[0].y;

			ratio_i = (o2.y*o.x - o2.x*o.y) / (o1.x*o2.y - o2.x*o1.y);
			ratio_j = (o1.x*o.y - o1.y*o.x) / (o1.x*o2.y - o2.x*o1.y);
			
			if ((ratio_i >= -0.005) && (ratio_j >= -0.005) && (ratio_i <= 1.005) && (ratio_j <= 1.005)) {
				zpos = (int)(y)*width + (int)(x);
				o.z = ratio_i*o1.z + ratio_j*o2.z + p[0].z;
				offset = (textureoffset * 200 * 200 + (int)(ratio_i * 200) + (int)(ratio_j * 200) * 200) * 3;
				offset = offset > 6 * 200 * 200 * 3 ? (offset - 200*3) : offset;
				c = RGB(color[offset + 2], color[offset + 1], color[offset]);
				if (ZBuffer[zpos] == 0xffffffff) {
					ZBuffer[zpos] = (int)(o.z * 10);
					SetPixel(hdc, (int)(x), (int)(y), c);
				}
				else if ((int)(o.z * 10) >= ZBuffer[zpos]) {
					ZBuffer[zpos] = (int)(o.z * 10);
					SetPixel(hdc, (int)(x), (int)(y), c);
				}
			}
		}
	}
}
/**
*	fragement shader + render，在3D面片上完成计算，然后投影显示
*	计算旋转后的顶点坐标，认为旋转后平面仍然为cube_side*cube_side点阵，
*	对旋转后的空间面片逐点着色，最后根据深度信息选择绘制最靠近观测者栅
*	格点像素
*/
void OneOneShaderRender(HDC& hdc,VertexP p[],int textureoffset)
{
	int offset = 0;
	int zpos;
	double ratio_i, ratio_j;
	VertexP vector_o = {0,0,0};
	VertexP vector_o1 = { p[1].x - p[0].x, p[1].y - p[0].y ,p[1].z - p[0].z };
	VertexP vector_o2 = { p[3].x - p[0].x, p[3].y - p[0].y ,p[3].z - p[0].z };

	for (int i = 0; i < cube_side; i++) {
		for (int j = 0; j < cube_side; j++) {
			ratio_i = i / cube_side;
			ratio_j = j / cube_side;
			vector_o.x = ratio_i*vector_o1.x + ratio_j*vector_o2.x + p[0].x;
			vector_o.y = ratio_i*vector_o1.y + ratio_j*vector_o2.y + p[0].y;
			vector_o.z = ratio_i*vector_o1.z + ratio_j*vector_o2.z + p[0].z;
			zpos = (int)(vector_o.y)*width + (int)(vector_o.x);

			offset = (textureoffset * 200 * 200 + (int)(ratio_i * 200) + (int)(ratio_j * 200) * 200) * 3;
			COLORREF c = RGB(color[offset + 2], color[offset + 1], color[offset]);
			
			if (ZBuffer[zpos] == 0xffffffff) {
				ZBuffer[zpos] = (int)(vector_o.z * 10);
				SetPixel(hdc, (int)(vector_o.x), (int)(vector_o.y), c);
				//插值补偿没有映射到的栅格，否则面片上有些点没有着色
				if (zpos >= 1 && ZBuffer[zpos - 1] == 0xffffffff) {
					ZBuffer[zpos - 1] = (int)(vector_o.z * 10);
					SetPixel(hdc, (int)(vector_o.x - 1), (int)(vector_o.y), c);
				}
			}
			else if ((int)(vector_o.z * 10) >= ZBuffer[zpos]) {
				ZBuffer[zpos] = (int)(vector_o.z * 10);
				SetPixel(hdc, (int)(vector_o.x), (int)(vector_o.y), c);
				//插值补偿没有映射到的栅格，否则面片上有些点没有着色
				if (zpos >= 1 && ZBuffer[zpos - 1] == 0xffffffff) {
					ZBuffer[zpos - 1] = (int)(vector_o.z * 10);
					SetPixel(hdc, (int)(vector_o.x - 1), (int)(vector_o.y), c);
				}
			}			
		}
	}
}
/*
*	绘制带贴图的方块
*/
void CubeDraw(HDC& hdc)
{
	VertexP Pt[4];
	for (int i = 0; i < 6; i++) {
		//如果将要绘制的面片被遮挡住，就不需要绘制
		//此操作可以减少计算量，不做此操作也没问题
		//Zbuffer同样用于遮挡检测
		if (!Present(cube.vp_run[cube.sf[i][0]], cube.vp_run[cube.sf[i][1]], cube.vp_run[cube.sf[i][2]])) {
			 continue;
		}
		Pt[0].x = cube.vp_run[cube.sf[i][0]].x; 
		Pt[0].y = cube.vp_run[cube.sf[i][0]].y;
		Pt[0].z = cube.vp_run[cube.sf[i][0]].z;

		Pt[1].x = cube.vp_run[cube.sf[i][1]].x; 
		Pt[1].y = cube.vp_run[cube.sf[i][1]].y;
		Pt[1].z = cube.vp_run[cube.sf[i][1]].z;

		Pt[2].x = cube.vp_run[cube.sf[i][2]].x; 
		Pt[2].y = cube.vp_run[cube.sf[i][2]].y;
		Pt[2].z = cube.vp_run[cube.sf[i][2]].z;

		Pt[3].x = cube.vp_run[cube.sf[i][3]].x; 
		Pt[3].y = cube.vp_run[cube.sf[i][3]].y;
		Pt[3].z = cube.vp_run[cube.sf[i][3]].z;

		//Scan着色算法效果更好，计算量也相对小一些
		//OneOneShaderRender(hdc, Pt, i);
		ScanShaderRender(hdc, Pt, i);
	}
}

/**
*	Cube brush
*/
void CubeDraw(HDC& hdc,bool fill)
{
	int degree = (int)(cube.aa.dx + cube.aa.dy + cube.aa.dz);
	BYTE color = (BYTE)(0xff * sin((degree%360)*M_PI/180)/4)+0xff*3/4;
	HBRUSH  Brush[6];
	Brush[0] = CreateSolidBrush(RGB(000, 000, color));
	Brush[1] = CreateSolidBrush(RGB(000, color, 000));
	Brush[2] = CreateSolidBrush(RGB(color, 000, 000));
	Brush[3] = CreateSolidBrush(RGB(000, color, color));
	Brush[4] = CreateSolidBrush(RGB(color, 000, color));
	Brush[5] = CreateSolidBrush(RGB(color, color, 000));
	
	POINT Pt[5];
	for (int i = 0; i < 6; i++) {
		if (!Present(cube.vp_run[cube.sf[i][0]], cube.vp_run[cube.sf[i][1]], cube.vp_run[cube.sf[i][2]])) {
			continue;
		}

		Pt[0].x = (long)cube.vp_run[cube.sf[i][0]].x; Pt[0].y = (long)cube.vp_run[cube.sf[i][0]].y;
		Pt[1].x = (long)cube.vp_run[cube.sf[i][1]].x; Pt[1].y = (long)cube.vp_run[cube.sf[i][1]].y;
		Pt[2].x = (long)cube.vp_run[cube.sf[i][2]].x; Pt[2].y = (long)cube.vp_run[cube.sf[i][2]].y;
		Pt[3].x = (long)cube.vp_run[cube.sf[i][3]].x; Pt[3].y = (long)cube.vp_run[cube.sf[i][3]].y;

		if (fill)
			SelectObject(hdc, Brush[i]);
		Polygon(hdc, Pt, 4);
	}
	DeleteObject(Brush[0]);
	DeleteObject(Brush[1]);
	DeleteObject(Brush[2]);
	DeleteObject(Brush[3]);
	DeleteObject(Brush[4]);
	DeleteObject(Brush[5]);
}

/**
*	Cube旋转
*/
void CubeRotate(double dx, double dy, double dz)
{
	double asdx = sin(dx*M_PI / 180);
	double asdy = sin(dy*M_PI / 180);
	double asdz = sin(dz*M_PI / 180);
	double acdx = cos(dx*M_PI / 180);
	double acdy = cos(dy*M_PI / 180);
	double acdz = cos(dz*M_PI / 180);

	double x[2], y[2], z[2];
	for (int i = 0; i < 8; i++) {
		cube.vp_run[i].x = cube.vp[i].x - cube_side / 2;
		cube.vp_run[i].y = cube.vp[i].y - cube_side / 2;
		cube.vp_run[i].z = cube.vp[i].z - cube_side / 2;

		x[0] = cube.vp_run[i].x * acdx + cube.vp_run[i].y * asdx;
		y[0] = -cube.vp_run[i].x * asdx + cube.vp_run[i].y * acdx;
		z[0] = cube.vp_run[i].z;

		x[1] = x[0] * acdy + z[0] * asdy;
		y[1] = y[0];
		z[1] = -x[0] * asdy + z[0] * acdy;

		cube.vp_run[i].x = x[1] + width / 2;
		cube.vp_run[i].y = y[1] * acdz + z[1] * asdz + height / 2;
		cube.vp_run[i].z = -y[1] * asdz + z[1] * acdz + cube_side / 2;
	}
}

// 控制方块旋转
void CALLBACK TimerProc(
	HWND hWnd,
	UINT uMsg,
	UINT idEvent,
	DWORD dwTime
)
{
	cube.aa.dx += 1;
	cube.aa.dy += 1;
	cube.aa.dz += 1;
	
	BitBlt(buffer_hdc, 0, 0, width, height, NULL, 0, 0, WHITENESS);
	memset(ZBuffer,0xffffffff,sizeof(ZBuffer));

	CubeRotate(cube.aa.dx, cube.aa.dy, cube.aa.dz);
	
	CubeDraw(buffer_hdc);
	
	BitBlt(hdc, 0, 0, width, height, buffer_hdc, 0, 0, SRCCOPY);
}

// 此代码模块中包含的函数的前向声明: 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此放置代码。
	hbmp = (HBITMAP)LoadImage(NULL, texture, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
	if (hbmp == NULL)
		return -1;

	GetBitmapBits(hbmp, 200 * 1200 * 3, color);

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_GRAPHIC, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化: 
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GRAPHIC));

	// 初始化绘图窗口
	hdc = GetDC(hWinHandle);
	buffer_hdc = CreateCompatibleDC(NULL);
	bufferbmp = CreateCompatibleBitmap(hdc, width, height);
	SelectObject(buffer_hdc, bufferbmp);

	// 定时任务
	TimerID = SetTimer(hWinHandle, 1, delay, (TIMERPROC)TimerProc);

    MSG msg;
    // 主消息循环: 
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
	/* 释放资源 */
	KillTimer(NULL, TimerID);
	if (bufferbmp)
		DeleteObject(bufferbmp);
	if (hdc)
		ReleaseDC(hWinHandle, hdc);
	if (buffer_hdc)
		DeleteDC(buffer_hdc);

    return (int) msg.wParam;
}

//
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GRAPHIC));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_GRAPHIC);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
//   注释: 
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   hWinHandle = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      (1920-width)/2, (1080-height)/2, width, height, nullptr, nullptr, hInstance, nullptr);

   if (!hWinHandle)
   {
      return FALSE;
   }

   ShowWindow(hWinHandle, nCmdShow);
   UpdateWindow(hWinHandle);

   return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择: 
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
			hdc = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
