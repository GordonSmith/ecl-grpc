# Copilot Instructions

## Architecture & Targets
- The root `CMakeLists.txt` defines a single library target `ecl_protobuf` (aliased as `ecl::protobuf`). Place implementation files in `src/` and public headers in `include/`; both folders are auto-discovered via `file(GLOB_RECURSE ...)`.
- Even if no sources exist, an `INTERFACE` library is created so downstream projects can still consume headers; prefer adding real `.cc/.cpp` files instead of editing the interface-only path.
- Generated protobuf code is treated as part of `ecl_protobuf`. CMake writes outputs under `build/<preset>/generated` and wires the directory into the public include path so consumers can `#include "addressbook.pb.h"` without extra work.

## Dependency Management
- vcpkg is vendored as a submodule (`vcpkg/`) and the manifest at `vcpkg.json` pins runtime deps (`protobuf`, `grpc`). Run `./vcpkg/vcpkg install` once per machine, or rely on CMake to trigger installs when configuring.
- Toolchain detection happens before `project()`: if `CMAKE_TOOLCHAIN_FILE` is unset, the script prefers `${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake`, then the local `vcpkg` clone. Respect this flow when adding new cache vars to avoid reordering the project command.
- Custom ports or registry overrides belong in `vcpkg_overlays/`; keep overlay layout consistent with upstream vcpkg expectations (each subfolder mirrors a port name).

## Build & Test Workflow
- Use presets from `CMakePresets.json` (version 10) instead of ad-hoc builds:
  - Configure: `cmake --preset debug` (or `relwithdebinfo`, `release`).
  - Build: `cmake --build --preset debug`.
  - Tests: `ctest --preset debug` (CTest is enabled only when `ECL_PROTOBUF_BUILD_TESTS` and `BUILD_TESTING` are true and `tests/` exists).
- Useful cache toggles (all default ON unless noted): `ECL_PROTOBUF_ENABLE_WARNINGS`, `ECL_PROTOBUF_WARNINGS_AS_ERRORS` (OFF), `ECL_PROTOBUF_BUILD_TESTS`, `ECL_PROTOBUF_BUILD_EXAMPLES`. Pass them via `-D` on the configure command or by extending presets.
- `CMAKE_EXPORT_COMPILE_COMMANDS` is enforced globally; keep Ninja as the default generator to preserve this behavior.

## Proto Compilation
- `find_package(protobuf CONFIG REQUIRED)` resolves vcpkg targets (`protobuf::libprotobuf`, `protobuf::libprotoc`). Use those imported targets when linking executables/tests instead of raw paths.
- `proto/route_guide.proto` is compiled via a custom command that prefers `${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/protobuf/protoc`, then `${repo}/vcpkg_installed/x64-linux/...`, then any system `protoc`. If you add new `.proto` files, extend this section rather than introducing a parallel invocation so all generated files share the same protoc binary and output dir.
- Keep proto search paths explicit: the generator currently points `--proto_path` to `proto/`. If you add nested directories (e.g., `proto2/`), either add extra `--proto_path` entries or relocate files under `proto/` for consistency.

## Coding Conventions & Integration Points
- Export/install rules already exist for `ecl_protobuf`; ensure any new targets also call `install(TARGETS ...)` and add themselves to the `ecl-protobuf-targets` export if they are part of the public API.
- When adding client binaries (examples/tests), place them under `examples/` or `tests/` so the existing `add_subdirectory` logic can discover them conditionally based on the build options.
- Favor modern CMake idioms (`target_link_libraries` with PUBLIC/PRIVATE scopes, generator expressions for options). Avoid editing `vcpkg_installed/` manually—use manifests or overlays instead.
