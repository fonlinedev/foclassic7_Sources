# PortAudio Win32/x86 Static Library — Migration & Troubleshooting Record

## Purpose

This document records the working procedure used to rebuild PortAudio for the FOClassic / BraveNewWorld client as a **32-bit (Win32/x86), static library using the MSVC static CRT (`/MT`)**, and then install that exact library where `ClientDX.exe` actually links it.

The final result was successful: the corrected `portaudio.lib` used `LIBCMT`, and `ClientDX.exe` subsequently linked successfully.

---

## 1. Build Environment

### PortAudio source

```text
root\BraveNewWorld\portlibaudio\portaudio
```

### PortAudio build directory

Examples used during the work:

```text
root\BraveNewWorld\portlibaudio\portaudio\build-x86
root\BraveNewWorld\portlibaudio\portaudio\build-test
```

### FOClassic source tree

```text
root\BraveNewWorld\FOnline-BraveNewWorld\foclassic
```

### PortAudio library actually consumed by FOClassic

```text
root\BraveNewWorld\FOnline-BraveNewWorld\foclassic\Source\Libs\portaudio.bin\portaudio.lib
```

### Toolchain

The successful build used MSVC/Ninja/CMake and an x86 target.

Compiler:

```text
C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.51.36231\bin\Hostx86\x86\cl.exe
```

Linker:

```text
C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.51.36231\bin\Hostx86\x86\link.exe
```

Final FOClassic configuration:

```text
Win32-Release
```

---

# 2. The Original Problem

The PortAudio library was originally being built/consumed with a mismatch between MSVC runtime models.

The important runtime options are:

```text
/MD = dynamic MSVC CRT
/MT = static MSVC CRT
```

The FOClassic build was using `/MT`, while the problematic PortAudio library was pulling in `MSVCRT`.

The resulting ClientDX link failed with:

```text
LINK : warning LNK4098: defaultlib 'MSVCRT' conflicts with use of other libs
```

and ultimately:

```text
MSVCRT.lib(chandler4gs.obj) :
error LNK2019:
unresolved external symbol __except_handler4_common
```

followed by:

```text
Source\ClientDX.exe : fatal error LNK1120
```

This was the CRT/runtime-library mismatch that needed to be eliminated.

---

# 3. Why Simply Editing CMakeLists.txt Was Not Enough

A major source of confusion was that there were multiple stages:

```text
CMakeLists.txt
    ↓
CMake configuration
    ↓
generated build.ninja
    ↓
cl.exe command line
    ↓
.obj files
    ↓
portaudio.lib
    ↓
FOClassic's copied portaudio.lib
    ↓
ClientDX.rsp
    ↓
link.exe
```

A correct setting in `CMakeLists.txt` does not guarantee that an already-generated build directory has the correct flags.

Likewise, a correct PortAudio build does not automatically mean FOClassic is using that newly built library.

The actual generated files and actual linked `.lib` therefore had to be inspected.

---

# 4. Correct MSVC Runtime Configuration

The PortAudio CMake configuration was changed so MSVC uses the static runtime.

The important CMake property is:

```cmake
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
```

For Debug-aware configurations, the equivalent form is:

```cmake
set(CMAKE_MSVC_RUNTIME_LIBRARY
    "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    CACHE STRING "" FORCE)
```

The CMake configuration also contained explicit replacement of dynamic CRT flags:

```cmake
string(REGEX REPLACE "/MDd" "/MTd" ${flag_var} "${${flag_var}}")
string(REGEX REPLACE "/MD"  "/MT"  ${flag_var} "${${flag_var}}")
```

This was necessary because the generated compiler command, rather than the source setting alone, is what ultimately determines what MSVC receives.

---

# 5. The `/MT /MD` Failure

During the troubleshooting there was an intermediate build containing:

```text
/MT /MD
```

For example:

```text
cl.exe ... /MT /MD /O2 /Ob2 /DNDEBUG ...
```

MSVC reported:

```text
cl : Command line warning D9025 :
overriding '/MT' with '/MD'
```

This is a critical diagnostic.

When both appear:

```text
/MT /MD
```

the later option wins.

Therefore the build was effectively using:

```text
/MD
```

despite `/MT` also appearing on the command line.

The correct final compiler command had to contain:

```text
/MT
```

and no:

```text
/MD
```

