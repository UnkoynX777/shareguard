# Native helper

`shareguard-native.exe` is the Native Messaging host for Chrome, Edge, and Firefox. The executable and the protocol are the same for all three. See [Architecture](../docs/ARCHITECTURE.md) and [Building](../docs/BUILDING.md).

Without arguments it speaks protocol version 2 on stdin and stdout. Logs go to stderr and, in debug mode, to `%LOCALAPPDATA%\ShareGuard\shareguard.log`. PCM is not logged.

```powershell
.\shareguard-native.exe --capture-wav .\filtered.wav --seconds 5 --block App.exe --debug
```

`--block` is the executable name and can be repeated. `--seconds` is clamped to 1..30. The WAV is 48 kHz stereo PCM.

One running blocked executable uses tree exclusion. Two or more use inclusion of the allowed trees that are producing audio, summed by `AudioMixer`. stderr reports `SystemLoopback`, `SingleProcessExclusion`, or `AllowedProcessMix`.
