#include "os.hh"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif 
#ifndef NOMINMAX
#define NOMINMAX
#endif 
#include <windows.h>

#include "renderer/renderer.hh"
#include "thirdparty/wgl.h"
#include "thirdparty/wglext.h"

struct OSInternal {
    HINSTANCE instance;
    HWND hwnd;
    
    LARGE_INTEGER last_frame_time;
    LARGE_INTEGER game_start_time;
    LONGLONG perf_count_frequency;
    
    PFNWGLCREATECONTEXTPROC    wglCreateContext;
    PFNWGLDELETECONTEXTPROC    wglDeleteContext;
    PFNWGLGETPROCADDRESSPROC   wglGetProcAddress;
    PFNWGLGETCURRENTDCPROC     wglGetCurrentDC;
    PFNWGLMAKECURRENTPROC      wglMakeCurrent;
    PFNWGLSWAPLAYERBUFFERSPROC wglSwapLayerBuffers;
    
    PFNWGLCHOOSEPIXELFORMATARBPROC    wglChoosePixelFormatARB;
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
	PFNWGLMAKECONTEXTCURRENTARBPROC   wglMakeContextCurrentARB;
	PFNWGLSWAPINTERVALEXTPROC         wglSwapIntervalEXT;
	PFNWGLGETSWAPINTERVALEXTPROC      wglGetSwapIntervalEXT;
};

static bool window_close_requested;

static void 
set_pixel_format(OSInternal *internal, HDC hdc) {
    i32 suggested_pixel_format_index = 0;
    u32 extended_pick = 0;

    if (internal->wglChoosePixelFormatARB) {
        int attributes[] = {
            WGL_DRAW_TO_WINDOW_ARB,				GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB,				GL_TRUE,
			WGL_PIXEL_TYPE_ARB,					WGL_TYPE_RGBA_ARB,
			WGL_ACCELERATION_ARB,				WGL_FULL_ACCELERATION_ARB,
			WGL_DOUBLE_BUFFER_ARB,				GL_TRUE,
			WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB,	GL_TRUE,
			WGL_COLOR_BITS_ARB,					32,
			WGL_DEPTH_BITS_ARB,					24,
			WGL_STENCIL_BITS_ARB,				8,
			WGL_ALPHA_BITS_ARB,					8,
			WGL_SAMPLE_BUFFERS_ARB,				GL_TRUE,
			WGL_SAMPLES_ARB,					4,
			0
        };
        internal->wglChoosePixelFormatARB(hdc, attributes, 0, 1,
									&suggested_pixel_format_index, &extended_pick);
    }

    if (!extended_pick) {
        PIXELFORMATDESCRIPTOR desired_pixel_format = {};
        desired_pixel_format.nSize = sizeof(desired_pixel_format);
        desired_pixel_format.nVersion = 1;
        desired_pixel_format.iPixelType = PFD_TYPE_RGBA;
        desired_pixel_format.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
        desired_pixel_format.cColorBits = 32;
        desired_pixel_format.cAlphaBits = 8;
        desired_pixel_format.cDepthBits = 24;
        desired_pixel_format.iLayerType = PFD_MAIN_PLANE;
        
        suggested_pixel_format_index = ChoosePixelFormat(hdc, &desired_pixel_format);
    }

    PIXELFORMATDESCRIPTOR suggested_pixel_format;
    DescribePixelFormat(hdc, suggested_pixel_format_index,
                        sizeof(suggested_pixel_format), &suggested_pixel_format);
    SetPixelFormat(hdc, suggested_pixel_format_index, &suggested_pixel_format);
}


static LRESULT WINAPI
main_window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    LRESULT result = 0;
    switch (message) {
        case WM_QUIT:
        case WM_DESTROY:
        case WM_CLOSE: {
            window_close_requested = true;
        } break;
        default: {
            result = DefWindowProcA(window, message, wparam, lparam);
        } break;
    }
    
    return result;
}

void OS::init() {
    logprintln("OS", "Init start");
    internal = new OSInternal;
    memset(internal, 0, sizeof(*internal));
    
    // Create window
    internal->instance = GetModuleHandle(0);
    WNDCLASSA wndclss = {};
    wndclss.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wndclss.lpfnWndProc = main_window_proc;
    wndclss.hInstance = internal->instance;
    wndclss.lpszClassName = "WINDOW";
    wndclss.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    wndclss.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    RegisterClassA(&wndclss);
    
    u32 window_width = 1280;
    u32 window_height = 720;
    char *window_name = "HEHE";
    internal->hwnd = CreateWindowExA(WS_EX_APPWINDOW, wndclss.lpszClassName, window_name, 
                                     WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                                     window_width, window_height, 0, 0, internal->instance, 0);
    ShowWindow(internal->hwnd, SW_SHOW);
    UpdateWindow(internal->hwnd);
    logprintln("OS", "Init end");
}