---

# 6. Verify the Generated Ninja Build

After fixing the configuration, the generated Ninja file was checked with:

```bat
findstr /S /I /C:"/MT" /C:"/MD" "build-test\build.ninja"
```

The final correct output contained:

```text
FLAGS = /DWIN32 /D_WINDOWS /W3 /MT /O2 /Ob2 /DNDEBUG
```

There were no `/MD` flags in the relevant PortAudio compilation rules.

This proved that the generated Ninja build was finally passing `/MT` to MSVC.

---

# 7. Successful PortAudio Configuration

The successful configuration command was:

```bat
cmake -S "root\BraveNewWorld\portlibaudio\portaudio" -B "root\BraveNewWorld\portlibaudio\portaudio\build-x86" -G Ninja -DCMAKE_BUILD_TYPE=Release -DPA_BUILD_SHARED_LIBS=OFF -DPA_BUILD_TESTS=OFF -DPA_BUILD_EXAMPLES=OFF -DPA_USE_ASIO=OFF -DPA_USE_DS=ON -DPA_USE_WMME=ON -DPA_USE_WASAPI=ON -DPA_USE_WDMKS=OFF
```

Important settings:

```text
Release
STATIC library
ASIO       OFF
DirectSound ON
WMME       ON
WASAPI     ON
WDMKS      OFF
Tests      OFF
Examples   OFF
```

CMake reported:

```text
-- Build using a Portaudio STATIC libary.
```

Some optional dependency checks failed:

```text
CMAKE_HAVE_LIBC_PTHREAD - Failed
pthread_create in pthreads - not found
pthread_create in pthread - not found
Regex/TRE - not found
JACK - not found
```

These messages were not the cause of the final CRT linker failure.

---

# 8. WDMKS Was Kept Disabled

The final configuration explicitly used:

```text
-DPA_USE_WDMKS=OFF
```

An earlier linker problem involved:

```text
_PaWin_WDMKS_QueryFilterMaximumChannelCount
```

with:

```text
error LNK2019: unresolved external symbol
_PaWin_WDMKS_QueryFilterMaximumChannelCount
```

This was a separate problem from the CRT mismatch.

The successful configuration did not enable the WDMKS path.

A source search showed conditional use of:

```c
#ifdef PAWIN_USE_WDMKS_DEVICE_INFO
```

in PortAudio's Windows host API files.

The final build did not include the problematic WDMKS definition.

---

# 9. Clean Rebuild

After the generated build files were confirmed to use `/MT`, PortAudio was rebuilt cleanly:

```bat
cmake --build build-test --clean-first
```

A clean rebuild matters because old `.obj` files can otherwise survive from a previous `/MD` configuration.

The resulting library was:

```text
build-test\portaudio.lib
```

---

# 10. Definitive Library Runtime Verification

The resulting library was inspected directly:

```bat
dumpbin /directives "build-test\portaudio.lib" | findstr /I /C:"MSVCRT" /C:"LIBCMT"
```

The final successful result was:

```text
/DEFAULTLIB:LIBCMT
```

repeated across the library members.

There were no `MSVCRT` entries in the verification output.

This is the critical verification.

### Interpretation

```text
/DEFAULTLIB:LIBCMT
```

means the library members request the static MSVC CRT.

By contrast:

```text
/DEFAULTLIB:MSVCRT
```

indicates the dynamic CRT.

The successful library therefore matched the intended `/MT` runtime model.

---

# 11. Important Discovery: FOClassic Was Using a Different Copy

One of the biggest problems was that the newly built PortAudio library was not automatically the library used by the game.

The FOClassic linker response file was found with:

```bat
dir /S /B "ClientDX.rsp"
```

The relevant file was:

```text
root\BraveNewWorld\FOnline-BraveNewWorld\foclassic\out\build\Win32-Release\CMakeFiles\ClientDX.rsp
```

An earlier attempted path was:

```text
out\build\Win32-Release\Source\CMakeFiles\ClientDX.rsp
```

That path did not exist.

---

# 12. Confirm Which PortAudio Library ClientDX Links

The correct response file was inspected with:

```bat
findstr /I "portaudio" "out\build\Win32-Release\CMakeFiles\ClientDX.rsp"
```

It showed:

```text
Source\Libs\portaudio.bin\portaudio.lib
```

