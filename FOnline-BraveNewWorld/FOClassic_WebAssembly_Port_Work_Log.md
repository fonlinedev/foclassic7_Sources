# FOClassic / FOnline Brave New World — WebAssembly Porting Work Log

## 1. Project and objective

This document records the main work completed while porting/building the FOClassic / FOnline Brave New World client as an Emscripten/WebAssembly application.

### Project paths

- Source:
  `C:\Games\FOnlineServer\foclassic7_Sources\FOnline-BraveNewWorld\foclassic`
- Web build:
  `build-web-debug`
- Native deployment:
  `C:\Games\FOnlineServer\foclassic7\Client`
- Web data:
  `webdata\data`
- Native data:
  `C:\Games\FOnlineServer\foclassic7\Client\data`
- Server configuration:
  `C:\Games\FOnlineServer\foclassic7\Server\Server.cfg`
- Server port:
  `2238`
- Browser WebSocket proxy port:
  `4000`

The target client is `ClientGL`.

---

## 2. Native build architecture

The native project uses **Win32/x86**, not x64.

An accidental x64 configuration produced:

```text
Source\Shared\PlatformSpecific.h(63,1):
#error: "X64 CPU not supported for now."
```

The working native Release configuration is:

```powershell
cmake -S . -B build-win-release -G "Visual Studio 18 2026" -A Win32 "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
```

Important PowerShell/CMake detail:

- Quote the complete `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` argument.
- Without the quotes, PowerShell/CMake can split the value and interpret `3.5` incorrectly.

Native build:

```powershell
cmake --build build-win-release --config Release --target ClientGL --verbose
```

---

## 3. Native ClientGL linker fix

Native `ClientGL` initially failed at link time with missing compatibility symbols:

- `_wopen`
- `___iob_func`

The project already had:

```text
SDK.VS2026/Source/MSVCCompat.cpp
```

The problem was that `ClientGL` was not compiling/linking this compatibility source.

The fix was to add:

```text
${CMAKE_CURRENT_LIST_DIR}/../SDK.VS2026/Source/MSVCCompat.cpp
```

to the `ClientGL` source list in:

```text
Source/CMakeLists.txt
```

This was also present for `ClientDX`.

After this change, the native `ClientGL` build succeeded and the native client was confirmed to run correctly with sound and graphics.

**Important baseline:** native ClientGL works. Therefore later web-specific problems should not be treated as generic client/source failures unless evidence points there.

---

## 4. Emscripten/WebAssembly build configuration

The top-level CMake configuration contains the Emscripten-specific link options:

```cmake
if( EMSCRIPTEN )
    target_link_options( ClientGL PRIVATE
        "--use-preload-plugins"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sASSERTIONS=2"
        "-sSTACK_SIZE=1048576"
        "--preload-file"
        "${CMAKE_SOURCE_DIR}/webdata/data@/data"
    )
endif()
```

### Memory growth

The browser initially suffered from WebAssembly memory exhaustion around 48.8 MB.

Adding:

```text
-sALLOW_MEMORY_GROWTH=1
```

resolved the initial fixed-heap limitation.

The later browser logs show WebAssembly heap growth occurring during normal execution.

### Debug information

`-g3` was added to the Emscripten compile/link configuration for detailed debugging.

This was verified with LLVM DWARF inspection.

### WebGL

The web build was moved to WebGL1.

The browser still reports missing WebGL extension warnings such as:

```text
(Not critical)
```

These are not currently considered the main blocker.

There are also messages such as:

```text
WebGL: INVALID_ENUM: texSubImage2D: invalid format
```

These are separate from the current networking/prototype processing problem and have not been treated as the primary cause of the startup failure.

---

## 5. Shader changes

The web build required shader compatibility work.

`2D_Default.glsl` and the remaining appropriate shaders were converted by:

1. Removing:

```glsl
#version 110
```

2. Adding to fragment shaders:

```glsl
precision mediump float;
```

`Flush_RenderTargetMS.glsl` was intentionally left alone because it is associated with desktop multisampling and should not be changed without evidence that it is required.

---

## 6. Browser image/font bridge

A web bridge was added for image/font loading.

`SpriteManager.cpp` under `FO_EMSCRIPTEN` uses Emscripten APIs and includes functions such as:

```cpp
FO_GetPreloadedImageSize
FO_CopyPreloadedImageBGRA
```

