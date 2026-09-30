[English](./DEVELOPMENT.md) | Português (Brasil)

# Desenvolvimento

Compile o helper e a extensão primeiro. Veja [Compilação](./BUILDING.pt-BR.md).

## Organização

```text
native/src          implementação do helper
extension/src       código compartilhado da extensão
extension/manifests manifest.base.json e os overrides de Chromium e Firefox
extension/scripts   esbuild e a junção dos manifestos
installer           script do Inno Setup
scripts             build nativa, conferência de versão, empacotamento da release, registro do host de desenvolvimento
```

`extension/src/platform/browser` é a fronteira com o navegador. Chrome e Edge usam `ChromiumAdapter`. O Firefox usa `FirefoxAdapter`. Não há um adapter separado para o Edge.

O manifesto Chromium contém uma chave pública da extensão. Essa chave fixa o ID da extensão descompactada em `bdkcdhphggeglifemnakdlcfbhcoempk`. Não é uma credencial privada. Não a troque, a menos que também mude `allowed_origins` do host nativo.

## Native Messaging numa máquina de desenvolvimento

O instalador aponta o registro para `%LOCALAPPDATA%\ShareGuard\shareguard-native.exe`. Para um helper que você acabou de compilar:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install-native-host.ps1
```

Isso grava dois manifestos JSON em `%LOCALAPPDATA%\ShareGuard` e aponta Chrome, Edge e Firefox para `native\build\Release\shareguard-native.exe`. Rodar o instalador de novo aponta essas mesmas chaves de registro de volta para a cópia em `%LOCALAPPDATA%\ShareGuard`. Use um dos dois e reinicie os navegadores.

Para remover o registro de desenvolvimento:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\uninstall-native-host.ps1
```

Isso remove as três chaves de registro, os manifestos JSON e `%LOCALAPPDATA%\ShareGuard\shareguard-native.exe`. Não apaga `native\build\Release\shareguard-native.exe`.

## Carregar a extensão

Chrome e Edge, para desenvolvimento: abra `chrome://extensions` ou `edge://extensions`, ative Developer mode e carregue `extension/dist/chromium`.

A instalação do usuário carrega `%LOCALAPPDATA%\ShareGuard\Extension\Chromium`. Não aponte um navegador para `extension/dist` e para a pasta instalada ao mesmo tempo, a menos que você queira testar as duas.

Firefox, para desenvolvimento: `npm run firefox` gera o pacote do Firefox e roda o `web-ext`. Ou carregue `extension/dist/firefox/manifest.json` em `about:debugging` como complemento temporário. O Firefox descarta esse complemento ao sair. Esse caminho não é a instalação do usuário. Quem usa o produto instala `ShareGuard-Firefox-vX.Y.Z.xpi` de uma Release do GitHub depois que a Mozilla assina o arquivo.

`npm run dev:chromium` e `npm run dev:firefox` recompilam enquanto você edita. Recarregue a extensão no navegador depois de uma recompilação. Reinicie o navegador depois de substituir o executável nativo.

## Log

Passe `--debug` ao helper, ou defina `SHAREGUARD_DEBUG`. Os logs vão para stderr e para `%LOCALAPPDATA%\ShareGuard\shareguard.log`. O arquivo fica limitado a cerca de 1 MB. PCM não é registrado. Com áudio em execução, uma linha `audio capture=...` é escrita a cada dois segundos. `waits` conta quantas vezes o mixer esperou um bloco completo de 20 ms. Essa espera é normal. `drops` e um `discontinuities` que continua subindo não são. A mesma linha inclui `revision`, `coalesced`, `incremental`, `rebuilds`, `expectedStops` e `unexpected`. Um toggle de policy deve avançar `revision` e pode aumentar `coalesced` ou `incremental`. `unexpected` deve permanecer em 0. A página escreve o lado do navegador com `console.debug` durante um compartilhamento.

Para gravar o mix nativo antes do Native Messaging:

```text
shareguard-native.exe --debug --capture-wav filtered.wav --seconds 10
shareguard-native.exe --debug --tone --capture-wav filtered.wav --seconds 10
```

`--tone` toca um seno de 440 Hz, a partir de outro processo, no endpoint de renderização padrão, enquanto o helper grava o loopback. `--block App.exe` continua valendo. Nenhuma dessas flags é usada pelo host instalado.

O popup mostra o texto de erro do helper na linha de mensagem. `Native` mostra `Connected` ou `Unavailable`.

## O que não acrescentar

Não trate o hostname de um site como caso especial. O gancho é `getDisplayMedia`. Não adicione um segundo executável nativo por navegador. Não envie áudio para um servidor. Este repositório não tem arquivo `.env`.
