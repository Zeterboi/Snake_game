#include "ui.h"
#include "../core/core.h"
#include <stdio.h>

static HWND g_hwnd = NULL;
static GameState g_gs = {0};
static HDC g_mem_dc = NULL;
static HBITMAP g_mem_bmp = NULL;
static HBRUSH g_brush_bg = NULL;
static HBRUSH g_brush_snake = NULL;
static HBRUSH g_brush_food = NULL;
static HPEN g_pen_grid = NULL;
static HFONT g_font = NULL;
static int g_client_w = 0;
static int g_client_h = 0;
static double g_scale = 32.0;
static int g_off_x = 0;
static int g_off_y = 0;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int ui_run(void) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = GetModuleHandleA(NULL);
    wc.lpszClassName = "SnakeWindow";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClassA(&wc)) {
        MessageBoxA(NULL, "RegisterClass failed", "Error", MB_ICONERROR);
        return 1;
    }
    const double INIT_SCALE = 32.0;
    g_client_w = (int)(GRID_W * INIT_SCALE);
    g_client_h = (int)(GRID_H * INIT_SCALE + 60);
    g_scale    = INIT_SCALE;
    g_off_x = 0;
    g_off_y = 0;
    g_hwnd = CreateWindowExA(0, "SnakeWindow", "Snake C99/Win32",
                             WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT, g_client_w, g_client_h,
                             NULL, NULL, wc.hInstance, NULL);
    if (!g_hwnd) {
        MessageBoxA(NULL, "CreateWindow failed", "Error", MB_ICONERROR);
        return 1;
    }
    core_init(&g_gs);
    HDC hdc = GetDC(g_hwnd);
    g_mem_dc = CreateCompatibleDC(hdc);
    g_mem_bmp = CreateCompatibleBitmap(hdc, g_client_w, g_client_h);
    SelectObject(g_mem_dc, g_mem_bmp);
    ReleaseDC(g_hwnd, hdc);

    g_brush_bg     = CreateSolidBrush(RGB(10, 40, 10));
    g_brush_snake  = CreateSolidBrush(RGB(128, 0, 0));
    g_brush_food   = CreateSolidBrush(RGB(255, 100, 100));
    g_pen_grid     = CreatePen(PS_SOLID, 1, RGB(60, 60, 60));
    g_font         = CreateFontA(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);
    LARGE_INTEGER freq, last_time;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&last_time);
    const double fixed_step = 0.15;
    double accumulator = 0.0;

    MSG msg;
    while (TRUE) {
        /* Обработка очереди сообщений */
        BOOL has_msg = PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
        if (has_msg) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        double dt = (double)(now.QuadPart - last_time.QuadPart) / freq.QuadPart;
        last_time = now;
        accumulator += dt;

        /* Фиксированный шаг логики */
        while (accumulator >= fixed_step) {
            core_step(&g_gs);
            accumulator -= fixed_step;
            InvalidateRect(g_hwnd, NULL, FALSE);
        }
    }
    DeleteObject(g_mem_bmp);
    DeleteDC(g_mem_dc);
    DeleteObject(g_brush_bg);
    DeleteObject(g_brush_snake);
    DeleteObject(g_brush_food);
    DeleteObject(g_pen_grid);
    DeleteObject(g_font);

    return 0;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_GETMINMAXINFO: {
            MINMAXINFO* mmi = (MINMAXINFO*)lParam;
            mmi->ptMinTrackSize.x = GRID_W * 15;
            mmi->ptMinTrackSize.y = GRID_H * 15 + 60;
            return 0;
        }
        case WM_SIZE: {
            if (wParam == SIZE_MINIMIZED) return 0;
            int new_w = LOWORD(lParam);
            int new_h = HIWORD(lParam);
            if (new_w <= 0 || new_h <= 0) return 0;
            double sx = (double)new_w / GRID_W;
            double sy = (double)new_h / GRID_H;
            g_scale = (sx < sy ? sx : sy);
            
            g_off_x = (int)((new_w - GRID_W * g_scale) / 2.0);
            g_off_y = (int)((new_h - GRID_H * g_scale) / 2.0);

            g_client_w = new_w;
            g_client_h = new_h;

            /* Безопасное пересоздание буфера */
            if (g_mem_bmp) DeleteObject(g_mem_bmp);
            if (g_mem_dc) DeleteDC(g_mem_dc);
            
            HDC hdc = GetDC(g_hwnd);
            g_mem_dc = CreateCompatibleDC(hdc);
            g_mem_bmp = CreateCompatibleBitmap(hdc, g_client_w, g_client_h);
            SelectObject(g_mem_dc, g_mem_bmp);
            ReleaseDC(g_hwnd, hdc);

            InvalidateRect(g_hwnd, NULL, TRUE);
            return 0;
        }
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            if ((lParam & 0x40000000) != 0) return 0;
            switch (wParam) {
                case VK_UP:    core_set_direction(&g_gs, DIR_UP); break;
                case VK_DOWN:  core_set_direction(&g_gs, DIR_DOWN); break;
                case VK_LEFT:  core_set_direction(&g_gs, DIR_LEFT); break;
                case VK_RIGHT: core_set_direction(&g_gs, DIR_RIGHT); break;
                case 'P': case VK_SPACE: core_toggle_pause(&g_gs); break;
                case 'R':
                    if (!core_is_alive(&g_gs)) {
                        core_init(&g_gs);
                        InvalidateRect(hwnd, NULL, TRUE);
                    }
                    break;
            }
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            int cell = (int)g_scale;
            int pad = cell / 8;
            if (pad < 1) pad = 1;

            RECT full_rect = {0, 0, g_client_w, g_client_h};
            FillRect(g_mem_dc, &full_rect, g_brush_bg);

            HPEN old_pen = (HPEN)SelectObject(g_mem_dc, g_pen_grid);
            for (int x = 0; x <= GRID_W; ++x) {
                int px = (int)(x * g_scale + g_off_x);
                MoveToEx(g_mem_dc, px, g_off_y, NULL);
                LineTo(g_mem_dc, px, (int)(GRID_H * g_scale + g_off_y));
            }
            for (int y = 0; y <= GRID_H; ++y) {
                int py = (int)(y * g_scale + g_off_y);
                MoveToEx(g_mem_dc, g_off_x, py, NULL);
                LineTo(g_mem_dc, (int)(GRID_W * g_scale + g_off_x), py);
            }

            HBRUSH old_brush = (HBRUSH)SelectObject(g_mem_dc, g_brush_food);
            int fx = (int)(g_gs.food.x * g_scale + g_off_x);
            int fy = (int)(g_gs.food.y * g_scale + g_off_y);
            Ellipse(g_mem_dc, fx + pad, fy + pad, fx + cell - pad, fy + cell - pad);

            SelectObject(g_mem_dc, g_brush_snake);
            for (int i = 0; i < g_gs.length; ++i) {
                int sx = (int)(g_gs.snake[i].x * g_scale + g_off_x);
                int sy = (int)(g_gs.snake[i].y * g_scale + g_off_y);
                Rectangle(g_mem_dc, sx + pad, sy + pad, sx + cell - pad, sy + cell - pad);
            }

            SetBkMode(g_mem_dc, TRANSPARENT);
            HFONT old_font = (HFONT)SelectObject(g_mem_dc, g_font);
            SetTextColor(g_mem_dc, RGB(255, 255, 255));
            char buf[64];
            int len = snprintf(buf, sizeof(buf), "Score: %d | P:Pause | R:Restart", core_get_score(&g_gs));
            TextOutA(g_mem_dc, 10, g_client_h - 30, buf, len);

            if (!core_is_alive(&g_gs)) {
                HFONT hFontBig = CreateFontA(48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                             DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
                HFONT hOldBig = (HFONT)SelectObject(g_mem_dc, hFontBig);
                SetTextColor(g_mem_dc, RGB(255, 50, 50));
                SIZE sz;
                GetTextExtentPoint32A(g_mem_dc, "GAME OVER", 9, &sz);
                int cx = (g_client_w - sz.cx) / 2;
                int cy = (g_client_h - sz.cy) / 2 - 20;
                TextOutA(g_mem_dc, cx, cy, "GAME OVER", 9);

                SelectObject(g_mem_dc, g_font);
                SetTextColor(g_mem_dc, RGB(200, 200, 200));
                GetTextExtentPoint32A(g_mem_dc, "Press R to Restart", 18, &sz);
                cx = (g_client_w - sz.cx) / 2;
                TextOutA(g_mem_dc, cx, cy + 40, "Press R to Restart", 18);

                SelectObject(g_mem_dc, hOldBig);
                DeleteObject(hFontBig);
            } else if (core_is_paused(&g_gs)) {
                HFONT hFontBig = CreateFontA(48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                             DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
                HFONT hOldBig = (HFONT)SelectObject(g_mem_dc, hFontBig);
                SetTextColor(g_mem_dc, RGB(255, 255, 80));
                SIZE sz;
                GetTextExtentPoint32A(g_mem_dc, "PAUSED", 6, &sz);
                int cx = (g_client_w - sz.cx) / 2;
                int cy = (g_client_h - sz.cy) / 2 - 20;
                TextOutA(g_mem_dc, cx, cy, "PAUSED", 6);

                SelectObject(g_mem_dc, g_font);
                SetTextColor(g_mem_dc, RGB(180, 180, 180));
                GetTextExtentPoint32A(g_mem_dc, "Press P to Resume", 19, &sz);
                cx = (g_client_w - sz.cx) / 2;
                TextOutA(g_mem_dc, cx, cy + 40, "Press P to Resume", 19);

                SelectObject(g_mem_dc, hOldBig);
                DeleteObject(hFontBig);
            }

            BitBlt(hdc, 0, 0, g_client_w, g_client_h, g_mem_dc, 0, 0, SRCCOPY);

            SelectObject(g_mem_dc, old_font);
            SelectObject(g_mem_dc, old_brush);
            SelectObject(g_mem_dc, old_pen);

            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}