`LoadAnimationOther()` under `FO_EMSCRIPTEN` loads predecoded browser canvas pixels through the bridge and uses:

```cpp
FillSurfaceFromMemory
```

The browser preload list contains 18 images, including:

```text
/data/fonts/OldDefault.png
```

The browser confirmed:

```text
FO WEB: OldDefault font OK
```

This indicates that the font preload/bridge path is functioning.

---

## 7. Recursive directory lookup / stack overflow

The POSIX/Emscripten version of `FileFindNext()` originally used a recursive directory implementation.

This caused a `RecursiveDirLook` stack overflow.

The POSIX/Emscripten implementation was changed to a `while(true)` loop.

The Windows implementation was restored to the original Win32 `FindNextFileW` logic.

This change was important because it removed the recursive stack-overflow path without breaking the working native ClientGL build.

---

## 8. WebSocket networking architecture

The server listens on:

```text
2238
```

The browser cannot directly use the TCP server connection, so a WebSocket-to-TCP proxy was installed:

```powershell
py -m pip install websockify
```

and started with:

```powershell
py -m websockify --verbose 4000 localhost:2238
```

### Correct architecture

```text
Chrome
  |
  | WebSocket :4000
  v
websockify :4000
  |
  | TCP :2238
  v
FOnline Server
```

Therefore:

- Browser/client `GameOpt.Port` remains `4000`.
- `Server.exe` continues listening on `2238`.
- Do not change the browser port to `2238`.

Browser logs eventually showed binary network activity, proving that the basic WebSocket connection was working.

---

## 9. AngelScript bytecode problem

The original browser startup reached:

```text
FO WEB: FOEngine->Init() OK
```

but then encountered an AngelScript bytecode failure:

```text
memory access out of bounds
```

The stack included:

```text
asCReader::ReadObjectTypeDeclaration
asCModule::LoadByteCode
Script::LoadScript
FOClient::ReloadScripts
Net_OnMsgData
```

The project uses:

```text
ANGELSCRIPT_VERSION 22701
```

which corresponds to AngelScript 2.27.1.

`Script::LoadScript` checks the bytecode version against:

```cpp
ANGELSCRIPT_VERSION
```

The client bytecode overload creates a `CBytecodeStream`, writes the received bytes, and calls:

```cpp
module->LoadByteCode
```

The server-side client-script compilation path sends the raw `module->SaveByteCode(&binary)` data.

A cached file examined during debugging was:

```text
CLIENT_client_CarTracker.fosb
```

with a size of approximately 59,888 bytes.

The source `.fos` was approximately 15,119 bytes.

The timestamp comparison did not indicate an obviously stale `.fosb` file.

A web-only bypass was considered/used during debugging to skip client script bytecode loading:

```cpp
#ifdef FO_EMSCRIPTEN
    WriteLog( "FO WEB: skipping client script bytecode load\n" );
    return true;
#endif
```

This was a diagnostic workaround, not established as the final correct architecture.

The main lesson is that the AngelScript failure was likely web-runtime/configuration specific, rather than evidence that the native client itself was broken.

---

## 10. Rain / AnyFrames browser crash

A significant browser crash occurred while loading rain animations.

The crash stack included:

```text
AnyFrames::~AnyFrames()
HexManager::SetRainAnimation
HexManager::ReloadSprites
FOClient::InitIface
FOClient::Net_OnMsgData
FOClient::NetProcess
FOClient::ParseSocket
FOClient::MainLoop
```

The relevant `DummyAnimation` code is:

```cpp
AnyFrames* SpriteManager::DummyAnimation = NULL;
```

and it is initialized in `SpriteManager::Init()`:

```cpp
if( !DummyAnimation )
{
    DummyAnimation = CreateAnimation( 1, 100 );
    if( !DummyAnimation )
    {
        WriteLog("Can't create dummy animation.\n");
        return false;
    }
}
```

`SetRainAnimation()` originally used:

```cpp
SAFEDEL( picRainFall );
SAFEDEL( picRainDrop );

picRainFall = SprMngr.LoadAnimation(
    picRainFallName.c_str(),
    PATH_DATA,
    ANIM_USE_DUMMY
);

picRainDrop = SprMngr.LoadAnimation(
    picRainDropName.c_str(),
    PATH_DATA,
    ANIM_USE_DUMMY
);
```

`LoadAnimation()` can return `DummyAnimation` when `ANIM_USE_DUMMY` is specified.

