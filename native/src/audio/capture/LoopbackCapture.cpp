#include "audio/capture/LoopbackCapture.hpp"

#include "logging/Logger.hpp"
#include "platform/windows/WinHandle.hpp"

#include <audioclient.h>
#include <audioclientactivationparams.h>
#include <mmdeviceapi.h>

#include <atomic>
#include <wrl/client.h>

namespace shareguard {
namespace {

class ActivationCompletion final : public IActivateAudioInterfaceCompletionHandler, public IAgileObject {
 public:
  explicit ActivationCompletion(HANDLE completed) : completed_(completed) {}

  HRESULT QueryInterface(REFIID riid, void** object) override {
    if (object == nullptr) {
      return E_POINTER;
    }
    if (riid == __uuidof(IUnknown) || riid == __uuidof(IActivateAudioInterfaceCompletionHandler)) {
      *object = static_cast<IActivateAudioInterfaceCompletionHandler*>(this);
      AddRef();
      return S_OK;
    }
    if (riid == __uuidof(IAgileObject)) {
      *object = static_cast<IAgileObject*>(this);
      AddRef();
      return S_OK;
    }
    *object = nullptr;
    return E_NOINTERFACE;
  }

  ULONG AddRef() override { return ++refs_; }
  ULONG Release() override {
    const ULONG refs = --refs_;
    if (refs == 0) {
      delete this;
    }
    return refs;
  }

  HRESULT ActivateCompleted(IActivateAudioInterfaceAsyncOperation* operation) override {
    HRESULT activateResult = E_FAIL;
    Microsoft::WRL::ComPtr<IUnknown> unknown;
    const HRESULT query = operation->GetActivateResult(&activateResult, &unknown);
    if (FAILED(query)) {
      result_.store(query);
    } else if (FAILED(activateResult)) {
      result_.store(activateResult);
    } else {
      result_.store(unknown.As(&client_));
    }
    SetEvent(completed_);
    return S_OK;
  }

  HRESULT result() const { return result_.load(); }
  Microsoft::WRL::ComPtr<IAudioClient> client() const { return client_; }

 private:
  std::atomic<ULONG> refs_{1};
  HANDLE completed_ = nullptr;
  std::atomic<HRESULT> result_{E_FAIL};
  Microsoft::WRL::ComPtr<IAudioClient> client_;
};

Microsoft::WRL::ComPtr<IAudioClient> activateProcessLoopback(DWORD processId, ProcessLoopbackMode mode,
                                                            std::string& error) {
  ScopedHandle completed(CreateEventW(nullptr, FALSE, FALSE, nullptr));
  if (!completed) {
    error = "Failed to create the activation event.";
    return nullptr;
  }

  AUDIOCLIENT_ACTIVATION_PARAMS params = {};
  params.ActivationType = AUDIOCLIENT_ACTIVATION_TYPE_PROCESS_LOOPBACK;
  params.ProcessLoopbackParams.TargetProcessId = processId;
  params.ProcessLoopbackParams.ProcessLoopbackMode = mode == ProcessLoopbackMode::IncludeTree
                                                         ? PROCESS_LOOPBACK_MODE_INCLUDE_TARGET_PROCESS_TREE
                                                         : PROCESS_LOOPBACK_MODE_EXCLUDE_TARGET_PROCESS_TREE;

  PROPVARIANT activateParams = {};
  activateParams.vt = VT_BLOB;
  activateParams.blob.cbSize = sizeof(params);
  activateParams.blob.pBlobData = reinterpret_cast<BYTE*>(&params);

  Microsoft::WRL::ComPtr<ActivationCompletion> handler;
  handler.Attach(new ActivationCompletion(completed.get()));

  Microsoft::WRL::ComPtr<IActivateAudioInterfaceAsyncOperation> operation;
  const HRESULT started = ActivateAudioInterfaceAsync(VIRTUAL_AUDIO_DEVICE_PROCESS_LOOPBACK, __uuidof(IAudioClient),
                                                      &activateParams, handler.Get(), &operation);
  if (FAILED(started)) {
    error = hresultText("ActivateAudioInterfaceAsync", started);
    return nullptr;
  }

  const DWORD wait = WaitForSingleObject(completed.get(), 5000);
  if (wait != WAIT_OBJECT_0) {
    error = "Timed out activating process loopback.";
    return nullptr;
  }
  if (FAILED(handler->result())) {
    error = hresultText("Process loopback activation", handler->result());
    return nullptr;
  }
  return handler->client();
}

bool openProcessWithFallback(DWORD processId, ProcessLoopbackMode mode, WasapiLoopbackSession& session,
                             std::string& error) {
  Microsoft::WRL::ComPtr<IAudioClient> client = activateProcessLoopback(processId, mode, error);
  if (!client) {
    return false;
  }
  if (session.initialize(std::move(client), error)) {
    return true;
  }

  const std::string preferred = error;
  client = activateProcessLoopback(processId, mode, error);
  if (!client) {
    error = preferred + " " + error;
    return false;
  }
  if (!session.initializeMix(std::move(client), error)) {
    error = preferred + " " + error;
    return false;
  }
  return true;
}

Microsoft::WRL::ComPtr<IAudioClient> activateDefaultRender(std::string& error) {
  Microsoft::WRL::ComPtr<IMMDeviceEnumerator> enumerator;
  HRESULT result = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator));
  if (FAILED(result)) {
    error = hresultText("CoCreateInstance(MMDeviceEnumerator)", result);
    return nullptr;
  }

  Microsoft::WRL::ComPtr<IMMDevice> device;
  result = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
  if (FAILED(result)) {
    error = hresultText("GetDefaultAudioEndpoint", result);
    return nullptr;
  }

  Microsoft::WRL::ComPtr<IAudioClient> client;
  result = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, &client);
  if (FAILED(result)) {
    error = hresultText("IMMDevice::Activate", result);
    return nullptr;
  }
  return client;
}

}

bool ProcessLoopbackCapture::open(std::uint32_t processId, ProcessLoopbackMode mode, WasapiLoopbackSession& session,
                                  std::string& error) {
  Logger::info(mode == ProcessLoopbackMode::IncludeTree ? "capture source include" : "capture source exclude");
  return openProcessWithFallback(processId, mode, session, error);
}

bool SystemLoopbackCapture::open(WasapiLoopbackSession& session, std::string& error) {
  Logger::info("capture source system");
  Microsoft::WRL::ComPtr<IAudioClient> client = activateDefaultRender(error);
  if (!client) {
    return false;
  }
  if (session.initialize(std::move(client), error)) {
    return true;
  }

  const std::string preferred = error;
  client = activateDefaultRender(error);
  if (!client) {
    error = preferred + " " + error;
    return false;
  }
  if (!session.initializeMix(std::move(client), error)) {
    error = preferred + " " + error;
    return false;
  }
  return true;
}

}
