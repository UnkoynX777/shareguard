[English](./CHANGELOG.md) | Português (Brasil)

# Registro de alterações

O formato segue [Keep a Changelog](https://keepachangelog.com/pt-BR/1.1.0/), e este projeto segue [Versionamento Semântico](https://semver.org/lang/pt-BR/spec/v2.0.0.html).

A versão em inglês é canônica. Esta página acompanha as mesmas releases.

## [Unreleased]

## [0.3.1] - 2026-09-30

### Corrigido

- O áudio compartilhado contínuo deixa de ser cortado pelo silêncio que o mixer inseria quando andava na frente da captura. A reprodução espera um buffer curto para o jitter do transporte não virar buraco.

### Adicionado

- Traduções em português brasileiro da documentação para usuários e contribuidores. O inglês continua sendo o idioma canônico.

## [0.3.0] - 2026-09-30

### Adicionado

- Filtro por aplicativo do áudio de compartilhamento de tela no navegador, no Windows, sem mudar a reprodução local.
- Um helper nativo, `shareguard-native.exe`, compartilhado por Chrome, Edge e Firefox via Native Messaging.
- Builds da extensão Chromium e Firefox a partir de uma base TypeScript.
- Um instalador por usuário que registra o host nativo para Chrome, Edge e Firefox e copia a extensão Chromium para `%LOCALAPPDATA%\ShareGuard\Extension\Chromium`.
- Pacotes de GitHub Release `ShareGuard-Setup-vX.Y.Z-x64.exe`, `ShareGuard-Chromium-vX.Y.Z.zip` e, quando a assinatura unlisted da Mozilla está configurada, `ShareGuard-Firefox-vX.Y.Z.xpi`.

[Unreleased]: https://github.com/UnkoynX777/shareguard/compare/v0.3.1...HEAD
[0.3.1]: https://github.com/UnkoynX777/shareguard/releases/tag/v0.3.1
[0.3.0]: https://github.com/UnkoynX777/shareguard/releases/tag/v0.3.0