void OS::cleanup() {
    logprintln("OS", "Cleanup");
    delete internal;        
}

void OS::init_renderer_backend() {
    HMODULE opengl_dll = LoadLibraryA("opengl32.dll");
    assert(opengl_dll);
    
    *(FARPROC *)&internal->wglCreateContext = GetProcAddress(opengl_dll, "wglCreateContext");
    *(FARPROC *)&internal->wglDeleteContext = GetProcAddress(opengl_dll, "wglDeleteContext");
    *(FARPROC *)&internal->wglGetProcAddress = GetProcAddress(opengl_dll, "wglGetProcAddress");
    *(FARPROC *)&internal->wglGetCurrentDC = GetProcAddress(opengl_dll, "wglGetCurrentDC");
    *(FARPROC *)&internal->wglMakeCurrent = GetProcAddress(opengl_dll, "wglMakeCurrent");
    *(FARPROC *)&internal->wglSwapLayerBuffers = GetProcAddress(opengl_dll, "wglSwapLayerBuffers");
    
    WNDCLASSA temp_wndclss = {};
    temp_wndclss.lpfnWndProc = DefWindowProcA;
    temp_wndclss.hInstance = internal->instance;
    temp_wndclss.lpszClassName = "GLLOADER";
    RegisterClassA(&temp_wndclss);
    HWND temp_hwnd = CreateWindowA(temp_wndclss.lpszClassName, "", 0,
                                   CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                   0, 0, internal->instance, 0);
    HDC temp_hwnd_dc = GetDC(temp_hwnd);
    set_pixel_format(internal, temp_hwnd_dc);
    
    HGLRC temp_gl_rc = internal->wglCreateContext(temp_hwnd_dc);
    internal->wglMakeCurrent(temp_hwnd_dc, temp_gl_rc);
    
    *(FARPROC *)&internal->wglChoosePixelFormatARB = internal->wglGetProcAddress("wglChoosePixelFormatARB");
    *(FARPROC *)&internal->wglCreateContextAttribsARB = internal->wglGetProcAddress("wglCreateContextAttribsARB");
    *(FARPROC *)&internal->wglMakeContextCurrentARB = internal->wglGetProcAddress("wglMakeContextCurrentARB");
    *(FARPROC *)&internal->wglSwapIntervalEXT = internal->wglGetProcAddress("wglSwapIntervalEXT");
    *(FARPROC *)&internal->wglGetSwapIntervalEXT = internal->wglGetProcAddress("wglGetSwapIntervalEXT");
    
    internal->wglDeleteContext(temp_gl_rc);
    
    ReleaseDC(temp_hwnd, temp_hwnd_dc);
    DestroyWindow(temp_hwnd);
    
    HDC hwnd_dc = GetDC(internal->hwnd);
    set_pixel_format(internal, hwnd_dc);
    
    int opengl_attributes[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_FLAGS_ARB, 
        WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | WGL_CONTEXT_DEBUG_BIT_ARB,
	    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
	    0
    };
    HGLRC gl_rc = internal->wglCreateContextAttribsARB(hwnd_dc, 0, opengl_attributes);
    internal->wglMakeCurrent(hwnd_dc, gl_rc);
    
#define GLPROC(_name, _type)                                        \
    *(void **)&_name = (void *)internal->wglGetProcAddress(#_name);                  \
    if (!_name) *(void **)&_name = (void *)GetProcAddress(opengl_dll, #_name); \
    if (!_name) logprintln("OpenGL", "Failed to load " #_name " OGL procedure.");
#include "renderer/gl_procs.inc"
#undef GLPROC
    
    internal->wglSwapIntervalEXT(1);
}

void OS::prepare_to_start() {
    logprintln("OS", "Prepare to start");
    LARGE_INTEGER pf;
    QueryPerformanceFrequency(&pf);
    internal->perf_count_frequency = pf.QuadPart;
    
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    internal->last_frame_time = internal->game_start_time = time;
}

void OS::update_input(Input *input) {
    input->mwheel = 0;
        
    for (u32 key_index = 0;
        key_index < (u32)Key::Count;
        ++key_index) {
        (input->keys + key_index)->transition_count = 0;        
    }
    
    MSG msg;
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
        switch (msg.message) {
            default: {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            } break;
            case WM_MOUSEWHEEL: {
                i16 wheel_delta = HIWORD(msg.wParam);
                input->mwheel += (f32)wheel_delta / WHEEL_DELTA;
            } break;
            case WM_CHAR: {
                u32 input_char = msg.wParam;
                input->utf32 = input_char;
            } break;
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP: {
                bool is_down = ((msg.lParam & (1 << 31)) == 0);
                i32  scancode = (HIWORD(msg.lParam) & (KF_EXTENDED | 0xFF));

                if (!scancode) {
                    scancode = MapVirtualKeyW(msg.wParam, MAPVK_VK_TO_VSC);
                }

                Key key = Key::None;
                switch (scancode) {
                    case 0x011: {
                        key = Key::W;
                    } break;
                    case 0x01E: {
                        key = Key::A;
                    } break;
                    case 0x01F: {
                        key = Key::S;
                    } break;
                    case 0x020: {
                        key = Key::D;
                    } break;
                    case 0x02A: {
                        key = Key::Shift;
                    } break;
                    case 0x01D: {
                        key = Key::Ctrl;
                    } break;
                    case 0x039: {
                        key = Key::Space;
                    } break;
                    case 0x03B: {
                        key = Key::F1;
                    } break;
                    case 0x03C: {
                        key = Key::F2;
                    } break;
                    case 0x03D: {
                        key = Key::F3;
                    } break;
                    case 0x03E: {
                        key = Key::F4;
                    } break;
                    case 0x03F: {
                        key = Key::F5;
                    } break;
                    case 0x040: {
                        key = Key::F6;
                    } break;
                    case 0x041: {
                        key = Key::F7;
                    } break;
                    case 0x042: {
                        key = Key::F8;
                    } break;
                    case 0x043: {
                        key = Key::F9;
                    } break;
                    case 0x044: {
                        key = Key::F10;
                    } break;
                    case 0x057: {
                        key = Key::F11;
                    } break;
                    case 0x058: {
                        key = Key::F12;
                    } break;
                }

                if ((u32)key) {
                    input->keys[(u32)key].update(is_down);
                }
                
                TranslateMessage(&msg);
            } break;
        }
    }
    
    input->keys[(u32)Key::MouseLeft].update(GetKeyState(VK_LBUTTON) & (1 << 31));
    input->keys[(u32)Key::MouseRight].update(GetKeyState(VK_RBUTTON) & (1 << 31));
    
    POINT mp;
    GetCursorPos(&mp);
    ScreenToClient(internal->hwnd, &mp);
    Vec2 mouse = Vec2(mp.x, mp.y);
    
    input->mdelta = mouse - input->mpos;
    input->mpos = mouse;
    
    RECT wr;
    GetClientRect(internal->hwnd, &wr);
    input->winsize = Vec2(wr.right - wr.left, wr.bottom - wr.top);
    
    LARGE_INTEGER current_time;
    QueryPerformanceCounter(&current_time);
    f32 delta_time = (f32)(current_time.QuadPart - internal->last_frame_time.QuadPart) / (f32)internal->perf_count_frequency;
    internal->last_frame_time = current_time;
    input->dt = delta_time;
    
    f32 time = (f32)(current_time.QuadPart - internal->game_start_time.QuadPart) / (f32)internal->perf_count_frequency;
    input->time = time;
    
    input->is_quit_requested = window_close_requested;
}

void OS::update_window() {
    internal->wglSwapLayerBuffers(internal->wglGetCurrentDC(), WGL_SWAP_MAIN_PLANE);
}

void OS::go_fullscreen(bool fullscreen) {
    static WINDOWPLACEMENT LastWindowPlacement = {sizeof(WINDOWPLACEMENT)};

    DWORD WindowStyle = GetWindowLong(this->internal->hwnd, GWL_STYLE);
    // Set fullscreen (actually this is windowed fullscreen!)
    if (fullscreen)
    {
        MONITORINFO MonitorInfo = {sizeof(MONITORINFO)};
        if (GetWindowPlacement(this->internal->hwnd, &LastWindowPlacement) &&
            GetMonitorInfo(MonitorFromWindow(this->internal->hwnd, MONITOR_DEFAULTTOPRIMARY),
                           &MonitorInfo))
        {
            SetWindowLong(this->internal->hwnd, GWL_STYLE, WindowStyle & ~WS_OVERLAPPEDWINDOW);

            SetWindowPos(this->internal->hwnd, HWND_TOP,
                         MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.top,
                         MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(this->internal->hwnd, GWL_STYLE, WindowStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(this->internal->hwnd, &LastWindowPlacement);
        SetWindowPos(this->internal->hwnd, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}