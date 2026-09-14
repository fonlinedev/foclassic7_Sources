# PReloaded - Reloaded Prototype

**FOClassic server example based on FOnline Reloaded**

This project is a customized FOClassic/FOnline prototype used for development and testing. Most original example content has been reduced while retaining functionality needed for more complex tests.

## Project Status

This repository is being used as a development base for **FOnline Revival / Brave New World** work.

Changes are being made primarily in the server and AngelScript layers, while the FOClassic client source is also being maintained so the client executables can be rebuilt from source when renderer-specific testing is required.

## Current Changes and Development Notes

### Character Generation

The character-generation system is being customized around the `CritterGenerate` function.

Current work includes changes to skill calculation during character creation, particularly tagged skills and passive skills.

#### Passive Tagged Skills

Passive skills include:

- Scavenging
- Repair
- Science
- Lockpick
- Outdoorsman
- Barter
- Gambling

The previous character-generation logic contained special handling for passive skills when they were tagged:

```cpp
cr.SkillBase[skill] = int(0x80000000);
cr.TagSkillBase[t] = 0;
```

This behavior was disabled because `0x80000000`, when interpreted as a signed integer, is a negative sentinel value. It could subsequently be treated as a normal skill value and produce invalid negative results.

A notable example was **Outdoorsman**: tagging the skill could cause the resulting value to become approximately **-300%**, making the character effectively unusable.

The passive-skill override is therefore currently disabled so tagged passive skills retain their normal calculated values.

### Client Car Tracker

The project/server does not use the car system.

A runtime exception was observed during map changes in both the GL and DX clients:

```text
Script exception: Null pointer access
client/client_CarTracker : CCarTracker::refresh(...)
RefreshCars(...)
_ShowCarOwnerIndicator(...)
```

The CarTracker system is unused functionality for this project and should not interfere with normal map loading or gameplay.

Client `.fos` scripts can be modified without recompiling the native client executable. This allows unused client-side systems to be disabled and tested quickly.

### Client Build / Renderer Testing

The project contains both OpenGL and DirectX client targets.

The client source can be built through the Visual Studio/CMake project rather than relying exclusively on precompiled executables.

Relevant targets include:

- `ClientGL`
- `ClientDX`

The renderer is selected through compile-time definitions such as:

```text
FO_D3D
FOCLASSIC_CLIENT
FOCLASSIC_ENGINE
```

The goal is to keep both clients buildable from source so the default GL client and DX client can be tested against the same server and data set.

This is important when determining whether a problem originates in:

1. the native client,
2. the renderer,
3. the client AngelScript layer, or
4. the server.

### Runtime Client Scripts

The client uses AngelScript `.fos` files for a significant portion of its gameplay and UI behavior.

These scripts can be changed independently from the native C++ client executable.

The resulting development cycle is:

```text
Edit .fos script
      ↓
Start client
      ↓
Test
      ↓
Check log
      ↓
Repeat
```

Native C++ changes still require rebuilding the appropriate client executable.

## Development Approach

When diagnosing a problem, first determine which layer is responsible before changing code:

```text
Server
  ↓
Network protocol
  ↓
Native client
  ↓
Client AngelScript
  ↓
UI / gameplay systems
```

A client-side script exception does not necessarily mean the native client executable is broken. If the same behavior occurs with both GL and DX clients, the server or shared script/data layer should be investigated first.

## Original Project References

### FOClassic

https://rotators.fodev.net/foclassic/

https://github.com/rotators/foclassic/

### FOnline Reloaded

https://www.fonline-reloaded.net/

### Fallout of Nevada

Content by Nevada Band Studio.

http://fallout-nevada.ucoz.ru/

## Credits and Licensing Notes

Most original example content was removed, but as much functionality as possible remained to allow more complex tests.

Fallout of Nevada content is credited to Nevada Band Studio. Credits are required if their content is used in another project.