#include "os.hh"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif 
#ifndef NOMINMAX
#define NOMINMAX
#endif 
#include <windows.h>
// wasapi
#include <initguid.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audiopolicy.h>
// xinput
#include <xinput.h>
// opengl
#include "wgl.h"
#include "wglext.h"
#include "renderer.hh"

#define CO_CREATE_INSTANCE(name) HRESULT name(REFCLSID rclsid, LPUNKNOWN *pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv)
typedef CO_CREATE_INSTANCE(CoCreateInstance_);
#define CO_INITIALIZE_EX(name) HRESULT name(LPVOID pvReserved, DWORD dwCoInit)
typedef CO_INITIALIZE_EX(CoInitializeEx_);

#define XINPUT_GET_STATE(_name) DWORD WINAPI _name(DWORD, XINPUT_STATE *)
#define XINPUT_SET_STATE(_name) DWORD WINAPI _name(DWORD, XINPUT_VIBRATION *)
typedef XINPUT_GET_STATE(XInputGetState_);
typedef XINPUT_SET_STATE(XInputSetState_);

struct OS {
    MemoryArena arena;
    
    bool old_fullscreen;
    bool old_vsync;    
    
    Platform platform;
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
    
    CoCreateInstance_ *CoCreateInstance;
    CoInitializeEx_ *CoInitializeEx;
    IMMDeviceEnumerator *sound_device_enum;
    IMMDevice *sound_device;
    IAudioClient *audio_client;
    IAudioRenderClient *audio_render_client;
    REFERENCE_TIME sound_buffer_duration;
    u64 sound_channels;
    u32 sound_buffer_frame_count;
    u64 sound_samples_per_sec;
    u64 sound_latency_frame_count;
    
    XInputGetState_ *XInputGetState;
    XInputSetState_ *XInputSetState;
};
static bool window_close_requested;

static void fill_sound_buffer(OS *os, i16 *samples, u64 samples_to_write) {
    if (!samples_to_write) {
        return;
    }
    
    BYTE *data = 0;
    HRESULT result = os->audio_render_client->GetBuffer(samples_to_write, &data);
    assert(result == S_OK);
    if (data) {
        i16 *dst = (i16 *)data;
        i16 *src = samples;
        for (size_t i = 0; i < samples_to_write; ++i) {
            *dst++ = *src++;
            *dst++ = *src++;
        }
    }
    os->audio_render_client->ReleaseBuffer(samples_to_write, 0);
}

