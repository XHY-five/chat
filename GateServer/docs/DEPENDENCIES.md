# Dependencies

This project now has a platform-dispatched dependency layer:

- Windows uses prebuilt local libraries
- Linux uses a CMake skeleton based on `find_package`, `pkg-config`, and `find_library`

For Windows, CMake resolves the dependency bundle in this order:

1. `GATESERVER_DEPS_ROOT` environment variable
2. `${repo}/third_party`
3. `E:/cppsoft`

## Expected layout

```text
<deps-root>/
  boost_1_89_0/
  grpc/
    include/
    third_party/
    visualpro/
      Debug/
      third_party/
  libjsonmd/
    include/
    lib/
  mysql-connector/
    include/
    lib64/
      vs14/
        debug/
  reids/
    deps/
      hiredis/
    lib/
```

## Current build model

- The project is standardized around imported CMake targets defined in [`cmake/ThirdPartyTargets.cmake`](/e:/QtProject/GateServer/cmake/ThirdPartyTargets.cmake).
- The main target links only `GateServer::third_party`.
- Dependency root validation lives in [`cmake/Dependencies.cmake`](/e:/QtProject/GateServer/cmake/Dependencies.cmake).
- Project headers live in [`include`](/e:/QtProject/GateServer/include) and sources live in [`source`](/e:/QtProject/GateServer/source).
- Runtime config lives in [`config`](/e:/QtProject/GateServer/config), protocol files in [`proto`](/e:/QtProject/GateServer/proto), and redistributable runtime files in [`bin`](/e:/QtProject/GateServer/bin).
- Platform-specific dependency glue lives in:
  - [`cmake/ThirdPartyTargetsWindows.cmake`](/e:/QtProject/GateServer/cmake/ThirdPartyTargetsWindows.cmake)
  - [`cmake/ThirdPartyTargetsLinux.cmake`](/e:/QtProject/GateServer/cmake/ThirdPartyTargetsLinux.cmake)

## Important note about configurations

Your current dependency bundle appears to be mostly `Debug`-only for gRPC-related libraries.

Because of that:

- `Debug` is the supported configuration
- `Release` / `RelWithDebInfo` / `MinSizeRel` are currently mapped to Debug libs by default

This behavior is controlled by:

```cmake
GATESERVER_ALLOW_DEBUG_ONLY_DEPS=ON
```

If you later produce proper Release libraries, CMake will pick them up automatically where the expected files exist.

## How another machine should prepare dependencies

Option 1:
- Copy your current `E:/cppsoft` bundle to another machine
- Set `GATESERVER_DEPS_ROOT` to that copied folder

Option 2:
- Place the same bundle under `${repo}/third_party`

## Configure and build

```powershell
cmake --preset vs2022-x64-debug
cmake --build --preset build-vs2022-x64-debug
```

Linux skeleton example:

```bash
cmake -S . -B build-linux -G Ninja
cmake --build build-linux
```

## Linux notes

The Linux CMake skeleton currently expects these packages or equivalents to be discoverable:

- `Boost::filesystem`
- `protobuf::libprotobuf`
- `gRPC::grpc++`
- `gRPC::grpc++_reflection`
- `hiredis` via `pkg-config`
- `jsoncpp` via `pkg-config`
- `mysql/jdbc.h` and `libmysqlcppconn`

This part has not been validated in this Windows workspace yet, so treat it as the starting scaffold for Linux enablement rather than a finished Linux port.
