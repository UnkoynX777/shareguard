#include "platform/windows/ProcessApi.hpp"

#include "logging/Logger.hpp"
#include "platform/windows/WinHandle.hpp"

#include <audiopolicy.h>
#include <mmdeviceapi.h>
#include <tlhelp32.h>

#include <wrl/client.h>

namespace shareguard {

std::vector<RawProcess> enumerateProcesses() {
  std::vector<RawProcess> processes;
  ScopedHandle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
  if (!snapshot) {
    Logger::error("CreateToolhelp32Snapshot failed.");
    return processes;
  }

  PROCESSENTRY32W entry{};
  entry.dwSize = sizeof(entry);
  if (!Process32FirstW(snapshot.get(), &entry)) {
    Logger::error("Process32FirstW failed.");
    return processes;
  }

  do {
    RawProcess process;
    process.pid = entry.th32ProcessID;
    process.parentPid = entry.th32ParentProcessID;
    process.executableName = entry.szExeFile;
    ScopedHandle handle(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process.pid));
    if (handle) {
      wchar_t path[32768] = {};
      DWORD size = 32768;
      if (QueryFullProcessImageNameW(handle.get(), 0, path, &size)) {
        process.executablePath = path;
      }
    }
    processes.push_back(std::move(process));
  } while (Process32NextW(snapshot.get(), &entry));
  return processes;
}

std::unordered_set<std::uint32_t> visibleWindowProcessIds() {
  std::unordered_set<std::uint32_t> pids;
  EnumWindows(
      [](HWND window, LPARAM parameter) -> BOOL {
        auto* output = reinterpret_cast<std::unordered_set<std::uint32_t>*>(parameter);
        if (!IsWindowVisible(window) || GetWindow(window, GW_OWNER) != nullptr) {
          return TRUE;
        }
        wchar_t title[8] = {};
        if (GetWindowTextW(window, title, 8) == 0) {
          return TRUE;
        }
        DWORD pid = 0;
        GetWindowThreadProcessId(window, &pid);
        if (pid != 0) {
          output->insert(pid);
        }
        return TRUE;
      },
      reinterpret_cast<LPARAM>(&pids));
  return pids;
}

std::unordered_set<std::uint32_t> activeAudioProcessIds() {
  std::unordered_set<std::uint32_t> pids;
  Microsoft::WRL::ComPtr<IMMDeviceEnumerator> enumerator;
  HRESULT result = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator));
  if (FAILED(result)) {
    return pids;
  }
  Microsoft::WRL::ComPtr<IMMDevice> device;
  result = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
  if (FAILED(result)) {
    return pids;
  }
  Microsoft::WRL::ComPtr<IAudioSessionManager2> manager;
  result = device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, &manager);
  if (FAILED(result)) {
    return pids;
  }
  Microsoft::WRL::ComPtr<IAudioSessionEnumerator> sessions;
  result = manager->GetSessionEnumerator(&sessions);
  if (FAILED(result)) {
    return pids;
  }
  int count = 0;
  sessions->GetCount(&count);
  for (int index = 0; index < count; ++index) {
    Microsoft::WRL::ComPtr<IAudioSessionControl> control;
    if (FAILED(sessions->GetSession(index, &control))) {
      continue;
    }
    Microsoft::WRL::ComPtr<IAudioSessionControl2> control2;
    if (FAILED(control.As(&control2))) {
      continue;
    }
    AudioSessionState state = AudioSessionStateInactive;
    control2->GetState(&state);
    if (state != AudioSessionStateActive) {
      continue;
    }
    DWORD pid = 0;
    if (SUCCEEDED(control2->GetProcessId(&pid)) && pid != 0) {
      pids.insert(pid);
    }
  }
  return pids;
}

}
