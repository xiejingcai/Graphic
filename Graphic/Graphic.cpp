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

// ȫ�ֱ���: 
HINSTANCE hInst;                                // ��ǰʵ��
WCHAR szTitle[MAX_LOADSTRING];                  // �������ı�
WCHAR szWindowClass[MAX_LOADSTRING];            // ����������
HDC hdc = NULL;					//���洰�� hdc ��ȫ�ֱ���
HDC buffer_hdc = NULL;			//�󱸴��� hdc buffer
UINT_PTR TimerID;
PAINTSTRUCT ps;
HBITMAP bufferbmp;
HWND hWinHandle = NULL; //���洰�ھ����ȫ�ֱ���

// �����3D������������
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
*	�ж��Ƿ���ʾ��ƽ��
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
*	fragement shader + render��ѡ��3D��Ƭ��ͶӰ��ʾ��Ƭ��
*	��������ÿһ������ɫ�����������Ϣѡ���Ƿ���Ƶ�ǰ��
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
*	fragement shader + render����3D��Ƭ����ɼ��㣬Ȼ��ͶӰ��ʾ
*	������ת��Ķ������꣬��Ϊ��ת��ƽ����ȻΪcube_side*cube_side����
*	����ת��Ŀռ���Ƭ�����ɫ�������������Ϣѡ���������۲���դ
*	�������
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
				//��ֵ����û��ӳ�䵽��դ�񣬷�����Ƭ����Щ��û����ɫ
				if (zpos >= 1 && ZBuffer[zpos - 1] == 0xffffffff) {
					ZBuffer[zpos - 1] = (int)(vector_o.z * 10);
					SetPixel(hdc, (int)(vector_o.x - 1), (int)(vector_o.y), c);
				}
			}
			else if ((int)(vector_o.z * 10) >= ZBuffer[zpos]) {
				ZBuffer[zpos] = (int)(vector_o.z * 10);
				SetPixel(hdc, (int)(vector_o.x), (int)(vector_o.y), c);
				//��ֵ����û��ӳ�䵽��դ�񣬷�����Ƭ����Щ��û����ɫ
				if (zpos >= 1 && ZBuffer[zpos - 1] == 0xffffffff) {
					ZBuffer[zpos - 1] = (int)(vector_o.z * 10);
					SetPixel(hdc, (int)(vector_o.x - 1), (int)(vector_o.y), c);
				}
			}			
		}
	}
}
/*
*	���ƴ���ͼ�ķ���
*/
void CubeDraw(HDC& hdc)
{
	VertexP Pt[4];
	for (int i = 0; i < 6; i++) {
		//�����Ҫ���Ƶ���Ƭ���ڵ�ס���Ͳ���Ҫ����
		//�˲������Լ��ټ������������˲���Ҳû����
		//Zbufferͬ�������ڵ����
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

		//Scan��ɫ�㷨Ч�����ã�������Ҳ���СһЩ
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
*	Cube��ת
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

// ���Ʒ�����ת
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

// �˴���ģ���а����ĺ�����ǰ������: 
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

    // TODO: �ڴ˷��ô��롣
	hbmp = (HBITMAP)LoadImage(NULL, texture, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
	if (hbmp == NULL)
		return -1;

	GetBitmapBits(hbmp, 200 * 1200 * 3, color);

    // ��ʼ��ȫ���ַ���
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_GRAPHIC, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // ִ��Ӧ�ó����ʼ��: 
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GRAPHIC));

	// ��ʼ����ͼ����
	hdc = GetDC(hWinHandle);
	buffer_hdc = CreateCompatibleDC(NULL);
	bufferbmp = CreateCompatibleBitmap(hdc, width, height);
	SelectObject(buffer_hdc, bufferbmp);

	// ��ʱ����
	TimerID = SetTimer(hWinHandle, 1, delay, (TIMERPROC)TimerProc);

    MSG msg;
    // ����Ϣѭ��: 
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
	/* �ͷ���Դ */
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
//  ����: MyRegisterClass()
//
//  Ŀ��: ע�ᴰ���ࡣ
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
//   ����: InitInstance(HINSTANCE, int)
//
//   Ŀ��: ����ʵ�����������������
//
//   ע��: 
//
//        �ڴ˺����У�������ȫ�ֱ����б���ʵ�������
//        ��������ʾ�����򴰿ڡ�
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // ��ʵ������洢��ȫ�ֱ�����

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
//  ����: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  Ŀ��:    ���������ڵ���Ϣ��
//
//  WM_COMMAND  - ����Ӧ�ó���˵�
//  WM_PAINT    - ����������
//  WM_DESTROY  - �����˳���Ϣ������
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
            // �����˵�ѡ��: 
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

// �����ڡ������Ϣ�������
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
