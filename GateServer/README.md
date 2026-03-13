# GateServer

This repository now uses CMake as the primary build entry.

## Platform support

- Windows: supported and verified in this repository
- Linux: CMake dependency skeleton is in place, but it still needs validation on a real Linux machine

## Requirements

Windows:
- Visual Studio 2022 with the C++ toolchain
- CMake 3.21 or newer
- Prebuilt third-party dependencies for:
  - Boost
  - gRPC / Protobuf / Abseil / zlib / cares / re2
  - MySQL Connector/C++
  - hiredis
  - jsoncpp

Linux:
- CMake 3.21 or newer
- A C++17 compiler
- System or custom-installed packages for:
  - Boost.Filesystem
  - Protobuf
  - gRPC
  - MySQL Connector/C++
  - hiredis
  - jsoncpp
  - pthreads

## Dependency layout

The project looks for dependencies in this order:

1. `GATESERVER_DEPS_ROOT` environment variable
2. `${repo}/third_party`
3. `E:/cppsoft`

The expected tree under the dependency root is:

```text
<deps-root>/
  boost_1_89_0/
  grpc/
    include/
    third_party/
    visualpro/
  libjsonmd/
  mysql-connector/
  reids/
```

Detailed dependency notes are in [`docs/DEPENDENCIES.md`](/e:/QtProject/GateServer/docs/DEPENDENCIES.md).

## Build

With CMake presets:

```powershell
cmake --preset vs2022-x64-debug
cmake --build --preset build-vs2022-x64-debug
```

With VS Code CMake Tools:

1. Select the configure preset `VS2022 x64 Debug`
2. Configure
3. Build

Linux skeleton example:

```bash
cmake -S . -B build-linux -G Ninja
cmake --build build-linux
```

## Run

The executable is produced at:

```text
x64/Debug/GateServer.exe
```

Run it from that directory so it can read the copied `config.ini`.

This service also depends on external services being available:

- Redis
- MySQL
- VarifyServer
- StatusServer

## Repo layout

- Headers live in [`include`](/e:/QtProject/GateServer/include)
- Sources live in [`source`](/e:/QtProject/GateServer/source)
- Runtime config lives in [`config`](/e:/QtProject/GateServer/config)
- Protocol files live in [`proto`](/e:/QtProject/GateServer/proto)
- Redistributable runtime files live in [`bin`](/e:/QtProject/GateServer/bin)
- `CMakeLists.txt` is the main build entry
- `CMakePresets.json` is the recommended configure/build entry for VS Code and CLI
- `cmake/Dependencies.cmake` resolves the dependency root and validates it
- `cmake/ThirdPartyTargets.cmake` dispatches to platform-specific dependency targets
- `cmake/ThirdPartyTargetsWindows.cmake` contains the verified Windows dependency mapping
- `cmake/ThirdPartyTargetsLinux.cmake` contains the Linux dependency skeleton
- The old Visual Studio solution/project files have been removed
