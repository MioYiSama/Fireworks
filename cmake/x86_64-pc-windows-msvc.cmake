# Cross-compile to Windows x64 with clang-cl + lld-link against an MSVC/Windows SDK sysroot
# laid out as <sysroot>/VC/Tools/MSVC/<ver> and "<sysroot>/Windows Kits/10".

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_VERSION 10.0)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

set(WINDOWS_SYSROOT "/opt/x86_64-pc-windows-msvc" CACHE PATH "MSVC + Windows SDK sysroot")
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES WINDOWS_SYSROOT)

set(CMAKE_C_COMPILER clang-cl)
set(CMAKE_CXX_COMPILER clang-cl)
set(CMAKE_C_COMPILER_TARGET x86_64-pc-windows-msvc)
set(CMAKE_CXX_COMPILER_TARGET x86_64-pc-windows-msvc)
set(CMAKE_LINKER lld-link)
set(CMAKE_AR llvm-lib)
set(CMAKE_RC_COMPILER llvm-rc)
set(CMAKE_MT llvm-mt)

set(CMAKE_C_FLAGS_INIT "/winsysroot ${WINDOWS_SYSROOT}")
set(CMAKE_CXX_FLAGS_INIT "/winsysroot ${WINDOWS_SYSROOT}")
foreach(kind EXE SHARED MODULE)
  set(CMAKE_${kind}_LINKER_FLAGS_INIT "/winsysroot:${WINDOWS_SYSROOT}")
endforeach()

# llvm-rc preprocesses through clang-cl without CMAKE_C_FLAGS, so /winsysroot never reaches it.
file(GLOB _msvc_dirs LIST_DIRECTORIES true "${WINDOWS_SYSROOT}/VC/Tools/MSVC/*")
file(GLOB _sdk_dirs LIST_DIRECTORIES true "${WINDOWS_SYSROOT}/Windows Kits/10/Include/*")
list(SORT _msvc_dirs COMPARE NATURAL ORDER DESCENDING)
list(SORT _sdk_dirs COMPARE NATURAL ORDER DESCENDING)
list(GET _msvc_dirs 0 _msvc_dir)
list(GET _sdk_dirs 0 _sdk_dir)
set(CMAKE_RC_STANDARD_INCLUDE_DIRECTORIES
  "${_msvc_dir}/include" "${_sdk_dir}/ucrt" "${_sdk_dir}/shared" "${_sdk_dir}/um"
)

# Static CRT: the .exe runs without the VC++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(SFML_USE_STATIC_STD_LIBS ON CACHE BOOL "")

set(CMAKE_FIND_ROOT_PATH "${WINDOWS_SYSROOT}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
