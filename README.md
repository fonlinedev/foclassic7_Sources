# FOClassic + PReloaded + Brave New World --- VS 2026 Update

This repository contains the FOClassic / PReloaded /
FOnline-BraveNewWorld source tree, with the source updated so that the
FOClassic 7 engine can be generated and built with **Visual Studio
2026**.

The original FOnline-BraveNewWorld project is intended as a development
and sharing layer around FOClassic, providing engine fixes, scripts,
examples, and reusable code for FOnline server development.

This fork/update adds a **VS 2026 build path** to the FOClassic source.
The goal is to preserve the original project while making the C++ engine
buildable with the modern Visual Studio 2026 toolchain.

------------------------------------------------------------------------

## Project background

  ----------------------------------------------------------------------------
                        FOClassic           PReloaded         Brave New World
                                                                  update
  ----------------- ----------------- ---------------------- -----------------
     Description      Based on the     A simplified version    New features,
                     Fallout Online   of Reloaded Season 3,   code fixes, and
                      engine which    intended as an example  improvements to
                       became open        for testing or      the engine and
                       source. The    integrating FOClassic      scripts.
                        most-used         into existing      
                      revision was        repositories.      
                        taken and                            
                       improved by                           
                        Rotators.                            

         Aim          The original    Testing and providing   Further updates
                       project was    an accessible working   and fixes in an
                    intended to help        FOClassic        independent way,
                    existing servers   engine/server/client    with separate
                     with bug fixes          bundle.          tags and usable
                    and patches after                             diffs.
                     FOnline became                          
                      open source.                           

       License            GPL3           See the original        When used
                                         project for its     together with the
                                       applicable license.    other projects,
                                                              their licenses
                                                               take priority
                                                                  (GPL3).
  ----------------------------------------------------------------------------

## FOnline-BraveNewWorld

FOClassic and PReloaded project, mostly intended for FOnline development
tutorials and module/code sharing.

The project was designed as a layer between development and server
operation:

-   Help server owners set up and host servers with minimal coding
    knowledge.
-   Make FOnline SDK development easier to learn.
-   Share reusable code pieces that can be integrated with minimal
    effort.
-   Prefer AngelScript changes where possible because they are generally
    easier to integrate and less risky than engine changes.
-   Allow independent developers to share work without having to
    affiliate with a particular server.

Changes should be cleanly written and documented where possible, in
code, discussions, wiki pages, or issues.

------------------------------------------------------------------------

# Visual Studio 2026 update

## What this repository changes

The main purpose of this update is to take the **FOClassic source from
the BraveNewWorld project** and make it build with **Visual Studio
2026**.

The original README was based around older Visual Studio versions and an
older CMake workflow. This update replaces that build path with:

-   Visual Studio 2026
-   Visual Studio 2026 C++ Win32 toolchain
-   CMake generation using the `Visual Studio 18 2026` generator
-   A small compatibility source file required by the old
    FOClassic/FLTK-era code when compiling with the modern MSVC runtime
-   A dedicated `SDK.VS2026` CMake build directory

The source itself remains under `foclassic/`; `SDK.VS2026` is a
generated build directory and should not contain normal source files
except the explicitly versioned compatibility file described below.

------------------------------------------------------------------------

## Requirements

### Git

Git is required.

Download:

https://git-scm.com/download/

### CMake

CMake is required.

The original project specified CMake 3.13.4 as the minimum. The VS 2026
update was tested with a modern CMake installation.

For the VS 2026 configuration, the command below uses:

``` text
-DCMAKE_POLICY_VERSION_MINIMUM=3.5
```

This is required because the old project contains CMake policy/version
assumptions that are not accepted directly by current CMake versions.

Download:

https://cmake.org/download/

### Visual Studio 2026

Visual Studio 2026 is required for the VS 2026 build.

Install the C++ desktop development workload and the Win32 C++ build
tools required by the project.

------------------------------------------------------------------------

# Getting the source

Clone the repository and initialize its submodules:

``` bat
git clone <repository-url>
cd FOnline-BraveNewWorld
git submodule update --init --recursive
```

If the repository is already cloned:

``` bat
cd /d C:\Games\FOnlineServer\foclassic7_Sources\FOnline-BraveNewWorld
git pull
git submodule update --init --recursive
```

