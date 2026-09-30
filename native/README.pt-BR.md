[English](./README.md) | Português (Brasil)

# Helper nativo

`shareguard-native.exe` é o host de Native Messaging para Chrome, Edge e Firefox. O executável e o protocolo são os mesmos para os três. Veja [Arquitetura](../docs/ARCHITECTURE.pt-BR.md) e [Compilação](../docs/BUILDING.pt-BR.md).

Sem argumentos, ele fala o protocolo versão 2 em stdin e stdout. Os logs vão para stderr e, no modo debug, para `%LOCALAPPDATA%\ShareGuard\shareguard.log`. PCM não é registrado.

```powershell
.\shareguard-native.exe --capture-wav .\filtered.wav --seconds 5 --block App.exe --debug
```

`--block` é o nome do executável e pode se repetir. `--seconds` fica limitado a 1..30. O WAV é PCM estéreo a 48 kHz.

Um executável bloqueado em execução usa exclusão da árvore. Dois ou mais usam a inclusão das árvores permitidas que estão produzindo áudio, somadas por `AudioMixer`. stderr informa `SystemLoopback`, `SingleProcessExclusion` ou `AllowedProcessMix`.