static void set_pixel_format(OS *os, HDC hdc) {
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
			WGL_ALPHA_BITS_ARB,					8,
            WGL_SAMPLES_ARB, 4,
			WGL_SAMPLE_BUFFERS_ARB,				true,
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


static LRESULT WINAPI main_window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
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

void go_fullscreen(OS *os, bool fullscreen) {
    static WINDOWPLACEMENT LastWindowPlacement = {sizeof(WINDOWPLACEMENT)};
    
    DWORD WindowStyle = GetWindowLong(os->hwnd, GWL_STYLE);
    // Set fullscreen (actually this is windowed fullscreen!)
    if (fullscreen) {
        MONITORINFO MonitorInfo = {sizeof(MONITORINFO)};
        if (GetWindowPlacement(os->hwnd, &LastWindowPlacement) &&
            GetMonitorInfo(MonitorFromWindow(os->hwnd, MONITOR_DEFAULTTOPRIMARY),
                           &MonitorInfo)) {
            SetWindowLong(os->hwnd, GWL_STYLE, WindowStyle & ~WS_OVERLAPPEDWINDOW);
            
            SetWindowPos(os->hwnd, HWND_TOP,
                         MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.top,
                         MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    } else {
        SetWindowLong(os->hwnd, GWL_STYLE, WindowStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(os->hwnd, &LastWindowPlacement);
        SetWindowPos(os->hwnd, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 -4
#endif 

#ifndef PROCESS_PER_MONITOR_DPI_AWARE 
#define PROCESS_PER_MONITOR_DPI_AWARE 2
#endif 

#ifndef PROCESS_DPI_AWARENESS
#define PROCESS_DPI_AWARENESS int 
#endif 

#define SET_PROCESS_DPI_AWARENESS_CONTEXT(_name) BOOL _name(DPI_AWARENESS_CONTEXT)
typedef SET_PROCESS_DPI_AWARENESS_CONTEXT(SetProcessDpiAwarenessContext_);
#define SET_PROCESS_DPI_AWARENESS(_name) HRESULT _name(PROCESS_DPI_AWARENESS)
typedef SET_PROCESS_DPI_AWARENESS(SetProcessDpiAwareness_);

static bool set_dpi_awareness() {
    bool result = false;
    
    HMODULE user32 = LoadLibraryA("user32.dll");
    if (user32) {
        SetProcessDpiAwarenessContext_ *SetProcessDpiAwarenessContext = (SetProcessDpiAwarenessContext_ *)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
        if (SetProcessDpiAwarenessContext) {
            result = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        } else {
            outf("WARN: SetProcessDpiAwarenessContext is not found\n");
        }
        FreeLibrary(user32);
    } else {
        outf("WARN: Failed to load user32.dll\n");
    }
    
    if (!result) {
        HMODULE shcore = LoadLibraryA("shcore.dll");
        if (shcore) {
            SetProcessDpiAwareness_ *SetProcessDpiAwareness = (SetProcessDpiAwareness_ *)GetProcAddress(shcore, "SetProcessDpiAwareness");
            if (SetProcessDpiAwareness) {
                result = (SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE) == 0);
            } else {
                outf("WARN: Failed to load SetProcessDpiAwareness\n");
            }
            FreeLibrary(shcore);
        } else {
            outf("WARN: Failed to load shcore.dll\n");
        }
    }
    
    if (!result) {
        result = SetProcessDPIAware();
    }
    
    if (!result) {
        outf("Failed to set DPI awareness\n");
    }
    return result;
}

static void check_for_sse() {
    int regs[4];
    __cpuidex(regs, 1, 0);
#define SSE4_1_BIT (1 << 19)
#define SSE4_2_BIT (1 << 20)
    u32 ecx = regs[2];
    
    bool has_sse4 = (bool)(ecx & SSE4_1_BIT) && (bool)(ecx & SSE4_2_BIT);
    if (!has_sse4) {
        outf("ERROR: SSE4 processor extensions not found\n");
    }
}

OS *os_init(Vec2 *display_size) {
    OS *os = bootstrap_alloc_struct(OS, arena);
    
    check_for_sse();
    
    // Set working directory
    char executable_directory[MAX_PATH];
    GetModuleFileNameA(0, executable_directory, sizeof(executable_directory));
    char *last_slash_location = 0;
    char *cursor = executable_directory;
    while (*cursor) {
        if (*cursor == '\\') {
            last_slash_location = cursor;
        }
        ++cursor;
    }
    memset(last_slash_location, 0, sizeof(executable_directory) - (last_slash_location - executable_directory));
    SetCurrentDirectoryA(executable_directory);
    
    //
    // Create window
    //
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
                               WS_CAPTION | WS_SYSMENU | WS_VISIBLE, 
                               CW_USEDEFAULT, CW_USEDEFAULT,
                               WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0, os->instance, 0);
    ShowWindow(os->hwnd, SW_SHOW);
    UpdateWindow(os->hwnd);
    SetForegroundWindow(os->hwnd);
    SetFocus(os->hwnd);
    
    // go_fullscreen(os, true);
    // os->platform.fullscreen = true;
    
    RECT wr;
    GetClientRect(os->hwnd, &wr);
    *display_size = Vec2((f32)(wr.right - wr.left), (f32)(wr.bottom - wr.top));
    
    LARGE_INTEGER pf;
    QueryPerformanceFrequency(&pf);
    os->perf_count_frequency = pf.QuadPart;
    
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    os->last_frame_time = os->game_start_time = time;
    
    set_dpi_awareness();
    //
    // Opengl
    //
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
if (!_name) assert(!"Failed to load " #_name " OGL procedure.");
#include "gl_procs.inc"
#undef GLPROC
    
#define DEFAULT_VSYNC true 
    os->platform.vsync = DEFAULT_VSYNC;
    os->wglSwapIntervalEXT(DEFAULT_VSYNC);
    // Sound
    os->sound_channels = 2;
    os->sound_samples_per_sec = 44100;
    os->sound_latency_frame_count = os->sound_samples_per_sec / 15;
    os->platform.sound_samples = alloc_arr(&os->arena, os->sound_samples_per_sec * 2, i16);
    HRESULT result;
    
    HMODULE wasapi_dll = LoadLibraryA("ole32.dll");
    assert(wasapi_dll);
    os->CoCreateInstance = (CoCreateInstance_ *)GetProcAddress(wasapi_dll, "CoCreateInstance");
    os->CoInitializeEx = (CoInitializeEx_ *)GetProcAddress(wasapi_dll, "CoInitializeEx");
    assert(os->CoCreateInstance);
    assert(os->CoInitializeEx);
    
    result = os->CoInitializeEx(0, COINIT_SPEED_OVER_MEMORY);
    assert(result == S_OK);
#define REFTIMES_PER_SEC 10000000
    REFERENCE_TIME requested_sound_duration = REFTIMES_PER_SEC * 2;
    static GUID CLSID_MMDeviceEnumerator = {0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E};
    static GUID IID_IMMDeviceEnumerator = {0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6};
    result = os->CoCreateInstance(CLSID_MMDeviceEnumerator, 0, CLSCTX_ALL, IID_IMMDeviceEnumerator, (LPVOID *)&os->sound_device_enum);
    assert(result == S_OK);
    result = os->sound_device_enum->GetDefaultAudioEndpoint(eRender, eConsole, &os->sound_device);
    assert(result == S_OK);
    static GUID IID_IAudioClient = {0x1CB9AD4C, 0xDBFA, 0x4c32, 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2};
    result = os->sound_device->Activate(IID_IAudioClient, CLSCTX_ALL, 0, (void **)&os->audio_client);
    assert(result == S_OK);
    
    WORD bits_per_sample = sizeof(i16) * 8;
    WORD block_align = (os->sound_channels * bits_per_sample) / 8;
    DWORD average_bytes_per_second = block_align * os->sound_samples_per_sec;
    WAVEFORMATEX wave_format = {};
    wave_format.wFormatTag = WAVE_FORMAT_PCM;
    wave_format.nChannels = os->sound_channels;
    wave_format.nSamplesPerSec = os->sound_samples_per_sec;
    wave_format.nAvgBytesPerSec = average_bytes_per_second;
    wave_format.nBlockAlign = block_align;
    wave_format.wBitsPerSample = bits_per_sample;
    
    result = os->audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED, 
                                          AUDCLNT_STREAMFLAGS_RATEADJUST | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
                                          requested_sound_duration,
                                          0, &wave_format, 0);
    assert(result == S_OK);
    
    static GUID IID_IAudioRenderClient = {0xF294ACFC, 0x3146, 0x4483, 0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2};
    result = os->audio_client->GetService(IID_IAudioRenderClient, (void **)&os->audio_render_client);
    assert(result == S_OK);
    
    result = os->audio_client->GetBufferSize(&os->sound_buffer_frame_count);
    assert(result == S_OK);
    os->sound_buffer_duration = (REFERENCE_TIME)((f64)REFTIMES_PER_SEC * os->sound_buffer_frame_count / os->sound_samples_per_sec);
    result = os->audio_client->Start();
    assert(result == S_OK);
    //
    // XInput
    //
    HMODULE xinput_dll = LoadLibraryA("xinput1_4.dll");
    if (!xinput_dll) {
        xinput_dll = LoadLibraryA("xinput9_1_0.dll");
    }
    if (!xinput_dll) {
        xinput_dll = LoadLibraryA("xinput1_3.dll");
    }
    assert(xinput_dll);
    os->XInputGetState = (XInputGetState_ *)GetProcAddress(xinput_dll, "XInputGetState");
    os->XInputSetState = (XInputSetState_ *)GetProcAddress(xinput_dll, "XInputSetState");
    assert(os->XInputGetState);
    assert(os->XInputSetState);
    // @TODO actual controller stuff
    
    return os;
}

Platform *os_begin_frame(OS *os) {
    TIMED_FUNCTION();
    Platform *input = &os->platform;
    input->mwheel = 0;
    input->utf32 = 0;
    memset(input->keys_transition_count, 0, sizeof(input->keys_transition_count));
    
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
                    case 0x038: {
                        key = KEY_ALT;
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
                    update_key_state(input, key, is_down);
                }
                
                TranslateMessage(&msg);
            } break;
        }
    }
    
    update_key_state(input, KEY_MOUSE_LEFT,  GetKeyState(VK_LBUTTON) & (1 << 31));
    update_key_state(input, KEY_MOUSE_RIGHT, GetKeyState(VK_RBUTTON) & (1 << 31));
    
    POINT mp;
    GetCursorPos(&mp);
    ScreenToClient(os->hwnd, &mp);
    Vec2 mouse = Vec2((f32)mp.x, (f32)mp.y);
    
    input->mdelta = mouse - input->mpos;
    input->mpos = mouse;
    
    RECT wr;
    GetClientRect(os->hwnd, &wr);
    Vec2 display_size = Vec2((f32)(wr.right - wr.left), (f32)(wr.bottom - wr.top));
    input->window_size_changed = display_size != input->display_size;
    input->display_size = display_size;
    
    LARGE_INTEGER current_time;
    QueryPerformanceCounter(&current_time);
    f32 delta_time = (f32)(current_time.QuadPart - os->last_frame_time.QuadPart) / (f32)os->perf_count_frequency;
    os->last_frame_time = current_time;
    input->frame_dt = delta_time;
    
    input->is_quit_requested = window_close_requested;
    
    u64 sound_sample_count_to_output = 0;
    u32 sound_padding_size;
    if (SUCCEEDED(os->audio_client->GetCurrentPadding(&sound_padding_size))) {
        sound_sample_count_to_output = (u64)(os->sound_latency_frame_count - sound_padding_size);
        if (sound_sample_count_to_output > os->sound_latency_frame_count) {
            sound_sample_count_to_output = os->sound_latency_frame_count;
        }
    }
    memset(input->sound_samples, 0, os->sound_buffer_frame_count * sizeof(i16));
    input->sample_count_to_output = sound_sample_count_to_output;
    input->samples_per_second = os->sound_samples_per_sec;
    
    os->old_vsync = input->vsync;
    os->old_fullscreen = input->fullscreen;
    
    return input;
}

void os_end_frame(OS *os) {
    TIMED_FUNCTION();
    fill_sound_buffer(os, os->platform.sound_samples, os->platform.sample_count_to_output);
    
    if (os->old_vsync != os->platform.vsync) {
        os->wglSwapIntervalEXT(os->platform.vsync);
    }
    os->wglSwapLayerBuffers(os->wglGetCurrentDC(), WGL_SWAP_MAIN_PLANE);
    
    if (os->old_fullscreen != os->platform.fullscreen) {
        go_fullscreen(os, os->platform.fullscreen);
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
    result.no_errors = (handle != INVALID_HANDLE_VALUE);
    memcpy(result.storage, &handle, sizeof(handle));
    return result;
}

bool file_handle_valid(FileHandle handle) {
    return handle.no_errors;
}

size_t get_file_size(FileHandle handle) {
    DWORD result = GetFileSize(*((HANDLE *)handle.storage), 0);
    return (size_t)result;
}

void read_file(FileHandle handle, size_t offset, size_t size, void *dest) {
    if (handle.no_errors) {
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
            INVALID_CODE_PATH;
        }
    }
}

void write_file(FileHandle handle, size_t offset, size_t size, const void *source) {
    if (handle.no_errors) {
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
            INVALID_CODE_PATH;
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

size_t outf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[4096];
    size_t len = vsnprintf(buffer, sizeof(buffer), format, args);
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), buffer, len, 0, 0);
    va_end(args);
    return len;
}