The main CMake project is inside:

``` text
FOnline-BraveNewWorld\foclassic
```

------------------------------------------------------------------------

# VS 2026 build

## 1. Enter the FOClassic source directory

``` bat
cd /d C:\Games\FOnlineServer\foclassic7_Sources\FOnline-BraveNewWorld\foclassic
```

Adjust the path if the repository was cloned somewhere else.

------------------------------------------------------------------------

## 2. Create the VS 2026 build directory

``` bat
mkdir SDK.VS2026
```

If the directory already exists, this command can be skipped.

------------------------------------------------------------------------

## 3. Generate the Visual Studio 2026 solution

Use:

``` bat
cmake -S . -B SDK.VS2026 -G "Visual Studio 18 2026" -A Win32 -DCMAKE_POLICY_VERSION_MINIMUM=3.5
```

This generates the Visual Studio 2026 solution under:

``` text
foclassic\SDK.VS2026
```

The project is generated as **Win32**, matching the architecture
expected by the original FOClassic code and its dependencies.

------------------------------------------------------------------------

# MSVC compatibility source

The old source contains dependencies on MSVC runtime symbols and
functions that are no longer provided in the same form by modern Visual
Studio.

The VS 2026 build therefore uses:

``` text
foclassic\SDK.VS2026\Source\MSVCCompat.cpp
```

This file provides compatibility implementations for the old symbols
used by the source/dependencies.

The file contains:

``` cpp
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <io.h>

extern "C" FILE* __cdecl _iob_func(void)
{
    return __acrt_iob_func(0);
}

extern "C" FILE* __cdecl __iob_func(void)
{
    return __acrt_iob_func(0);
}

extern "C" FILE* __cdecl ___iob_func(void)
{
    return __acrt_iob_func(0);
}

extern "C" int __cdecl compat_vfprintf(FILE* stream, const char* format, va_list args)
{
    return vfprintf(stream, format, args);
}

extern "C" int __cdecl compat_vsnprintf(char* buffer, size_t size, const char* format, va_list args)
{
    return vsnprintf(buffer, size, format, args);
}

int __cdecl fltk_wopen_compat(const wchar_t* filename, int oflag, int pmode)
{
    return _open_osfhandle(
        (intptr_t)CreateFileW(
            filename,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        ),
        oflag
    );
}

#pragma comment(linker, "/alternatename:?_wopen@@YAHPB_WHH@Z=?fltk_wopen_compat@@YAHPB_WHH@Z")
#pragma comment(linker, "/alternatename:__vsnprintf=_vsnprintf")
```

This compatibility source is the only source file that needs to be kept
inside the generated `SDK.VS2026` directory.

------------------------------------------------------------------------

# Building the complete solution

After CMake generation, build all targets with:

``` bat
cmake --build SDK.VS2026 --config Release -- /m:1
```

This builds the entire generated Visual Studio solution.

For a normal parallel build, the shorter form can also be used:

``` bat
cmake --build SDK.VS2026 --config Release
```

`/m:1` is useful when a serialized build is preferred or when diagnosing
build-order/resource issues.

------------------------------------------------------------------------

# Building the client

To build only the DirectX client target:

``` bat
cmake --build SDK.VS2026 --config Release --target ClientDX -- /m:1
```

The resulting executable is generated under:

``` text
SDK.VS2026\Source\Release\ClientDX.exe
```

------------------------------------------------------------------------

# Rebuilding after source changes

Normally, after modifying C++ source:

``` bat
cd /d C:\Games\FOnlineServer\foclassic7_Sources\FOnline-BraveNewWorld\foclassic
cmake --build SDK.VS2026 --config Release -- /m:1
```

There is normally no need to regenerate CMake just because a source file
changed.

If the CMake configuration itself changes, regenerate first:

``` bat
cmake -S . -B SDK.VS2026 -G "Visual Studio 18 2026" -A Win32 -DCMAKE_POLICY_VERSION_MINIMUM=3.5
```

Then build:

``` bat
cmake --build SDK.VS2026 --config Release -- /m:1
```

------------------------------------------------------------------------

# Clean rebuild

To remove the generated VS 2026 build directory and generate it again:

