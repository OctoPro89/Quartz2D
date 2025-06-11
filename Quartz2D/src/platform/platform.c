#include <platform/platform.h>

#ifdef _WIN64

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <glad/glad.h>

HDC hDC;
HGLRC hRC;
HWND hWnd;
HINSTANCE hInstance;
MSG msg;
LARGE_INTEGER qpc_frequency;
LPCSTR class_name = "Quartz2DWindowClass";
keys keyboard;
u8 running = true;

u8* get_key_state_ptr(keys* k, WPARAM key) {
    switch (key) {
        case VK_UP: return &k->up;
        case VK_DOWN: return &k->down;
        case VK_LEFT: return &k->left;
        case VK_RIGHT: return &k->right;

        case 'W': return &k->w;
        case 'A': return &k->a;
        case 'S': return &k->s;
        case 'D': return &k->d;

        case 'Q': return &k->q;
        case 'E': return &k->e;
        case 'R': return &k->r;
        case 'F': return &k->f;

        case 'Z': return &k->z;
        case 'X': return &k->x;
        case 'C': return &k->c;
        case 'V': return &k->v;

        case '1': return &k->one;
        case '2': return &k->two;
        case '3': return &k->three;
        case '4': return &k->four;
        case '5': return &k->five;
        case '6': return &k->six;
        case '7': return &k->seven;
        case '8': return &k->eight;
        case '9': return &k->nine;
        case '0': return &k->zero;

        case VK_SPACE: return &k->space;
        case VK_RETURN: return &k->enter;
        case VK_SHIFT: return &k->shift;
        case VK_CONTROL: return &k->ctrl;
        case VK_MENU: return &k->alt;

        case VK_ESCAPE: return &k->esc;
        case VK_TAB: return &k->tab;
        case VK_BACK: return &k->backspace;

        default: return NULL;
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CLOSE:
        case WM_DESTROY:
            running = false;
            return 0;

        case WM_KEYDOWN: {
            u8* keyPtr = get_key_state_ptr(&keyboard, wParam);
            if (keyPtr) *keyPtr = true;
            break;
        }

        case WM_KEYUP: {
            u8* keyPtr = get_key_state_ptr(&keyboard, wParam);
            if (keyPtr) *keyPtr = false;
            break;
        }
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

u8 SetupPixelFormat(HDC hdc) {
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, // Flags
        PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or color index.
        32,                   // Color depth
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        24,                   // Depth buffer
        8,                    // Stencil buffer
        0,                    // Auxiliary buffer
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };

    i32 pixelFormat = ChoosePixelFormat(hdc, &pfd);
    if (pixelFormat == 0) return false;

    return SetPixelFormat(hdc, pixelFormat, &pfd);
}

void platform_sleep_ms(u64 ms) {
    Sleep((DWORD)ms);
}

f64 platform_get_elapsed_time_ms() {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return ((f64)(now.QuadPart) * 1000.0) / (f64)(qpc_frequency.QuadPart);
}

void platform_swap_buffers() {
    SwapBuffers(hDC);
}

void platform_shutdown() {
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hRC);
    ReleaseDC(hWnd, hDC);
    DestroyWindow(hWnd);
}

u8 platform_should_run() {
    return running == 1;
}

void platform_pump_messages() {
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int interval);

u8 platform_open_window(i32 width, i32 height) {
    hInstance = GetModuleHandleA(NULL);

    WNDCLASSA wc = { 0 };
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = class_name;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClassA(&wc)) {
        MessageBoxA(NULL, "RegisterClass failed!", "Error", MB_OK);
        return false;
    }

    RECT rect = { 0, 0, width, height };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    // rect now contains the total window size needed
    i32 windowWidth = rect.right - rect.left;
    i32 windowHeight = rect.bottom - rect.top;

    LPCSTR title = "Quartz2D Window";

    hWnd = CreateWindowExA(
        0, class_name, title,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight,
        NULL, NULL, hInstance, NULL
    );

    if (!hWnd) {
        MessageBoxA(NULL, "CreateWindowExA failed!", "Error", MB_OK);
        return false;
    }

    hDC = GetDC(hWnd);

    if (!SetupPixelFormat(hDC)) {
        MessageBoxA(NULL, "Failed to set pixel format.", "Error", MB_OK);
        return false;
    }

    hRC = wglCreateContext(hDC);
    wglMakeCurrent(hDC, hRC);

    // Load GLAD
    if (!gladLoadGL()) {
        MessageBoxA(NULL, "Failed to initialize GLAD", "Error", MB_OK);
        return false;
    }

    QueryPerformanceFrequency(&qpc_frequency);

    return true;
}

u8 platform_set_vsync(u8 sync) {
    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = NULL;
    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

    if (wglSwapIntervalEXT) {
        wglSwapIntervalEXT(sync);
        return true;
    }
    else {
        return false;
    }
}

keys platform_get_keys() {
    return keyboard;
}

#endif // _WIN64