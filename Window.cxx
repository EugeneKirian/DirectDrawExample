/*
Copyright (c) 2024 Eugene Kirian

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define DIRECTDRAW_VERSION 0x0700
#include <ddraw.h>

// Defines
#define WINDOW_CLASS_NAME L"DirectDrawExample"
#define WINDOW_TITLE_NAME L"DirectDraw Example"

// Window State Properties
int Width = 800;
int Height = 600;
BOOL Execute = TRUE;
BOOL Resize = FALSE;

// Direct Draw Properties
IDirectDraw7* Instance = NULL;
IDirectDrawSurface7* Main = NULL;
IDirectDrawSurface7* Back = NULL;
IDirectDrawClipper* Clipper = NULL;

// Error Codes
#define ERROR_NONE 0
#define ERROR_CREATE_WINDOW_CLASS 1
#define ERROR_CREATE_WINDOW_INSTANCE 2
#define ERROR_CREATE_DIRECT_DRAW_INSTANCE 3
#define ERROR_CREATE_DIRECT_DRAW_COOPERATIVE_LEVEL 4
#define ERROR_CREATE_DIRECT_DRAW_PRIMARY_SURFACE 5
#define ERROR_CREATE_DIRECT_DRAW_BACK_SURFACE 6
#define ERROR_CREATE_DIRECT_DRAW_CLIPPLER 7
#define ERROR_SET_DIRECT_DRAW_SET_CLIPPLER 8
#define ERROR_SET_DIRECT_DRAW_PRIMARY_SURFACE_CLIPPER 9
#define ERROR_RESTORE_PRIMARY_SURFACE 10
#define ERROR_RESTORE_BACK_SURFACE 11
#define ERROR_LOCK_BACK_SURFACE 12
#define ERROR_UNLOCK_BACK_SURFACE 13
#define ERROR_BLIT_PRIMARY_SURFACE 14

// ------------------------------------------------------------------
// Pixels
// ------------------------------------------------------------------

// Draw a grey circle in the middle of the window.
void Draw(unsigned char* pixels, const int pitch)
{
    const int c_x = Width / 2;
    const int c_y = Height / 2;

    const int radius = min(c_x, c_y);
    const int radius2 = radius * radius;
    
    int i = 0;
    
    for (int y = 0; y < Height; y++)
    {
        for (int x = 0; x < Width; x++)
        {
            if ((x - c_x) * (x - c_x) + (y - c_y) * (y - c_y) <= radius2)
            {
                pixels[i++] = 64;
                pixels[i++] = 64;
                pixels[i++] = 64;
                pixels[i++] = 0;
            }
            else
            {
                pixels[i++] = 0;
                pixels[i++] = 0;
                pixels[i++] = 0;
                pixels[i++] = 0;
            }
        }

        i = pitch * (y + 1);
    }
}

// ------------------------------------------------------------------
// Renderer
// ------------------------------------------------------------------

// Recreate back surface when the window size is changed.
// Note, primary surface does not need to be recreated because it has the same dimensions as the screen itself.
int SizeSurface()
{
    Back->Release();

    DDSURFACEDESC2 desc;
    ZeroMemory(&desc, sizeof(DDSURFACEDESC2));

    desc.dwSize = sizeof(DDSURFACEDESC2);

    desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
    desc.dwWidth = Width;
    desc.dwHeight = Height;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

    return Instance->CreateSurface(&desc, &Back, NULL) == DD_OK ? ERROR_NONE : ERROR_CREATE_DIRECT_DRAW_BACK_SURFACE;
}

int Render(HWND hWnd)
{
    // Check if the surfaces need to be restored
    if (Main->IsLost() == DDERR_SURFACELOST) { if (Main->Restore() != DD_OK) { return ERROR_RESTORE_PRIMARY_SURFACE; } }
    if (Back->IsLost() == DDERR_SURFACELOST) { if (Back->Restore() != DD_OK) { return ERROR_RESTORE_BACK_SURFACE; } }

    // Resize back surface to match the window client area.
    if (Resize) { Resize = FALSE; SizeSurface(); }

    DDSURFACEDESC2 desc;
    ZeroMemory(&desc, sizeof(DDSURFACEDESC2));

    desc.dwSize = sizeof(DDSURFACEDESC2);

    desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
    desc.dwHeight = Height;
    desc.dwWidth = Width;

    if (Back->Lock(NULL, &desc, DDLOCK_WAIT | DDLOCK_WRITEONLY, NULL) != DD_OK) { return ERROR_LOCK_BACK_SURFACE; }

    // Update surface
    Draw((unsigned char*)desc.lpSurface, desc.lPitch);

    if (Back->Unlock(NULL) != DD_OK) { return ERROR_UNLOCK_BACK_SURFACE; }

    // Update screen surface.
    RECT src; // Source blit rectangle.
    RECT dst; // Destination blit rectangle.

    POINT p;
    ZeroMemory(&p, sizeof(POINT));

    // Find where on the primary surface our window is.
    ClientToScreen(hWnd, &p);
    GetClientRect(hWnd, &dst);
    OffsetRect(&dst, p.x, p.y);
    SetRect(&src, 0, 0, Width, Height);

    // Copy back surface to the primary surface.
    if (Main->Blt(&dst, Back, &src, DDBLT_WAIT, NULL) != DD_OK) { return ERROR_BLIT_PRIMARY_SURFACE; }

    return ERROR_NONE;
}

// ------------------------------------------------------------------
// Release
// ------------------------------------------------------------------

// Release DirectDraw and window objects.
void Release(HINSTANCE hInstance, HWND hwnd)
{
    if (Clipper != NULL) { Clipper->Release(); }
    if (Back != NULL) { Back->Release(); }
    if (Main != NULL) { Main->Release(); }
    if (Instance != NULL) { Instance->Release(); }

    DestroyWindow(hwnd);
    UnregisterClass(WINDOW_CLASS_NAME, hInstance);
}

// ------------------------------------------------------------------
// Window
// ------------------------------------------------------------------

// Handle Windows messages.
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE: { PostQuitMessage(0); break; }
    case WM_SIZE: { Width = LOWORD(lParam); Height = HIWORD(lParam); Resize = TRUE; break; }
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

// ------------------------------------------------------------------
// Main
// ------------------------------------------------------------------

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    ZeroMemory(&msg, sizeof(MSG));

    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(WNDCLASS));

    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = WINDOW_CLASS_NAME;

    if (!RegisterClass(&wc)) { return ERROR_CREATE_WINDOW_CLASS; }

    HWND hwnd = CreateWindow(
        wc.lpszClassName,
        WINDOW_TITLE_NAME,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, Width, Height, NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) { UnregisterClass(WINDOW_CLASS_NAME, hInstance); return ERROR_CREATE_WINDOW_INSTANCE; }

    if (DirectDrawCreateEx(NULL, (void**)&Instance, IID_IDirectDraw7, NULL) != DD_OK) { Release(hInstance, hwnd); return ERROR_CREATE_DIRECT_DRAW_INSTANCE; }

    if (Instance->SetCooperativeLevel(hwnd, DDSCL_NORMAL) != DD_OK) { Release(hInstance, hwnd); return ERROR_CREATE_DIRECT_DRAW_COOPERATIVE_LEVEL; }

    // Create Primary Surface
    {
        DDSURFACEDESC2 desc;
        ZeroMemory(&desc, sizeof(DDSURFACEDESC2));

        desc.dwSize = sizeof(DDSURFACEDESC2);

        desc.dwFlags = DDSD_CAPS;

        desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

        if (Instance->CreateSurface(&desc, &Main, NULL) != DD_OK) { Release(hInstance, hwnd); return ERROR_CREATE_DIRECT_DRAW_PRIMARY_SURFACE; }
    }

    // Create Back Surface
    {
        DDSURFACEDESC2 desc;
        ZeroMemory(&desc, sizeof(DDSURFACEDESC2));

        desc.dwSize = sizeof(DDSURFACEDESC2);

        desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;

        desc.dwWidth = Width;
        desc.dwHeight = Height;

        desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

        if (Instance->CreateSurface(&desc, &Back, NULL) != DD_OK) { Release(hInstance, hwnd);  return ERROR_CREATE_DIRECT_DRAW_PRIMARY_SURFACE; }
    }

    // Create Clipper
    {
        if (Instance->CreateClipper(0, &Clipper, NULL) != DD_OK) { Release(hInstance, hwnd);  return ERROR_CREATE_DIRECT_DRAW_CLIPPLER; }
        if (Clipper->SetHWnd(0, hwnd) != DD_OK) { Release(hInstance, hwnd); return ERROR_SET_DIRECT_DRAW_SET_CLIPPLER; }
        if (Main->SetClipper(Clipper) != DD_OK) { Release(hInstance, hwnd); return ERROR_SET_DIRECT_DRAW_PRIMARY_SURFACE_CLIPPER; }
    }

    // Render
    while (Execute)
    {
        // Process Windows messages
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT) { Execute = FALSE; break; }
        }

        // Render
        if (Execute && Width != 0 && Height != 0)
        {
            const int result = Render(hwnd);

            if (result != ERROR_NONE) { Release(hInstance, hwnd); return result; }
        }
    }

    // Clean
    Release(hInstance, hwnd);

    return ERROR_NONE;
}