``` bat
cd /d C:\Games\FOnlineServer\foclassic7_Sources\FOnline-BraveNewWorld\foclassic
rmdir /S /Q SDK.VS2026
mkdir SDK.VS2026
cmake -S . -B SDK.VS2026 -G "Visual Studio 18 2026" -A Win32 -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build SDK.VS2026 --config Release -- /m:1
```

Be aware that this removes all generated files in `SDK.VS2026`. The
compatibility source `Source\MSVCCompat.cpp` must therefore be
restored/copied into the generated directory if it is not maintained
there by the repository.

------------------------------------------------------------------------

# CMake build workflow summary

The complete sequence from an existing source checkout is:

``` bat
cd /d C:\Games\FOnlineServer\foclassic7_Sources\FOnline-BraveNewWorld
git submodule update --init --recursive

cd foclassic
mkdir SDK.VS2026

cmake -S . -B SDK.VS2026 -G "Visual Studio 18 2026" -A Win32 -DCMAKE_POLICY_VERSION_MINIMUM=3.5

cmake --build SDK.VS2026 --config Release -- /m:1
```

For a client-only build:

``` bat
cmake --build SDK.VS2026 --config Release --target ClientDX -- /m:1
```

------------------------------------------------------------------------

# Generated files and Git

`SDK.VS2026` is a generated CMake/Visual Studio build directory.

The generated build files should not normally be committed.

The repository `.gitignore` therefore ignores:

``` gitignore
/foclassic/SDK*
```

The VS 2026 compatibility source is intentionally versioned separately:

``` text
foclassic/SDK.VS2026/Source/MSVCCompat.cpp
```

If the file is ignored by the general `SDK*` rule, it can be explicitly
added to Git with:

``` bat
git add -f foclassic/SDK.VS2026/Source/MSVCCompat.cpp
```

After that it is tracked normally.

------------------------------------------------------------------------

# Original automated build

The original FOClassic project also provided an automated CMake build
script:

``` bat
cmake -P Build.cmake
```

The original workflow prepared files, prepared build directories, built
all targets, and prepared a release package.

The historical Windows build process used multiple Visual Studio
generations and generated directories such as `SDK.VS2017.v100` and
`SDK.VS2017`.

The VS 2026 update described in this README uses the dedicated:

``` text
SDK.VS2026
```

build directory and the Visual Studio 2026 generator instead.

------------------------------------------------------------------------

# Project downloads / historical snapshots

The BraveNewWorld project has historically provided snapshots for users
who need complete packaged versions.

Examples from the original project README include:

-   2023-06-01 --- Spearfishing test
-   2024-03-25 --- Car Tracker and Hub Parkinglot
-   2024-05-22 --- Tragic the Garnering Collectible
-   2024-06-03 --- NPC names and quest colors, `~find` command,
    Status/Misc text, and plasma/fire/electric damage effects
-   2026-03-25 --- latest uploaded FOClassic BNW snapshot at the time of
    the original README

The original project also contained separate Jinxed Jack and Hex Throw
Flare snapshots.

These historical downloads are preserved as project history rather than
being required for the VS 2026 source build.

------------------------------------------------------------------------

# License

FOClassic is GPL3.

The original BraveNewWorld README describes the project as providing
updates and patches around FOClassic and PReloaded. When these
components are used together, the upstream GPL3 licensing requirements
apply as described by the original project.

For licensing questions, consult the license files in the repository and
the upstream project documentation.

------------------------------------------------------------------------

# Notes

This VS 2026 update is intended to make the existing FOClassic 7 source
tree buildable with a modern Visual Studio environment without replacing
the underlying FOClassic/BraveNewWorld project structure.

The important build requirements are:

1.  Visual Studio 2026 with Win32 C++ support.

2.  A current CMake installation.

3.  Initialized Git submodules.

4.  The `MSVCCompat.cpp` compatibility source.

5.  CMake generation using:

    ``` bat
    cmake -S . -B SDK.VS2026 -G "Visual Studio 18 2026" -A Win32 -DCMAKE_POLICY_VERSION_MINIMUM=3.5
    ```

6.  Building with:

    ``` bat
    cmake --build SDK.VS2026 --config Release -- /m:1
    ```

This README documents the VS 2026 migration work while retaining the
original FOClassic and FOnline-BraveNewWorld project background.
