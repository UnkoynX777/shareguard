cmake_minimum_required(VERSION 3.20)
project(ShareGuardNative VERSION 0.3.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT MSVC)
  message(FATAL_ERROR "ShareGuard native helper requires MSVC and the Windows SDK.")
endif()

add_executable(shareguard-native
  src/main.cpp
  src/app/Application.cpp
  src/logging/Logger.cpp
  src/platform/windows/WinText.cpp
  src/platform/windows/ComRuntime.cpp
  src/platform/windows/WindowsVersion.cpp
  src/platform/windows/ProcessApi.cpp
  src/process/ProcessModel.cpp
  src/process/ProcessIdentity.cpp
  src/process/ProcessTree.cpp
  src/process/ProcessCatalog.cpp
  src/process/AudioSessionCatalog.cpp
  src/process/ProcessMonitor.cpp
  src/policy/AudioPolicy.cpp
  src/audio/format/AudioConverter.cpp
  src/audio/format/AudioFrame.cpp
  src/audio/format/WavWriter.cpp
  src/audio/buffer/AudioRingBuffer.cpp
  src/audio/capture/WasapiLoopbackSession.cpp
  src/audio/capture/LoopbackCapture.cpp
  src/audio/capture/CaptureSource.cpp
  src/audio/capture/ProcessCapturePool.cpp
  src/audio/engine/AudioMixer.cpp
  src/audio/engine/AudioPolicyEngine.cpp
  src/audio/engine/AudioEngine.cpp
  src/messaging/Protocol.cpp
  src/messaging/NativeMessagingHost.cpp
  src/messaging/MessageDispatcher.cpp
)

target_include_directories(shareguard-native PRIVATE src)

target_compile_definitions(shareguard-native PRIVATE
  UNICODE
  _UNICODE
  WIN32_LEAN_AND_MEAN
  NOMINMAX
  _CRT_SECURE_NO_WARNINGS
)

set_property(TARGET shareguard-native PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_compile_options(shareguard-native PRIVATE
  /W4
  /EHsc
  /utf-8
  /permissive-
  $<$<CONFIG:Release>:/O1>
  $<$<CONFIG:Debug>:/Od>
)

target_link_options(shareguard-native PRIVATE
  /OPT:REF
  /OPT:ICF
  /INCREMENTAL:NO
  $<$<CONFIG:Release>:/LTCG>
)

target_compile_options(shareguard-native PRIVATE $<$<CONFIG:Release>:/GL>)

target_link_libraries(shareguard-native PRIVATE
  mmdevapi
  ole32
  avrt
  uuid
  version
)