This was extremely important.

The game was not directly linking:

```text
build-test\portaudio.lib
```

It was linking:

```text
Source\Libs\portaudio.bin\portaudio.lib
```

Therefore the newly built library had to be copied into that location.

---

# 13. Copy the Correct Library

From the PortAudio directory:

```bat
copy /Y "build-test\portaudio.lib" "root\BraveNewWorld\FOnline-BraveNewWorld\foclassic\Source\Libs\portaudio.bin\portaudio.lib"
```

This replaced the old library with the newly built `/MT` version.

---

# 14. Verify the Library FOClassic Actually Uses

The destination library was checked directly:

```bat
dumpbin /directives "root\BraveNewWorld\FOnline-BraveNewWorld\foclassic\Source\Libs\portaudio.bin\portaudio.lib" | findstr /I /C:"MSVCRT" /C:"LIBCMT"
```

The destination library reported:

```text
/DEFAULTLIB:LIBCMT
```

and no `MSVCRT`.

This proved that the library actually consumed by ClientDX had been replaced with the corrected static-CRT library.

---

# 15. Final Client Link

The final ClientDX link command used:

```text
/machine:X86
```

and the response file contained:

```text
Source\Libs\portaudio.bin\portaudio.lib
```

After the corrected PortAudio library was installed, the previous CRT linker failure disappeared and:

```text
ClientDX.exe
```

successfully linked.

This is the end-to-end confirmation that the corrected library was actually being consumed by the client.

---

# 16. Before and After

## Before

PortAudio/game link produced:

```text
LINK : warning LNK4098:
defaultlib 'MSVCRT' conflicts with use of other libs
```

and:

```text
MSVCRT.lib(chandler4gs.obj) :
error LNK2019:
unresolved external symbol __except_handler4_common
```

plus:

```text
Source\ClientDX.exe : fatal error LNK1120
```

The build stopped.

## After

PortAudio compiler flags:

```text
/MT
```

PortAudio library directives:

```text
/DEFAULTLIB:LIBCMT
```

No `MSVCRT` directive in the verified PortAudio library.

The corrected library was copied to:

```text
Source\Libs\portaudio.bin\portaudio.lib
```

ClientDX then linked successfully.

---

# 17. The `C4244` Warning

During PortAudio compilation, this warning appeared:

```text
pa_win_ds.c(1804): warning C4244:
'=': conversion from 'double' to 'unsigned long',
possible loss of data
```

This did not stop PortAudio from building.

It was not the cause of the CRT linker failure.

It should therefore be treated separately unless the application later demonstrates a runtime issue associated with that specific conversion.

---

# 18. The Most Important Lesson

There were two different questions:

### Question A

"What runtime is PortAudio being compiled with?"

Check:

```bat
findstr /S /I /C:"/MT" /C:"/MD" "build-test\build.ninja"
```

Desired:

```text
/MT
```

without `/MD`.

### Question B

"What runtime does the actual `.lib` contain?"

Check:

```bat
dumpbin /directives "build-test\portaudio.lib" | findstr /I /C:"MSVCRT" /C:"LIBCMT"
```

Desired:

```text
/DEFAULTLIB:LIBCMT
```

### Question C

"What library does ClientDX actually link?"

Check:

```bat
findstr /I "portaudio" "out\build\Win32-Release\CMakeFiles\ClientDX.rsp"
```

Desired:

```text
Source\Libs\portaudio.bin\portaudio.lib
```

### Question D

"Is that exact library the corrected one?"

Check:

```bat
dumpbin /directives "root\BraveNewWorld\FOnline-BraveNewWorld\foclassic\Source\Libs\portaudio.bin\portaudio.lib" | findstr /I /C:"MSVCRT" /C:"LIBCMT"
```

Desired:

```text
/DEFAULTLIB:LIBCMT
```

All four checks together give a reliable chain from source configuration to final executable.

---

# 19. Known-Good Final Configuration

```text
Target architecture: Win32 / x86
Build type:          Release
Library:             STATIC

MSVC CRT:            /MT
CRT directive:       LIBCMT

DirectSound:         ON
WMME:                ON
WASAPI:               ON
WDMKS:                OFF
ASIO:                 OFF

PortAudio tests:     OFF
PortAudio examples:  OFF
```

Final library:

