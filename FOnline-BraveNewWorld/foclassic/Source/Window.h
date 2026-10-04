#ifndef __WINDOW__
#define __WINDOW__

#include "Shared/PlatformSpecific.h"

#if defined (FOCLASSIC_CLIENT) || defined (FOCLASSIC_MAPPER)

# include "Types.h"

# ifndef FO_EMSCRIPTEN
#  include "FL/Fl_Window.H"

#  ifdef FO_MACOSX
#   define fl_display    glXGetCurrentDisplay()
#   define fl_window     ( (uint)fl_xid( MainWindow ) )
#  endif
# endif

# ifdef FO_EMSCRIPTEN

class FOWindow
{
public:
    FOWindow();
    virtual ~FOWindow() {}

    int handle( int event );

    int x() const { return WindowX; }
    int y() const { return WindowY; }
    int w() const { return WindowW; }
    int h() const { return WindowH; }

    void size( int width, int height )
    {
        WindowW = width;
        WindowH = height;
    }

    void position( int xpos, int ypos )
    {
        WindowX = xpos;
        WindowY = ypos;
    }

    void size_range( int, int ) {}
    void size_range( int, int, int, int ) {}

    void fullscreen() {}
    void fullscreen_off() {}
    void iconize() {}

    bool Focused;
    IntVec KeyboardEvents;
    StrVec KeyboardEventsText;
    IntVec MouseEvents;

private:
    int WindowX;
    int WindowY;
    int WindowW;
    int WindowH;
};

# endif

extern FOWindow* MainWindow; // Initialized and handled in MainClient.cpp / MainMapper.cpp

#endif // FOCLASSIC_CLIENT || FOCLASSIC_MAPPER

extern const char* GetWindowName();

#endif // __WINDOW__