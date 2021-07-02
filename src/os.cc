#include "os.hh"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif 
#ifndef NOMINMAX
#define NOMINMAX
#endif 
#include <windows.h>

#include "framework/renderer.hh"
#include "thirdparty/wgl.h"
#include "thirdparty/wglext.h"

struct OS {
    MemoryArena arena;
    
    Input input;
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
set_pixel_format(OS *os, HDC hdc) {
    i32 suggested_pixel_format_index = 0;
    u32 extended_pick = 0;

    if (os->wglChoosePixelFormatARB) {
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
        os->wglChoosePixelFormatARB(hdc, attributes, 0, 1,
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

OS *os_init() {
    logprintln("OS", "Init start");
    OS *os = bootstrap_alloc_struct(OS, arena);
    
    // Create window
    os->instance = GetModuleHandle(0);
    WNDCLASSA wndclss = {};
    wndclss.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wndclss.lpfnWndProc = main_window_proc;
    wndclss.hInstance = os->instance;
    wndclss.lpszClassName = "lpszClassName";
    wndclss.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    wndclss.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    RegisterClassA(&wndclss);
    
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define WINDOW_NAME "GOSUDAR"
    os->hwnd = CreateWindowExA(WS_EX_APPWINDOW, wndclss.lpszClassName, WINDOW_NAME, 
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0, os->instance, 0);
    ShowWindow(os->hwnd, SW_SHOW);
    UpdateWindow(os->hwnd);
    
    LARGE_INTEGER pf;
    QueryPerformanceFrequency(&pf);
    os->perf_count_frequency = pf.QuadPart;
    
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    os->last_frame_time = os->game_start_time = time;
    logprintln("OS", "Init end");
    return os;
}

void init_renderer_backend(OS *os) {
    HMODULE opengl_dll = LoadLibraryA("opengl32.dll");
    assert(opengl_dll);
    
    *(FARPROC *)&os->wglCreateContext = GetProcAddress(opengl_dll, "wglCreateContext");
    *(FARPROC *)&os->wglDeleteContext = GetProcAddress(opengl_dll, "wglDeleteContext");
    *(FARPROC *)&os->wglGetProcAddress = GetProcAddress(opengl_dll, "wglGetProcAddress");
    *(FARPROC *)&os->wglGetCurrentDC = GetProcAddress(opengl_dll, "wglGetCurrentDC");
    *(FARPROC *)&os->wglMakeCurrent = GetProcAddress(opengl_dll, "wglMakeCurrent");
    *(FARPROC *)&os->wglSwapLayerBuffers = GetProcAddress(opengl_dll, "wglSwapLayerBuffers");
    assert(os->wglCreateContext);
    assert(os->wglDeleteContext);
    assert(os->wglGetProcAddress);
    assert(os->wglGetCurrentDC);
    assert(os->wglMakeCurrent);
    assert(os->wglSwapLayerBuffers);
    
    WNDCLASSA temp_wndclss = {};
    temp_wndclss.lpfnWndProc = DefWindowProcA;
    temp_wndclss.hInstance = os->instance;
    temp_wndclss.lpszClassName = "GLLOADER";
    RegisterClassA(&temp_wndclss);
    HWND temp_hwnd = CreateWindowA(temp_wndclss.lpszClassName, "", 0,
                                   CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                   0, 0, os->instance, 0);
    HDC temp_hwnd_dc = GetDC(temp_hwnd);
    set_pixel_format(os, temp_hwnd_dc);
    
    HGLRC temp_gl_rc = os->wglCreateContext(temp_hwnd_dc);
    os->wglMakeCurrent(temp_hwnd_dc, temp_gl_rc);
    
    *(FARPROC *)&os->wglChoosePixelFormatARB = os->wglGetProcAddress("wglChoosePixelFormatARB");
    *(FARPROC *)&os->wglCreateContextAttribsARB = os->wglGetProcAddress("wglCreateContextAttribsARB");
    *(FARPROC *)&os->wglMakeContextCurrentARB = os->wglGetProcAddress("wglMakeContextCurrentARB");
    *(FARPROC *)&os->wglSwapIntervalEXT = os->wglGetProcAddress("wglSwapIntervalEXT");
    *(FARPROC *)&os->wglGetSwapIntervalEXT = os->wglGetProcAddress("wglGetSwapIntervalEXT");
    assert(os->wglChoosePixelFormatARB);
    assert(os->wglCreateContextAttribsARB);
    assert(os->wglMakeContextCurrentARB);
    assert(os->wglSwapIntervalEXT);
    assert(os->wglGetSwapIntervalEXT);
    
    os->wglDeleteContext(temp_gl_rc);
    
    ReleaseDC(temp_hwnd, temp_hwnd_dc);
    DestroyWindow(temp_hwnd);
    
    HDC hwnd_dc = GetDC(os->hwnd);
    set_pixel_format(os, hwnd_dc);
    
    int opengl_attributes[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_FLAGS_ARB, 
        WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | WGL_CONTEXT_DEBUG_BIT_ARB,
	    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
	    0
    };
    HGLRC gl_rc = os->wglCreateContextAttribsARB(hwnd_dc, 0, opengl_attributes);
    os->wglMakeCurrent(hwnd_dc, gl_rc);
    
#define GLPROC(_name, _type)                                                   \
    *(void **)&_name = (void *)os->wglGetProcAddress(#_name);                  \
    if (!_name) *(void **)&_name = (void *)GetProcAddress(opengl_dll, #_name); \
    if (!_name) logprintln("OpenGL", "Failed to load " #_name " OGL procedure.");
#include "framework/gl_procs.inc"
#undef GLPROC
    
    os->wglSwapIntervalEXT(1);
}

Input *update_input(OS *os) {
    Input *input = &os->input;
    input->mwheel = 0;
        
    for (u32 key_index = 0;
        key_index < KEY_COUNT;
        ++key_index) {
        (input->keys + key_index)->transition_count = 0;        
    }
    input->utf32 = 0;
    
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
                u32 input_char = (u32)msg.wParam;
                // Limit to ASCII for now
                if (0x20 <= input_char && input_char <= 0x7F) {
                    input->utf32 = input_char;
                }
            } break;
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP: {
                bool is_down = ((msg.lParam & (1 << 31)) == 0);
                i32  scancode = (HIWORD(msg.lParam) & (KF_EXTENDED | 0xFF));

                if (!scancode) {
                    scancode = MapVirtualKeyW((UINT)msg.wParam, MAPVK_VK_TO_VSC);
                }

                u32 key = KEY_NONE;
                switch (scancode) {
                    case 0x011: {
                        key = KEY_W;
                    } break;
                    case 0x01E: {
                        key = KEY_A;
                    } break;
                    case 0x01F: {
                        key = KEY_S;
                    } break;
                    case 0x020: {
                        key = KEY_D;
                    } break;
                    case 0x02C: {
                        key = KEY_Z;
                    } break;
                    case 0x030: {
                        key = KEY_B;
                    } break;
                    case 0x02D: {
                        key = KEY_X;
                    } break;
                    case 0x02A: {
                        key = KEY_SHIFT;
                    } break;
                    case 0x01D: {
                        key = KEY_CTRL;
                    } break;
                    case 0x039: {
                        key = KEY_SPACE;
                    } break;
                    case 0x01C: {
                        key = KEY_ENTER;
                    } break;
                    case 0x001: {
                        key = KEY_ESCAPE;
                    } break;
                    case 0x00E: {
                        key = KEY_BACKSPACE;
                    } break;
                    case 0x153: {
                        key = KEY_DELETE;
                    } break;
                    case 0x147: {
                        key = KEY_HOME;
                    } break;
                    case 0x14F: {
                        key = KEY_END;
                    } break;
                    case 0x14B: {
                        key = KEY_LEFT;
                    } break;
                    case 0x14D: {
                        key = KEY_RIGHT;
                    } break;
                    case 0x03B: {
                        key = KEY_F1;
                    } break;
                    case 0x03C: {
                        key = KEY_F2;
                    } break;
                    case 0x03D: {
                        key = KEY_F3;
                    } break;
                    case 0x03E: {
                        key = KEY_F4;
                    } break;
                    case 0x03F: {
                        key = KEY_F5;
                    } break;
                    case 0x040: {
                        key = KEY_F6;
                    } break;
                    case 0x041: {
                        key = KEY_F7;
                    } break;
                    case 0x042: {
                        key = KEY_F8;
                    } break;
                    case 0x043: {
                        key = KEY_F9;
                    } break;
                    case 0x044: {
                        key = KEY_F10;
                    } break;
                    case 0x057: {
                        key = KEY_F11;
                    } break;
                    case 0x058: {
                        key = KEY_F12;
                    } break;
                }

                if ((u32)key) {
                    input->keys[(u32)key].update(is_down);
                }
                
                TranslateMessage(&msg);
            } break;
        }
    }
    
    input->keys[KEY_MOUSE_LEFT].update(GetKeyState(VK_LBUTTON) & (1 << 31));
    input->keys[KEY_MOUSE_RIGHT].update(GetKeyState(VK_RBUTTON) & (1 << 31));
    
    POINT mp;
    GetCursorPos(&mp);
    ScreenToClient(os->hwnd, &mp);
    Vec2 mouse = Vec2((f32)mp.x, (f32)mp.y);
    
    input->mdelta = mouse - input->mpos;
    input->mpos = mouse;
    
    RECT wr;
    GetClientRect(os->hwnd, &wr);
    input->winsize = Vec2((f32)(wr.right - wr.left), (f32)(wr.bottom - wr.top));
    
    LARGE_INTEGER current_time;
    QueryPerformanceCounter(&current_time);
    f32 delta_time = (f32)(current_time.QuadPart - os->last_frame_time.QuadPart) / (f32)os->perf_count_frequency;
    os->last_frame_time = current_time;
    input->dt = delta_time;
    
    input->is_quit_requested = window_close_requested;
    return input;
}

void update_window(OS *os) {
    TIMED_FUNCTION();
    os->wglSwapLayerBuffers(os->wglGetCurrentDC(), WGL_SWAP_MAIN_PLANE);
}

void go_fullscreen(OS *os, bool fullscreen) {
    static WINDOWPLACEMENT LastWindowPlacement = {sizeof(WINDOWPLACEMENT)};

    DWORD WindowStyle = GetWindowLong(os->hwnd, GWL_STYLE);
    // Set fullscreen (actually this is windowed fullscreen!)
    if (fullscreen)
    {
        MONITORINFO MonitorInfo = {sizeof(MONITORINFO)};
        if (GetWindowPlacement(os->hwnd, &LastWindowPlacement) &&
            GetMonitorInfo(MonitorFromWindow(os->hwnd, MONITOR_DEFAULTTOPRIMARY),
                           &MonitorInfo))
        {
            SetWindowLong(os->hwnd, GWL_STYLE, WindowStyle & ~WS_OVERLAPPEDWINDOW);

            SetWindowPos(os->hwnd, HWND_TOP,
                         MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.top,
                         MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(os->hwnd, GWL_STYLE, WindowStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(os->hwnd, &LastWindowPlacement);
        SetWindowPos(os->hwnd, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

RealWorldTime get_real_world_time() {
    SYSTEMTIME time;
    GetLocalTime(&time);
    RealWorldTime result;
    result.year = time.wYear;
    result.month = time.wMonth;
    result.day = time.wDay;
    result.hour = time.wHour;
    result.minute = time.wMinute;
    result.second = time.wSecond;
    result.millisecond = time.wMilliseconds;
    return result;
}


void mkdir(const char *name) {
    HRESULT result = CreateDirectoryA(name, 0);
    UNREFERENCED_VARIABLE(result);
    // assert(result);
    // @TODO errors
}

FileHandle open_file(const char *name, bool read) {
    FileHandle result = {};
    CT_ASSERT(sizeof(result.storage) >= sizeof(HANDLE));
    HANDLE handle;
    if (read) {
        handle = CreateFileA(name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    } else {
        handle = CreateFileA(name, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0, 0);
    }
    result.errors = (handle == INVALID_HANDLE_VALUE);
    memcpy(result.storage, &handle, sizeof(handle));
    return result;
}

bool file_handle_valid(FileHandle handle) {
    return !handle.errors;
}

size_t get_file_size(FileHandle handle) {
    DWORD result = GetFileSize(*((HANDLE *)handle.storage), 0);
    return (size_t)result;
}

void read_file(FileHandle handle, size_t offset, size_t size, void *dest) {
    assert(dest);
    if (!handle.errors)
    {
        HANDLE win32handle = *((HANDLE *)handle.storage);
        OVERLAPPED overlapped = {};
        overlapped.Offset     = (u32)((offset >> 0)  & 0xFFFFFFFF);
        overlapped.OffsetHigh = (u32)((offset >> 32) & 0xFFFFFFFF);
        
        u32 size32 = (u32)size;
        assert(size32 == size);
        
        DWORD bytes_read;
        if (ReadFile(win32handle, dest, size32, &bytes_read, &overlapped) &&
            (size32 == bytes_read))
        {
            // Success        
        }
        else 
        {
            assert(false);
        }
    }
}

void write_file(FileHandle handle, size_t offset, size_t size, const void *source) {
    assert(source);
    if (!handle.errors) {
        HANDLE win32handle = *((HANDLE *)handle.storage);
        OVERLAPPED overlapped = {};
        overlapped.Offset     = (u32)((offset >> 0)  & 0xFFFFFFFF);
        overlapped.OffsetHigh = (u32)((offset >> 32) & 0xFFFFFFFF);
        
        u32 size32 = (u32)size;
        assert(size32 == size);
        
        DWORD bytes_wrote;
        if (WriteFile(win32handle, source, size32, &bytes_wrote, &overlapped) &&
            (size32 == bytes_wrote))
        {
            // Success        
        }
        else 
        {
            assert(false);
        }
    }    
}

void close_file(FileHandle handle) {
    BOOL result = CloseHandle(*((HANDLE *)handle.storage));
    assert(result);
}

void sleep(u32 ms) {
    Sleep(ms);
}

void *os_alloc(size_t size) {
    void *result = VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);    
    assert(result);
    return result;
}

void os_free(void *ptr) {
    if (ptr) {
        VirtualFree(ptr, 0, MEM_RELEASE);
    }
}