That creates a dangerous situation if `picRainFall` or `picRainDrop` points at the shared `DummyAnimation` and is subsequently deleted.

### Diagnostic fix

A guarded deletion was first tested:

```cpp
if( picRainFall && picRainFall != SpriteManager::DummyAnimation )
    SAFEDEL( picRainFall );
else
    picRainFall = NULL;

if( picRainDrop && picRainDrop != SpriteManager::DummyAnimation )
    SAFEDEL( picRainDrop );
else
    picRainDrop = NULL;
```

A stronger web-only test was then used:

```cpp
void HexManager::SetRainAnimation( const char* fall_anim_name, const char* drop_anim_name )
{
    if( fall_anim_name )
        picRainFallName = fall_anim_name;
    if( drop_anim_name )
        picRainDropName = drop_anim_name;

#ifdef FO_EMSCRIPTEN
    picRainFall = SpriteManager::DummyAnimation;
    picRainDrop = SpriteManager::DummyAnimation;
    return;
#else
    SAFEDEL( picRainFall );
    SAFEDEL( picRainDrop );
    picRainFall = SprMngr.LoadAnimation(
        picRainFallName.c_str(),
        PATH_DATA,
        ANIM_USE_DUMMY
    );
    picRainDrop = SprMngr.LoadAnimation(
        picRainDropName.c_str(),
        PATH_DATA,
        ANIM_USE_DUMMY
    );
#endif
}
```

After rebuilding and copying the web files, the previous rain destructor crash disappeared.

The browser no longer produced the old `AnyFrames::~AnyFrames()` abort stack.

The canvas remained black, so the startup/network investigation continued.

**Do not undo this workaround until the browser path is stable and the ownership issue is properly understood.**

---

## 11. Web output deployment

After a successful web build, the following files were copied into:

```text
C:\Games\FOnlineServer\foclassic7\Client\
```

using:

```powershell
Copy-Item ".\build-web-debug\FOClassic-v7\ClientGL.js",".\build-web-debug\Source\ClientGL.wasm",".\build-web-debug\Source\ClientGL.data" "C:\Games\FOnlineServer\foclassic7\Client\" -Force
```

The deployed files are therefore:

```text
ClientGL.js
ClientGL.wasm
ClientGL.data
```

---

## 12. Cache refresh / network startup discovery

One of the important startup discoveries involved:

```cpp
bool refresh_cache =
    (!Singleplayer &&
     !CommandLine->IsOption( "DefCache" ) &&
     !Crypt.IsCacheTable( cache_fname ));
```

A web-only override had previously been:

```cpp
#ifdef FO_EMSCRIPTEN
    refresh_cache = false;
#endif
```

This prevented the normal cache initialization path from setting:

```cpp
InitNetReason = INIT_NET_REASON_CACHE;
```

The startup code contains:

```cpp
if( refresh_cache )
    InitNetReason = INIT_NET_REASON_CACHE;
```

The main loop later consumes `InitNetReason` and calls `InitNet()`.

The web-specific setting was therefore changed to:

```cpp
#ifdef FO_EMSCRIPTEN
    refresh_cache = true;
#endif
```

This was an important discovery.

After this change, the browser began receiving server network data.

This means the problem was no longer simply “the browser cannot connect.”

---

## 13. Main network startup flow

The relevant `MainLoop()` logic is approximately:

```cpp
if( InitNetReason != INIT_NET_REASON_NONE )
{
    int reason = InitNetReason;
    InitNetReason = INIT_NET_REASON_NONE;

    ShowMainScreen( CLIENT_MAIN_SCREEN_WAIT );

    if( !InitNet() )
    {
        ShowMainScreen( CLIENT_MAIN_SCREEN_LOGIN );
        AddMess(...);
        return 1;
    }

    if( reason == INIT_NET_REASON_CACHE )
        Net_SendLogIn( NULL, NULL );
    else if( reason == INIT_NET_REASON_LOGIN )
        Net_SendLogIn(
            GameOpt.Name.c_str(),
            Password.c_str()
        );
    ...
}

if( IsConnected )
    ParseSocket();

if( !IsConnected &&
    !IsMainScreen( LOGIN/REGISTRATION/CREDITS ) )
{
    ShowMainScreen( LOGIN );
}
```

`InitNet()` establishes the connection and sets:

```cpp
IsConnected = true;
```

`NetConnect()` uses the Emscripten WebSocket wrapper.

