#include "stdafx.h"
#include "931901.kuznetsov.semyon.SystProg.lab4.h"
#include "resource.h"
#include <windows.h>
#include <windowsX.h>
#pragma once
#include <d2d1.h>
#include <list>
#include <memory>
using namespace std;

class BaseWindow
{
public:
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		BaseWindow *pThis = NULL;
		if (uMsg == WM_NCCREATE)
		{
			CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
			pThis = (BaseWindow*)pCreate->lpCreateParams;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);

			pThis->m_hwnd = hwnd;
		}
		else
		{
			pThis = (BaseWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		}
		if (pThis)
		{
			return pThis->HandleMessage(uMsg, wParam, lParam);
		}
		else
		{
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
	}
	BaseWindow() : m_hwnd(NULL) { }
	void Register()
	{
		WNDCLASS wc = { 0 };
		wc.lpfnWndProc = WindowProc;
		wc.hInstance = GetModuleHandle(NULL);
		wc.lpszClassName = ClassName();
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		RegisterClass(&wc);
	}
	BOOL Create(
		PCWSTR lpWindowName,
		DWORD dwStyle,
		DWORD dwExStyle = 0,
		int x = CW_USEDEFAULT,
		int y = CW_USEDEFAULT,
		int nWidth = CW_USEDEFAULT,
		int nHeight = CW_USEDEFAULT,
		HWND hWndParent = 0,
		HMENU hMenu = 0
	)
	{
		m_hwnd = CreateWindowEx(
			dwExStyle, ClassName(), lpWindowName, dwStyle, x, y,
			nWidth, nHeight, hWndParent, hMenu, GetModuleHandle(NULL), this
		);
		return (m_hwnd ? TRUE : FALSE);
	}
	HWND Window() const { return m_hwnd; }
protected:
	virtual PCWSTR  ClassName() const = 0;
	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
	HWND m_hwnd;
};
class DPIScale
{
	static float scaleX;
	static float scaleY;
public:
	static void Initialize(ID2D1Factory *pFactory)
	{
		FLOAT dpiX, dpiY;
		pFactory->GetDesktopDpi(&dpiX, &dpiY);
		scaleX = dpiX / 96.0f;
		scaleY = dpiY / 96.0f;
	}
	template <typename T>
	static float PixelsToDipsX(T x)
	{
		return static_cast<float>(x) / scaleX;
	}
	template <typename T>
	static float PixelsToDipsY(T y)
	{
		return static_cast<float>(y) / scaleY;
	}
	template <typename T>
	static D2D1_POINT_2F PixelsToDips(T x, T y)
	{
		return D2D1::Point2F(static_cast<float>(x) / scaleX, static_cast<float>(y) / scaleY);
	}
};
struct MyEllipse
{
	D2D1_ELLIPSE    ellipse;
	D2D1_COLOR_F    color;
	void Draw(ID2D1RenderTarget *pRT, ID2D1SolidColorBrush *pBrush, BOOL isSelect)
	{
		pBrush->SetColor(color);
		pRT->FillEllipse(ellipse, pBrush);
		pBrush->SetColor(isSelect ? D2D1::ColorF(D2D1::ColorF::White) :
			D2D1::ColorF(D2D1::ColorF::Black));
		pRT->DrawEllipse(ellipse, pBrush, 1.0f);
	}
	BOOL HitTest(float x, float y)
	{
		const float a = ellipse.radiusX;
		const float b = ellipse.radiusY;
		const float x1 = x - ellipse.point.x;
		const float y1 = y - ellipse.point.y;
		const float d = ((x1 * x1) / (a * a)) + ((y1 * y1) / (b * b));
		return d <= 1.0f;
	}
};
template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}
class Direct2DWindow :
	public BaseWindow
{
	ID2D1Factory            *pFactory;
protected:
	ID2D1HwndRenderTarget * pRenderTarget;
public:
	Direct2DWindow();
	~Direct2DWindow();
	virtual void    CalculateLayout() {}
	HRESULT CreateGraphicsResources();
	virtual HRESULT CreateAdditionalGraphicsResources();
	virtual void    DiscardGraphicsResources();
	void    OnPaint();
	void    Resize();
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void OnPaintScene() {}
	virtual bool HandleAdditionalMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *);
};
class MainWindow : public Direct2DWindow
{
	ID2D1SolidColorBrush    *pBrush;
	list<shared_ptr<MyEllipse>>             ellipses;
	enum Mode { DrawMode, SelectMode, DragMode } mode;
	D2D1_POINT_2F           ptMouse;
	HCURSOR hCursor;
	list<shared_ptr<MyEllipse>>::iterator   selection;
	int numColor;
	static D2D1::ColorF colors[7];
	shared_ptr<MyEllipse> Selection()
	{
		if (selection == ellipses.end())
		{
			return nullptr;
		}
		else
		{
			return (*selection);
		}
	}
	void    ClearSelection() { selection = ellipses.end(); }
	void	SetMode(Mode m)
	{
		mode = m;
		LPWSTR cursor;
		switch (mode)
		{
		case DrawMode:
			cursor = IDC_CROSS;
			break;

		case SelectMode:
			cursor = IDC_HAND;
			break;

		case DragMode:
			cursor = IDC_SIZEALL;
			break;
		}

		hCursor = LoadCursor(NULL, cursor);
		SetCursor(hCursor);
	}
	void	InsertEllipse(float x, float y);
	void	DeleteEllipse();
	BOOL	HitTest(float x, float y);
	HRESULT CreateAdditionalGraphicsResources();
	void    DiscardGraphicsResources();
	void OnPaintScene();
	void    OnLButtonDown(int pixelX, int pixelY, DWORD flags);
	void    OnLButtonUp();
	void    OnMouseMove(int pixelX, int pixelY, DWORD flags);
public:
	MainWindow() : pBrush(NULL),
		ptMouse(D2D1::Point2F()),
		numColor(0)
	{
		ClearSelection();
		SetMode(DrawMode);
	}
	PCWSTR  ClassName() const { return L"Ellipses Class"; }
	bool HandleAdditionalMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *);
};
D2D1::ColorF COLOR = D2D1::ColorF::Red;
D2D1::ColorF MainWindow::colors[7] = { D2D1::ColorF::Red,D2D1::ColorF::Orange,D2D1::ColorF::Yellow,
D2D1::ColorF::Green,D2D1::ColorF::Cyan,D2D1::ColorF::Blue,D2D1::ColorF::Violet };
void MainWindow::InsertEllipse(float x, float y)
{
	shared_ptr<MyEllipse> pme(new MyEllipse);
	D2D1_POINT_2F pt = { x,y };
	pme->ellipse.point = ptMouse = pt;
	pme->ellipse.radiusX = pme->ellipse.radiusY = 1.0f;
	//pme->color = colors[numColor++]; numColor %= 7;
	pme->color = COLOR;
	ellipses.push_front(pme);
	selection = ellipses.begin();
}
void MainWindow::DeleteEllipse()
{
	ellipses.erase(selection);
	ClearSelection();
}
HRESULT MainWindow::CreateAdditionalGraphicsResources()
{
	const D2D1_COLOR_F color = D2D1::ColorF(1.0f, 1.0f, 0);
	HRESULT hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);
	if (SUCCEEDED(hr))
	{
		CalculateLayout();
	}
	return hr;
}
void MainWindow::DiscardGraphicsResources()
{
	Direct2DWindow::DiscardGraphicsResources();
	SafeRelease(&pBrush);
}
void MainWindow::OnPaintScene()
{
	list<shared_ptr<MyEllipse>>::reverse_iterator riter;
	for (riter = ellipses.rbegin(); riter != ellipses.rend(); riter++)
		(*riter)->Draw(pRenderTarget, pBrush, (Selection() && (*riter == *selection)));
}
void MainWindow::OnLButtonDown(int pixelX, int pixelY, DWORD flags)
{
	const float dipX = DPIScale::PixelsToDipsX(pixelX);
	const float dipY = DPIScale::PixelsToDipsY(pixelY);
	if (mode == DrawMode)
	{
		POINT pt = { pixelX, pixelY };
		if (DragDetect(m_hwnd, pt))
		{
			SetCapture(m_hwnd);
			// Start a new ellipse.
			InsertEllipse(dipX, dipY);
		}
	}
	else
	{
		ClearSelection();
		if (HitTest(dipX, dipY))
		{
			SetCapture(m_hwnd);
			ptMouse = Selection()->ellipse.point;
			ptMouse.x -= dipX;
			ptMouse.y -= dipY;
			SetMode(DragMode);
		}
	}
	InvalidateRect(m_hwnd, NULL, FALSE);
}
void MainWindow::OnMouseMove(int pixelX, int pixelY, DWORD flags)
{
	const float dipX = DPIScale::PixelsToDipsX(pixelX);
	const float dipY = DPIScale::PixelsToDipsY(pixelY);
	if ((flags & MK_LBUTTON) && Selection())
	{
		if (mode == DrawMode)
		{
			// Resize the ellipse.
			const float width = (dipX - ptMouse.x) / 2;
			const float height = (dipY - ptMouse.y) / 2;
			const float x1 = ptMouse.x + width;
			const float y1 = ptMouse.y + height;
			Selection()->ellipse = D2D1::Ellipse(D2D1::Point2F(x1, y1), width, height);
		}
		else if (mode == DragMode)
		{
			// Move the ellipse.
			Selection()->ellipse.point.x = dipX + ptMouse.x;
			Selection()->ellipse.point.y = dipY + ptMouse.y;
		}
		InvalidateRect(m_hwnd, NULL, FALSE);
	}
}
void MainWindow::OnLButtonUp()
{
	if ((mode == DrawMode) && Selection())
	{
		ClearSelection();
		InvalidateRect(m_hwnd, NULL, FALSE);
	}
	else if (mode == DragMode)
	{
		SetMode(SelectMode);
	}
	ReleaseCapture();
}
bool MainWindow::HandleAdditionalMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_DRAW_MODE:
			SetMode(DrawMode);
			return true;
		case ID_SELECT_MODE:
			SetMode(SelectMode);
			return true;
		case ID_TOGGLE_MODE:
			if (mode == DrawMode)
			{
				SetMode(SelectMode);
			}
			else
			{
				SetMode(DrawMode);
			}
			return true;
		case ID_DELETE:
			if (Selection())
			{
				DeleteEllipse();
				InvalidateRect(m_hwnd, NULL, FALSE);
			}
			return true;
		}
		return false;
	case WM_LBUTTONDOWN:
		OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD)wParam);
		return true;
	case WM_LBUTTONUP:
		OnLButtonUp();
		return true;
	case WM_MOUSEMOVE:
		OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD)wParam);
		return true;
	case WM_SETCURSOR:
		if (LOWORD(lParam) == HTCLIENT)
		{
			SetCursor(hCursor);
			*pResult = TRUE;
			return true;
		}
		return false;
	}
	return false;
}
#pragma comment(lib, "d2d1")
float DPIScale::scaleX = 1.0f;
float DPIScale::scaleY = 1.0f;
Direct2DWindow::Direct2DWindow() : pFactory(NULL), pRenderTarget(NULL)
{
}
Direct2DWindow::~Direct2DWindow()
{
}
BOOL MainWindow::HitTest(float x, float y)
{
	list<shared_ptr<MyEllipse>>::iterator iter;
	for (iter = ellipses.begin(); iter != ellipses.end(); iter++)
		if ((*iter)->HitTest(x, y)) { selection = iter; return TRUE; }
	return 0;
}
HRESULT Direct2DWindow::CreateGraphicsResources()
{
	HRESULT hr = S_OK;
	if (pRenderTarget == NULL)
	{
		RECT rc;
		GetClientRect(m_hwnd, &rc);
		D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
		hr = pFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(m_hwnd, size),
			&pRenderTarget);
		if (SUCCEEDED(hr))
		{
			hr = CreateAdditionalGraphicsResources();
		}
	}
	return hr;
}
HRESULT Direct2DWindow::CreateAdditionalGraphicsResources()
{
	return S_OK;
}
void Direct2DWindow::DiscardGraphicsResources()
{
	SafeRelease(&pRenderTarget);
}
void Direct2DWindow::OnPaint()
{
	HRESULT hr = CreateGraphicsResources();
	if (SUCCEEDED(hr))
	{
		PAINTSTRUCT ps;
		BeginPaint(m_hwnd, &ps);
		pRenderTarget->BeginDraw();
		pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));
		OnPaintScene();
		hr = pRenderTarget->EndDraw();
		if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
		{
			DiscardGraphicsResources();
		}
		EndPaint(m_hwnd, &ps);
	}
}
void Direct2DWindow::Resize()
{
	if (pRenderTarget != NULL)
	{
		RECT rc;
		GetClientRect(m_hwnd, &rc);
		D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
		pRenderTarget->Resize(size);
		CalculateLayout();
		InvalidateRect(m_hwnd, NULL, FALSE);
	}
}
MainWindow win;
HMENU hMenu;
HWND hDlg;
static int nCmd;
bool Direct2DWindow::HandleAdditionalMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
	return false;
}
INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			return TRUE;
		case IDOK:
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			return TRUE;
		case IDC_BUTTON1:
			COLOR = D2D1::ColorF::Red;
			return TRUE;	
		case IDC_BUTTON2:
			COLOR = D2D1::ColorF::Gold;
			return TRUE;		
		case IDC_BUTTON3:
			COLOR = D2D1::ColorF::Violet;
			return TRUE;
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hDlg);
		return TRUE;
	case WM_DESTROY:
		return TRUE;
	}
	return FALSE;
}
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
	nCmd = nCmdShow;
	HACCEL hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCEL1));
	win.Register();
	if (!win.Create(L"Ellipses", WS_OVERLAPPEDWINDOW))
	{
		return 0;
	}
	hMenu = CreateMenu();
	AppendMenu(hMenu, MF_ENABLED | MF_STRING, IDM_DIALOG_COLOR, TEXT("&Change color"));
	SetMenu(win.Window(), hMenu);
	hDlg = CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), 0, DialogProc, 0);
	ShowWindow(win.Window(), nCmdShow);
	UpdateWindow(win.Window());
	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(win.Window(), hAccel, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return 0;
}
LRESULT Direct2DWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		if (FAILED(D2D1CreateFactory(
			D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory)))
		{
			return -1;
		}
		DPIScale::Initialize(pFactory);
		return 0;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		switch (wmId)
		{
		case IDM_DIALOG_COLOR:
			ShowWindow(hDlg, nCmd);
			InvalidateRect(win.Window(), 0, 0);
			break;
		default:
			return DefWindowProc(win.Window(), uMsg, wParam, lParam);
		}
	}
	break;
	case WM_DESTROY:
		DiscardGraphicsResources();
		SafeRelease(&pFactory);
		PostQuitMessage(0);
		return 0;
	case WM_PAINT:
		OnPaint();
		return 0;
	case WM_SIZE:
		Resize();
		return 0;
	}
	LRESULT Result = 0;
	if (HandleAdditionalMessage(uMsg, wParam, lParam, &Result)) return Result;
	return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}