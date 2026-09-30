[English](./BUILDING.md) | Português (Brasil)

# Compilação

Os comandos abaixo são executados na raiz do repositório, salvo quando o passo diz o contrário.

## Requisitos

- Windows 10 ou 11, 64 bits
- Visual Studio 2022 com a carga de trabalho Desktop development with C++ e o Windows SDK
- CMake 3.20 ou mais recente. O CMake do Visual Studio basta quando vem com essa carga de trabalho
- Node.js 22 ou mais recente
- Inno Setup 6.7, só para o instalador

`native/CMakeLists.txt` interrompe a configuração quando o compilador não é o MSVC.

## Conferência da versão

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\check-version.ps1
```

Isso imprime a versão compartilhada, hoje `0.3.2`, ou falha quando estes arquivos discordam de `VERSION`:

- `VERSION`
- `extension/manifests/manifest.base.json`
- `extension/package.json`
- `native/CMakeLists.txt`
- `native/src/messaging/Protocol.hpp`
- `installer/shareguard.iss`

Altere `VERSION` e depois rode `scripts/apply-version.ps1`. Não edite os outros cinco à mão.

## Helper nativo

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-native.ps1
```

O script encontra o Visual Studio com `vswhere`, carrega o ambiente de desenvolvedor x64 e então executa:

```text
cmake -S native -B native/build -A x64
cmake --build native/build --config Release
```

O executável é `native\build\Release\shareguard-native.exe`.

Uma captura que não usa navegador:

```powershell
.\native\build\Release\shareguard-native.exe --capture-wav .\filtered.wav --seconds 5 --block App.exe --debug
```

`--block` pode se repetir. `--seconds` fica limitado a 1..30. O WAV é PCM estéreo a 48 kHz. Os logs de debug vão para stderr e, com debug ligado, para `%LOCALAPPDATA%\ShareGuard\shareguard.log`. Amostras de áudio não entram no log. `SHAREGUARD_DEBUG` também liga o log de debug.

## Extensão

```powershell
cd extension
npm ci
npm run typecheck
npm test
npm run build
npm run lint:firefox
```

`npm run build` grava:

- `extension/dist/chromium`
- `extension/dist/firefox`

A build falha se a chave do manifesto Chromium não produzir o ID `bdkcdhphggeglifemnakdlcfbhcoempk`, ou se o ID do Firefox não for `shareguard@shareguard.local`. Esses IDs são os que o host nativo aceita.

O zip Chromium para o usuário é separado:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\package-release.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\validate-packages.ps1
```

Isso grava `release/ShareGuard-Chromium-v0.3.2.zip`. Dentro dele, `ShareGuard-Chromium/manifest.json` é a raiz da extensão. `extension/dist/firefox` continua sendo a build de desenvolvimento sem assinatura. Ela não se chama `ShareGuard-Firefox-vX.Y.Z.xpi`.

Também existem `npm run build:chromium`, `npm run build:firefox`, `npm run dev:chromium` e `npm run dev:firefox`.

`npm test` roda `extension/test/display-media-contract.test.ts`. Ele confere o contrato do wrapper de `getDisplayMedia`. Não abre um navegador.

`npm run lint:firefox` roda `web-ext lint` em `extension/dist/firefox`.

## Instalador

Compile a extensão e o helper nativo antes. O script copia `extension/dist/chromium` para o setup. Depois, com o Inno Setup 6 instalado:

```powershell
& "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe" .\installer\shareguard.iss
```

O resultado é `installer\Output\ShareGuard-Setup-v0.3.2-x64.exe`. A versão no nome do arquivo vem de `AppVersion` em `installer/shareguard.iss`.

## Build de release

O workflow de release, numa tag como `v0.3.2`, confere a versão, compila a extensão, empacota o zip Chromium, valida os IDs, assina o Firefox quando `AMO_JWT_ISSUER` e `AMO_JWT_SECRET` estão definidos, compila o helper nativo e então o setup. Uma tag como `v0.3.2-beta.1` é publicada como pre-release e ainda precisa corresponder ao `VERSION` `0.3.2`.

Nomes publicados:

- `ShareGuard-Setup-v0.3.2-x64.exe`
- `ShareGuard-Chromium-v0.3.2.zip`
- `ShareGuard-Firefox-v0.3.2.xpi` só quando a assinatura da Mozilla deu certo
- `SHA256SUMS.txt`

Se os segredos de assinatura não existirem, o workflow ainda publica o setup e o zip Chromium, e as notas da release dizem que o pacote do Firefox para o usuário não está disponível. Se a assinatura for tentada e falhar, a release não é criada. Um arquivo sem assinatura nunca é enviado como `ShareGuard-Firefox-vX.Y.Z.xpi`.