The browser-side port remains `4000`.

---

## 14. Browser logs and current status

Two large browser logs were examined.

The later complete log was:

```text
Pasted text(20261004-192440).txt
```

It contains approximately 24,643 lines.

The log confirms:

- Emscripten startup.
- WebGL extension warnings marked as non-critical.
- Font/image bridge activity.
- `OldDefault` font loading successfully.
- WebAssembly heap growth.
- `FOEngine->Init()` completing.
- Network traffic being processed.
- No longer seeing the old rain destructor crash.

The most important section occurs around the later network processing.

The log repeatedly reports a value associated with:

```text
163
```

and then shows heap growth followed by:

```text
$std::__2::vector<ProtoItem>::push_back
$ItemManager::GetCopyAllProtos(...)
$FOClient::Net_OnProtoItemData()
$FOClient::NetProcess()
$FOClient::ParseSocket()
$FOClient::MainLoop()
```

The relevant log lines show repeated `163` activity followed by a heap resize and entry into `Net_OnProtoItemData()`.

This is strong evidence that the current browser execution is reaching the item-prototype network handler rather than failing at the WebSocket connection layer.

---

## 15. Network message dispatch

The message dispatcher contains:

```cpp
case NETMSG_MSG_DATA:
    Net_OnMsgData();
    break;

case NETMSG_ITEM_PROTOS:
    Net_OnProtoItemData();
    break;
```

The default path reports invalid messages and attempts to seek a valid message.

The protocol definitions examined include:

### `NETMSG_MSG_DATA`

Its structure contains fields such as:

```text
uint msg_len
uint MSG_language
uint16 MSG_num
uint MSG_hash
char[...] MSG_data
    uint num
    uint len
    char data[len]
```

### `NETMSG_ITEM_PROTOS`

Its structure contains:

```text
uint msg_len
uint8 item_type
uint item_hash
ProtoItem[...]
```

The exact number of `ProtoItem` entries is derived from the message length.

---

## 16. Current `Net_OnProtoItemData()` implementation

The current source shown during the latest investigation is:

```cpp
void FOClient::Net_OnProtoItemData()
{
    uint         msg_len;
    uint8        type;
    uint         data_hash;
    ProtoItemVec data;

    Bin >> msg_len;
    Bin >> type;
    Bin >> data_hash;

    data.resize(
        (msg_len -
         sizeof(uint) -
         sizeof(msg_len) -
         sizeof(type) -
         sizeof(data_hash)) /
        sizeof(ProtoItem)
    );

    Bin.Pop(
        (char*)&data[0],
        (uint)data.size() * sizeof(ProtoItem)
    );

    CHECK_IN_BUFF_ERROR;

    if( data_hash != Crypt.Crc32(
            (uint8*)&data[0],
            (uint)data.size() * sizeof(ProtoItem) ) )
    {
        WriteLogF( _FUNC_, " - Hash error.\n" );
        return;
    }

    ItemMngr.ClearProtos( type );
    ItemMngr.ParseProtos( data );

    ProtoItemVec proto_items;
    ItemMngr.GetCopyAllProtos( proto_items );

    uint len = (uint)proto_items.size() * sizeof(ProtoItem);

    uint8* proto_data =
        Crypt.Compress(
            (uint8*)&proto_items[0],
            len
        );

    if( !proto_data )
    {
        WriteLogF( _FUNC_, " - Compression fail.\n" );
        return;
    }

    Crypt.SetCache(
        "item_protos",
        proto_data,
        len
    );

    delete[] proto_data;

    uint count = (uint)proto_items.size();

    Crypt.SetCache(
        "item_protos_count",
        (uint8*)&count,
        sizeof(count)
    );

    // Refresh craft names
    MrFixit.GenerateNames( *MsgGame, *MsgItem );
}
```

This function does the following:

1. Reads message length.
2. Reads item type.
3. Reads prototype data hash.
4. Calculates the number of `ProtoItem` structures from `msg_len`.
5. Reads the prototype data from the network buffer.
6. Checks the buffer state.
7. Verifies the CRC32 hash.
8. Clears existing prototypes for the specified type.
9. Parses the received prototypes.
10. Copies all current item prototypes into another vector.
11. Compresses the complete prototype list.
12. Stores it in the cache.
13. Stores the prototype count.
14. Regenerates craft names.

The current investigation should focus on this function and its relationship with repeated `NETMSG_ITEM_PROTOS` messages.

