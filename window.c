#include <windowsx.h>
#include <windows.h>
#include <stdint.h>
#include <stdio.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define internal static
#define global static
#define persistent static

internal void
Win32ShowErrorMessageBox(const char* Description)
{
    MessageBoxA(NULL,
                Description,
                "Error",
                MB_OK | MB_ICONERROR);
}

internal void
Win32ShowInfoMessageBoxArgs(const char* Title, const char* Format, va_list Args)
{
    char Buffer[1024] = {0};
    vsnprintf(Buffer, sizeof(Buffer), Format, Args);
    MessageBoxA(NULL, Buffer, Title, MB_OK);
}

internal void
Win32ShowInfoMessageBox(const char* Title, const char* Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    Win32ShowInfoMessageBoxArgs(Title, Format, Args);
    va_end(Args);
}

internal void
Win32DebugPrintfArgs(const char* Format, va_list Args)
{
    char Buffer[1024] = {0};
    vsnprintf(Buffer, sizeof(Buffer), Format, Args);
    OutputDebugStringA(Buffer);
}

internal void
Win32DebugPrintf(const char* Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    Win32DebugPrintfArgs(Format, Args);
    va_end(Args);
}

global unsigned WindowRunning;
global int BufferWidth;
global int BufferHeight;
global unsigned char* BufferData;

internal LRESULT CALLBACK
Win32WindowProc(HWND Window,
                UINT Message,
                WPARAM WParam,
                LPARAM LParam)
{
    LRESULT Result = 0;

    switch(Message)
    {
        case WM_DESTROY:
        {
            WindowRunning = 0;
        } break;

        case WM_CLOSE:
        {
            if(!DestroyWindow(Window))
            {
                Win32ShowErrorMessageBox("Failed to destroy window");
            }
        } break;

        case WM_LBUTTONDOWN:
        {
            int X = GET_X_LPARAM(LParam);
            int Y = GET_Y_LPARAM(LParam);
            if(X < 0 ||
               X > BufferWidth ||
               Y < 0 ||
               Y > BufferHeight)
            {
                Win32ShowErrorMessageBox("Invalid X coordinate");
                WindowRunning = 0;
            }

            u32 Color = *(u32*)(BufferData + Y*BufferWidth*4 + X*4);
            Win32ShowInfoMessageBox("Color of point", "(%d,%d) = %08X\n", X, Y, Color);
        } break;

        default:
        {
            Win32DebugPrintf("Unknown message: 0x%p %d %I64d %I64d\n",
                             Window, Message, WParam, LParam);

            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }

    return Result;
}

static void
ShowImage(unsigned char* Data, int Width, int Height, const char* Title)
{
    HMODULE Instance = GetModuleHandleA(0);
    if(!Instance)
    {
        Win32ShowErrorMessageBox("Failed to get module handle");
        return;
    }

    WNDCLASSA WindowsClass = {0};
    WindowsClass.lpfnWndProc = Win32WindowProc;
    WindowsClass.hInstance = Instance;
    WindowsClass.lpszClassName = "ShowImageWindowClass";
    WindowsClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    if(!RegisterClassA(&WindowsClass))
    {
        Win32ShowErrorMessageBox("Failed to register window class");
        return;
    }

    WindowRunning = 1;
    BufferWidth = Width;
    BufferHeight = Height;
    BufferData = Data;

    unsigned WindowStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    HWND Window = CreateWindowEx(0, WindowsClass.lpszClassName, Title,
                                 WindowStyle,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 0, 0, Instance, 0);
    if(!Window)
    {
        Win32ShowErrorMessageBox("Failed to create window");
        return;
    }

    RECT ClientRect;
    ClientRect.left = 0;
    ClientRect.right = Width;
    ClientRect.top = 0;
    ClientRect.bottom = Height;
    if(!AdjustWindowRectEx(&ClientRect, WindowStyle, 0, 0))
    {
        Win32ShowErrorMessageBox("Failed to adjust client rect");
        return;
    }

    if(ClientRect.right < ClientRect.left ||
       ClientRect.bottom < ClientRect.top)
    {
        Win32ShowErrorMessageBox("Invalid adjusted client rect");
        return;
    }

    int WindowWidth = ClientRect.right - ClientRect.left;
    int WindowHeight = ClientRect.bottom - ClientRect.top;
    if(!SetWindowPos(Window, HWND_TOP, 0, 0, WindowWidth, WindowHeight, SWP_NOMOVE))
    {
        Win32ShowErrorMessageBox("Failed to set window pos");
        return;
    }

    HDC DeviceContext = GetDC(Window);
    if(!DeviceContext)
    {
        Win32ShowErrorMessageBox("Failed to get device context");
        return;
    }

    MSG Message = {0};
    BOOL GetMessageResult;
    while((GetMessageResult = GetMessage(&Message, 0, 0, 0)) != 0)
    {
        if(GetMessageResult == -1)
        {
            Win32ShowErrorMessageBox("Failed to get message");
            return;
        }

        TranslateMessage(&Message);
        DispatchMessage(&Message);

        if(!WindowRunning)
        {
            break;
        }

        BITMAPINFO BitmapInfo = {0};
        BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
        BitmapInfo.bmiHeader.biWidth = Width;
        BitmapInfo.bmiHeader.biHeight = -Height;
        BitmapInfo.bmiHeader.biPlanes = 1;
        BitmapInfo.bmiHeader.biBitCount = 32;
        BitmapInfo.bmiHeader.biCompression = BI_RGB;

        int ScanLinesSet = SetDIBitsToDevice(DeviceContext,
                                             0, 0,
                                             Width, Height,
                                             0, 0,
                                             0, Height,
                                             Data, &BitmapInfo,
                                             DIB_RGB_COLORS);
        if(ScanLinesSet == 0)
        {
            Win32ShowErrorMessageBox("Failed to set di bits to device");
            return;
        }
    }

    return;
}