```text
root\BraveNewWorld\FOnline-BraveNewWorld\foclassic\Source\Libs\portaudio.bin\portaudio.lib
```

Final client:

```text
ClientDX.exe
```

Final outcome:

```text
PortAudio built with /MT
        ↓
portaudio.lib contains LIBCMT
        ↓
correct library copied into FOClassic
        ↓
ClientDX links successfully
```

---

# 20. Repeatable Build Procedure

Use this procedure for future PortAudio rebuilds.

## A. Enter PortAudio

```bat
cd /D "root\BraveNewWorld\portlibaudio\portaudio"
```

## B. Configure

```bat
cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DPA_BUILD_SHARED_LIBS=OFF -DPA_BUILD_TESTS=OFF -DPA_BUILD_EXAMPLES=OFF -DPA_USE_ASIO=OFF -DPA_USE_DS=ON -DPA_USE_WMME=ON -DPA_USE_WASAPI=ON -DPA_USE_WDMKS=OFF
```

## C. Check generated flags

```bat
findstr /S /I /C:"/MT" /C:"/MD" "build-x86\build.ninja"
```

The output must show:

```text
/MT
```

and must not show:

```text
/MD
```

for the PortAudio compile rules.

## D. Clean rebuild

```bat
cmake --build build-x86 --clean-first
```

## E. Verify PortAudio

```bat
dumpbin /directives "build-x86\portaudio.lib" | findstr /I /C:"MSVCRT" /C:"LIBCMT"
```

Expected:

```text
/DEFAULTLIB:LIBCMT
```

No:

```text
/DEFAULTLIB:MSVCRT
```

## F. Copy to FOClassic

```bat
copy /Y "build-x86\portaudio.lib" "root\BraveNewWorld\FOnline-BraveNewWorld\foclassic\Source\Libs\portaudio.bin\portaudio.lib"
```

## G. Verify destination

```bat
dumpbin /directives "root\BraveNewWorld\FOnline-BraveNewWorld\foclassic\Source\Libs\portaudio.bin\portaudio.lib" | findstr /I /C:"MSVCRT" /C:"LIBCMT"
```

Expected:

```text
/DEFAULTLIB:LIBCMT
```

## H. Confirm ClientDX uses it

From the FOClassic root:

```bat
findstr /I "portaudio" "out\build\Win32-Release\CMakeFiles\ClientDX.rsp"
```

Expected:

```text
Source\Libs\portaudio.bin\portaudio.lib
```

## I. Build ClientDX

Run the normal Win32 Release build.

---

# 21. Do Not Repeat These Mistakes

### Do not trust an old build directory

If CMake configuration changes, regenerate or clean the build.

### Do not accept `/MT /MD`

If the compiler command contains:

```text
/MT /MD
```

the later `/MD` wins.

### Do not inspect only the PortAudio source

Inspect:

```text
build.ninja
```

and:

```text
portaudio.lib
```

### Do not assume the game uses the newest build

Find the actual library in:

```text
ClientDX.rsp
```

### Do not copy the library and skip verification

Always verify the destination with:

```text
dumpbin /directives
```

### Do not re-enable WDMKS just because an unrelated audio problem appears

WDMKS was associated with a separate unresolved-symbol problem:

```text
PaWin_WDMKS_QueryFilterMaximumChannelCount
```

Keep the known-good WDMKS setting unless there is a specific reason to change it.

---

# 22. Final Status

The PortAudio migration/build problem documented here was successfully resolved.

The final evidence was:

1. Generated Ninja rules used `/MT`.
2. The rebuilt `portaudio.lib` reported `/DEFAULTLIB:LIBCMT`.
3. The FOClassic copy of `portaudio.lib` reported `/DEFAULTLIB:LIBCMT`.
4. `ClientDX.rsp` confirmed that FOClassic linked that exact library path.
5. `ClientDX.exe` successfully linked.

The key fix was therefore not merely changing a CMake variable. It was the complete chain:

```text
Correct CMake runtime configuration
        ↓
Generated Ninja files use /MT
        ↓
Clean PortAudio rebuild
        ↓
dumpbin confirms LIBCMT
        ↓
Copy the NEW library to the library path used by FOClassic
        ↓
Verify that exact destination library
        ↓
ClientDX successfully links
```

This is the known-good procedure to preserve for future upgrades/rebuilds.