---

## 17. Important protocol/header diagnostic caveat

A debug bridge was added for browser-side network diagnostics.

The bridge has the form:

```cpp
#ifdef FO_EMSCRIPTEN
EM_JS(void, FO_Net_DebugMsg, (int msg), {
    const u = msg >>> 0;
    console.log(
        "[FO NET DEBUG] raw:",
        u,
        "hex:",
        "0x" + u.toString(16).padStart(8, "0"),
        "id:",
        (u >> 8) & 0xFF
    );
});
#endif
```

The browser log showed values such as:

```text
raw: 56 hex: 0x00000038 id: 0
raw: 163 hex: 0x000000a3 id: 0
```

However, this needs caution.

The protocol header macros are:

```cpp
#define CONST_NETMSG_HEADER( number ) \
    ( (uint)( (0xF0C1A51C + number - 1) ) )

#define CALC_NETMSG_HEADER( number ) \
    ( (uint)( (0xF0C1A51C ^ number) * \
    (number ## ull << (number % 16) ) % 0xFFFFFFFF ) )

#define MAKE_NETMSG_HEADER( number ) CALC_NETMSG_HEADER( number )
```

A calculation was performed over a range of possible message numbers and did not show 56 or 163 as full calculated protocol headers.

Therefore the debug output values 56/163 should not automatically be interpreted as the complete 32-bit network header.

The current diagnostic must be checked against the exact source call site before drawing conclusions about the wire header.

---

## 18. Diagnostic instrumentation

Two Emscripten browser debug functions were added around `Net_OnMsgData()`:

```cpp
#ifdef FO_EMSCRIPTEN
EM_JS(void, FO_Net_OnMsgData_Begin, (), {
    console.log("[FO NET DEBUG] Net_OnMsgData BEGIN");
});

EM_JS(void, FO_Net_OnMsgData_End, (), {
    console.log("[FO NET DEBUG] Net_OnMsgData END");
});
#endif
```

They are called at the beginning/end of `Net_OnMsgData()`.

A separate message diagnostic bridge was also used:

```cpp
#ifdef FO_EMSCRIPTEN
EM_JS(void, FO_Net_DebugMsg, (int msg), {
    const u = msg >>> 0;
    console.log(
        "[FO NET DEBUG] raw:",
        u,
        "hex:",
        "0x" + u.toString(16).padStart(8, "0"),
        "id:",
        (u >> 8) & 0xFF
    );
});
#endif
```

The latest complete log did **not** contain a `Net_OnMsgData END` marker, meaning the exact instrumentation/build state needs to be verified before relying on that absence as proof of a function hang.

---

## 19. Current interpretation of the browser state

The current state is substantially further than the original failure:

### Confirmed working

- Native Win32/x86 ClientGL builds.
- Native ClientGL runs.
- Emscripten build produces JS/WASM/data.
- WebAssembly heap can grow.
- Browser image/font preload bridge works.
- OldDefault font is successfully loaded.
- Recursive Emscripten directory lookup no longer overflows.
- WebSocket proxy is functioning.
- Browser receives binary server traffic.
- `FOEngine->Init()` succeeds.
- Previous rain/AnyFrames destructor crash has been eliminated by the current web-only rain workaround.

### Still unresolved

The browser canvas remains black and startup does not reach a usable client UI.

The latest complete log points to repeated item-prototype network processing:

```text
NETMSG_ITEM_PROTOS
    ->
Net_OnProtoItemData
    ->
ItemMngr.GetCopyAllProtos
    ->
vector allocations / heap growth
```

The next investigation step is therefore **not** another general WebSocket or WebGL change.

The next step is to understand:

- why `NETMSG_ITEM_PROTOS` is being received repeatedly,
- whether the message length/type/hash are valid,
- whether `Net_OnProtoItemData()` advances the network buffer correctly,
- whether `ItemMngr.ParseProtos()` / `GetCopyAllProtos()` causes unintended repeated growth,
- and whether the server is intentionally sending multiple item-prototype chunks or repeatedly sending the same packet because of a cache/hash mismatch.

---

## 20. Most recent source inspection

The latest PowerShell command used was:

```powershell
Get-ChildItem ".\Source" -Recurse -File -Include *.cpp,*.h | Select-String -Pattern "void FOClient::Net_OnProtoItemData|Net_OnProtoItemData\(" -Context 5,80 | ForEach-Object { $_.Context.PreContext; $_.Line; $_.Context.PostContext }
```

