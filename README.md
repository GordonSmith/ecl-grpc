# ecl-grpc

A small experimental workspace for wiring HPCC's ECL dataflows into modern protobuf and gRPC tooling. The root CMake project builds the `ecl::protobuf` library together with a demo client that exercises both plain protobuf serialization (address book sample) and the bidirectional Route Guide RPCs from the upstream gRPC examples.

## Prerequisites
- CMake 3.24+ and a C++20-capable compiler (GCC, Clang, or MSVC)
- Ninja (or another generator supported by CMake presets)
- `vcpkg` bootstrap once per clone: `./vcpkg/bootstrap-vcpkg.sh -disableMetrics`
- A running Route Guide server on `localhost:50051` to see the gRPC client in action (you can launch the reference server from `examples/cpp/route_guide/server.cc` in the upstream gRPC repo or any compatible implementation)

## Quick Start
```bash
cmake --preset debug       # configure, installs protobuf/grpc via the bundled vcpkg manifest
cmake --build --preset debug
./build/debug/ecl-grpc     # runs the route_guide demo client using the embedded dataset
```
The configure step auto-generates C++ sources for every `.proto` file under `proto/` (address book + route guide today). Generated headers end up under `src/generated/` and are transparently exposed by the `ecl::protobuf` target.

## Repo Layout
- `proto/` – canonical protobuf definitions compiled as part of the default build
- `src/` – library sources, the demo client, and supporting assets such as `route_guide_db.json`
- `vcpkg/` – vendored dependency manager; manifests live at the repo root

## ECL Harness
The repo still targets HPCC's ECL tooling: `src/main.ecl` is a small job that embeds the C++ client so you can execute it inside an ECL cluster.
- `src/main.ecl` uses `EMBED(C++)` to pull in `grpc-client.hpp`, call `test()` / `test2()`, and surface a helper function (`cppVersion()`) that reports the compiler level used during codegen. You can submit it with `ecl run src/main.ecl --manifest=src/main.manifest` to exercise the C++ client from within an HPCC workunit.
- `src/main.manifest` lists every C++ header/source (including generated protobuf stubs) needed by the embedded snippet. When `eclcc` bundles the job, it zips these files next to the workunit so the HPCC Thor/ROXIE engines can compile the C++ payload in-place.

If you add new C++ helpers that the ECL job should see, remember to update both the manifest and the `EMBED` section to include the new headers.

## Useful CMake Options
The presets default to Debug + warnings enabled. You can toggle behavior when configuring:
- `-DECL_PROTOBUF_BUILD_TESTS=OFF` – skip `tests/` (once populated)
- `-DECL_PROTOBUF_BUILD_EXAMPLES=ON` – recurse into `examples/`
- `-DECL_PROTOBUF_WARNINGS_AS_ERRORS=ON` – treat compiler warnings as errors

Tests, once added, can be run with `ctest --preset debug`. For reproducible builds in other configurations, swap the preset name (`relwithdebinfo`, `release`, …).

## Adding New Protos
1. Drop the `.proto` file under `proto/`.
2. Re-run `cmake --build --preset <preset>`; the custom commands will regenerate sources with the same `protoc` binary that vcpkg installed.
3. Include the new headers via `#include "generated/<name>.pb.h"` and update any client code under `src/`.

Feel free to extend the gRPC client or add new binaries under `examples/` or `tests/`; the top-level CMake already contains the boilerplate to pick them up when the corresponding options are enabled.
