#ifndef WINDOW_H
#define WINDOW_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

struct window {
    HDC hdc;
    HWND hwnd;
    HDC memdc;
    int surface_width;
    int surface_height;
    uint32_t *pixels;    
};

struct key {
    bool down;
    bool up;
    bool is_down;
};

#define KEYCOUNT 7

struct key g_keys[KEYCOUNT];
struct key g_mouse_left;

struct key keyboard_key(char c) {
    switch (c) {
    case 'A':
        return g_keys[0];
    case 'D':
        return g_keys[1];
    case 'W':
        return g_keys[2];
    case 'S':
        return g_keys[3];
    case 'K':
        return g_keys[4];
    case 'E':
        return g_keys[5];
    case 'Q':
        return g_keys[6];
    default:
        assert(false);
        return g_keys[0];
    }
}

struct key mouse_left() {
    return g_mouse_left;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN: {
        bool is_repeat = (l & (1 << 30)) != 0;
        if (is_repeat) return 0;
        if (w == 'A') {
            g_keys[0].down = true;
            g_keys[0].is_down = true;
        }
        if (w == 'D') {
            g_keys[1].down = true;
            g_keys[1].is_down = true;
        }
        if (w == 'W') {
            g_keys[2].down = true;
            g_keys[2].is_down = true;
        }
        if (w == 'S') {
            g_keys[3].down = true;
            g_keys[3].is_down = true;
        }
        if (w == 'K') {
            g_keys[4].down = true;
            g_keys[4].is_down = true;
        }
        if (w == 'E') {
            g_keys[5].down = true;
            g_keys[5].is_down = true;
        }
        if (w == 'Q') {
            g_keys[6].down = true;
            g_keys[6].is_down = true;
        }
        if (w == VK_ESCAPE) PostQuitMessage(0);
        return 0;
    }
    case WM_KEYUP: {
        if (w == 'A') {
            g_keys[0].up = true;
            g_keys[0].is_down = false;
        }
        if (w == 'D') {
            g_keys[1].up = true;
            g_keys[1].is_down = false;
        }
        if (w == 'W') {
            g_keys[2].up = true;
            g_keys[2].is_down = false;
        }
        if (w == 'S') {
            g_keys[3].up = true;
            g_keys[3].is_down = false;
        }
        if (w == 'K') {
            g_keys[4].up = true;
            g_keys[4].is_down = false;
        }
        if (w == 'E') {
            g_keys[5].up = true;
            g_keys[5].is_down = false;
        }
        if (w == 'Q') {
            g_keys[6].up = true;
            g_keys[6].is_down = false;
        }
        return 0;
    }
    case WM_LBUTTONDOWN:
        g_mouse_left.down = true;
        g_mouse_left.is_down = true;
        return 0;
    case WM_LBUTTONUP:
        g_mouse_left.up = true;
        g_mouse_left.is_down = false;
        return 0;
    default:
        return DefWindowProc(hwnd, msg, w, l);
    }
}

double window_get_time() {
    static LARGE_INTEGER freq;
    if (freq.QuadPart == 0) {
        QueryPerformanceFrequency(&freq);
    }

    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);

    return (double) t.QuadPart / (double) freq.QuadPart;
}

void window_update_events(bool *quit) {
    for (int i = 0; i < KEYCOUNT; i++) {
        g_keys[i].up = false;
        g_keys[i].down = false;
    }
    g_mouse_left.down = false;
    g_mouse_left.up = false;
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT) {
            *quit = true;
            return;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void window_dim(struct window *w, int *width, int *height) {
    RECT rc;
    GetClientRect(w->hwnd, &rc);
    *width = rc.right - rc.left;
    *height = rc.bottom - rc.top;
}

void window_mouse_coords(struct window *w, int *mx, int *my) {
    POINT p;
    GetCursorPos(&p);
    ScreenToClient(w->hwnd, &p);
    *mx = p.x;
    *my = p.y;
}

void window_sleep(float ms) {
    Sleep((DWORD)(ms));
}

void window_init(struct window *w, int surface_width, int surface_height, int window_width, int window_height) {
    HINSTANCE hInstance = GetModuleHandle(NULL);

    STARTUPINFO si;
    GetStartupInfo(&si);
    int show = SW_SHOWDEFAULT;
    if (si.dwFlags & STARTF_USESHOWWINDOW) {
        show = si.wShowWindow;
    }

    // --- Create DIBSection ---
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = surface_width;
    bmi.bmiHeader.biHeight      = surface_height; // top-down
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC screen = GetDC(NULL);
    HBITMAP bitmap = CreateDIBSection(screen, &bmi, DIB_RGB_COLORS,
                                (void**)&w->pixels, NULL, 0);
    ReleaseDC(NULL, screen);

    HDC memdc = CreateCompatibleDC(NULL);
    SelectObject(memdc, bitmap);


    // --- Window class ---
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = "DIBSectionWindow";
    RegisterClassA(&wc);

    HWND hwnd = CreateWindowA(
        "DIBSectionWindow", "Software Renderer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        window_width, window_height,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, show);
    HDC hdc = GetDC(hwnd);

    w->hdc = hdc;
    w->hwnd = hwnd;
    w->memdc = memdc;
    w->surface_width = surface_width;
    w->surface_height = surface_height;

    for (int i = 0; i < KEYCOUNT; i++) {
        g_keys[i].up = false;
        g_keys[i].down = false;
        g_keys[i].is_down = false;
    }
    g_mouse_left.down = false;
    g_mouse_left.up = false;
    g_mouse_left.is_down = false;
}

void window_blit(struct window *w) {
    // Blit to window
    RECT rc;
    GetClientRect(w->hwnd, &rc);

    StretchBlt(w->hdc,
               0, 0, rc.right, rc.bottom,
               w->memdc,
               0, 0, w->surface_width, w->surface_height,
               SRCCOPY);
}

void window_free(struct window *w) {
    ReleaseDC(w->hwnd, w->hdc);
}

void window_mouse_lock(struct window *w)
{
    RECT rect;
    GetClientRect(w->hwnd, &rect);
    POINT ul = { rect.left, rect.top };
    POINT lr = { rect.right, rect.bottom };

    // Convert client coords → screen coords
    ClientToScreen(w->hwnd, &ul);
    ClientToScreen(w->hwnd, &lr);

    RECT clip = { ul.x, ul.y, lr.x, lr.y };
    ClipCursor(&clip);      // lock cursor to window
    ShowCursor(FALSE);      // hide cursor
}

void window_mouse_unlock(void)
{
    ClipCursor(NULL);       // free cursor
    ShowCursor(TRUE);       // show cursor
}

#endif //WINDOW_H