The resulting source confirmed both the dispatcher:

```cpp
case NETMSG_ITEM_PROTOS:
    Net_OnProtoItemData();
    break;
```

and the complete `Net_OnProtoItemData()` function shown above.

No new source modification should be made solely from the existence of this function. The next change should be based on evidence from its message fields and the server-side sender.

---

## 21. Recommended investigation order

The safest order from this point is:

### Step 1 — inspect the actual message dispatch/header call

Confirm exactly what value is passed to `FO_Net_DebugMsg()` and how `msg` is decoded.

Do not assume that the logged `56` or `163` values are complete wire headers.

### Step 2 — instrument `Net_OnProtoItemData()`

For the browser only, log:

- `msg_len`
- `type`
- `data_hash`
- calculated `data.size()`
- calculated byte count
- whether the CRC check succeeds
- `proto_items.size()`
- compression result

Use `EM_JS` browser bridges rather than unsupported `EM_ASM` usage in `Client.cpp`.

### Step 3 — inspect the server sender

Find the server code that sends:

```text
NETMSG_ITEM_PROTOS
```

Determine:

- when it is sent,
- whether it is sent once or repeatedly,
- what `type` is used,
- how `msg_len` is calculated,
- how the CRC is calculated,
- whether the browser client is expected to request it repeatedly.

### Step 4 — only then change code

Avoid speculative changes to:

- WebGL,
- shaders,
- WebSocket ports,
- protocol constants,
- item prototype parsing,
- cache format,
- or memory settings.

The native client is already working, and the browser is already receiving data. The evidence now points to the item-prototype/network startup stage.

---

## 22. Known commands

### Configure native Release

```powershell
cmake -S . -B build-win-release -G "Visual Studio 18 2026" -A Win32 "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
```

### Build native ClientGL

```powershell
cmake --build build-win-release --config Release --target ClientGL --verbose
```

### Install websockify

```powershell
py -m pip install websockify
```

### Start WebSocket proxy

```powershell
py -m websockify --verbose 4000 localhost:2238
```

### Deploy web build

```powershell
Copy-Item ".\build-web-debug\FOClassic-v7\ClientGL.js",".\build-web-debug\Source\ClientGL.wasm",".\build-web-debug\Source\ClientGL.data" "C:\Games\FOnlineServer\foclassic7\Client\" -Force
```

### Inspect `Net_OnProtoItemData`

```powershell
Get-ChildItem ".\Source" -Recurse -File -Include *.cpp,*.h | Select-String -Pattern "void FOClient::Net_OnProtoItemData|Net_OnProtoItemData\(" -Context 5,80 | ForEach-Object { $_.Context.PreContext; $_.Line; $_.Context.PostContext }
```

---

## 23. Practical rules established during the work

1. Always build native as **Win32/x86**.
2. Do not switch native configuration to x64.
3. Keep browser WebSocket port at **4000**.
4. Keep the server TCP port at **2238**.
5. Use `websockify` between browser and server.
6. Do not assume WebGL warnings marked `(Not critical)` are the current blocker.
7. Do not undo the rain workaround until the browser is stable.
8. Use `EM_JS` for browser console diagnostics in source locations where `EM_ASM` is unavailable.
9. Do not make speculative source changes when the native client already works.
10. For future diagnostic builds, verify that the new diagnostic text actually appears in the browser log before relying on it.
11. When investigating network messages, inspect both the client receiver and server sender before changing protocol handling.
12. Keep all PowerShell commands as a **single line** for copy/paste reliability.

---

## 24. Current checkpoint

At the end of the current work:

```text
Native ClientGL:                 WORKING
Emscripten compilation:          WORKING
WebAssembly memory growth:       WORKING
Browser image/font bridge:       WORKING
OldDefault font preload:         WORKING
Emscripten directory lookup:     FIXED
WebSocket proxy:                 WORKING
Server connection:               WORKING
FOEngine initialization:         WORKING
Rain/AnyFrames crash:            WORKED AROUND
Browser UI/startup:              NOT YET WORKING
Current focus:                   NETMSG_ITEM_PROTOS /
                                  Net_OnProtoItemData /
                                  ItemManager prototype handling
```

The next useful source evidence is the implementation of the server-side `NETMSG_ITEM_PROTOS` sender and browser diagnostics from inside `Net_OnProtoItemData()`.
