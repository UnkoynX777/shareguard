[English](./INSTALLATION.md) | Português (Brasil)

# Instalação do ShareGuard

O ShareGuard no Windows tem duas partes.

1. O ShareGuard Native, instalado pelo setup do Windows.
2. A extensão do ShareGuard no navegador, adicionada depois.

O setup configura o componente nativo. Ele não instala a extensão dentro do Chrome, do Edge nem do Firefox. A extensão é distribuída pelo GitHub, então o navegador pede que você a adicione.

## Antes de começar

- Windows 10 ou 11, 64 bits.
- Google Chrome 116 ou mais recente, Microsoft Edge atual, ou Firefox 128 ou mais recente.

O Firefox ESR 115 não é suportado.

Baixe somente de [github.com/UnkoynX777/shareguard/releases](https://github.com/UnkoynX777/shareguard/releases).

## Passo 1 — Instalar o ShareGuard

1. Abra a [última release](https://github.com/UnkoynX777/shareguard/releases/latest).
2. Baixe `ShareGuard-Setup-vX.Y.Z-x64.exe`.
3. Execute e conclua o setup.

O setup não precisa de conta de administrador. Ele instala o helper nativo para o usuário atual do Windows e copia os arquivos da extensão do Chrome e do Edge para:

```text
%LOCALAPPDATA%\ShareGuard\Extension\Chromium
```

Na última página, o ShareGuard avisa que a extensão ainda é necessária. Deixe **Open the installation guide** marcado se quiser abrir esta página de novo.

O ShareGuard usa a [SignPath Foundation](https://signpath.org) para assinatura de código. O Windows SmartScreen ainda pode dizer que protegeu o PC até essa assinatura ficar conhecida. As builds oficiais são só os arquivos da Release do GitHub acima. Para conferir o arquivo antes de executar, compare o SHA-256 com o `SHA256SUMS.txt` dessa mesma release:

```powershell
Get-FileHash .\ShareGuard-Setup-vX.Y.Z-x64.exe -Algorithm SHA256
```

Não execute um setup vindo de outro lugar porque apareceu um aviso, e não execute um setup vindo de outro lugar porque o aviso não apareceu.

Depois do setup, feche o Chrome, o Edge e o Firefox por completo, inclusive o ícone da bandeja, e abra o navegador de novo.

## Passo 2 — Instalar a extensão

### Chrome

Como o ShareGuard é distribuído pelo GitHub, e não por uma loja de navegador, o Chrome exige o modo de desenvolvedor para carregar a extensão manualmente.

1. Abra `chrome://extensions`.
2. Ative **Developer mode**.
3. Clique em **Load unpacked**.
4. Selecione esta pasta:

```text
%LOCALAPPDATA%\ShareGuard\Extension\Chromium
```

O atalho **ShareGuard → Install Browser Extension** no menu Iniciar abre essa pasta. Selecione a pasta em si. Não selecione um arquivo zip.

5. Fixe o ShareGuard na barra de ferramentas.
6. Abra o ShareGuard.

Mantenha essa pasta. Se você apagá-la, o Chrome deixa de ter os arquivos da extensão.

`ShareGuard-Chromium-vX.Y.Z.zip` na release é a mesma extensão. Extraia primeiro. Depois da extração, a pasta `ShareGuard-Chromium` contém `manifest.json`. Carregue essa pasta. Não carregue o zip.

### Edge

O Edge usa a mesma extensão Chromium. Não há um download separado para o Edge.

1. Instale o ShareGuard Setup.
2. Abra `edge://extensions`.
3. Ative **Developer mode**.
4. Clique em **Load unpacked**.
5. Selecione `%LOCALAPPDATA%\ShareGuard\Extension\Chromium`.
6. Fixe o ShareGuard.

### Firefox

O Firefox estável não mantém uma extensão sem assinatura depois da instalação. O pacote para o usuário é `ShareGuard-Firefox-vX.Y.Z.xpi`, na Release do GitHub. Esse arquivo é assinado pela Mozilla para distribuição pelo GitHub. Não é uma listagem pública na loja.

Se as notas da release disserem que o pacote do Firefox não está disponível, não há arquivo do Firefox para instalar nessa versão.

1. Instale o ShareGuard Setup.
2. Baixe `ShareGuard-Firefox-vX.Y.Z.xpi` da mesma release do setup.
3. Abra o Firefox.
4. Abra **Add-ons and themes** (`about:addons`).
5. Abra o menu de configurações (a engrenagem).
6. Escolha **Install Add-on From File**.
7. Selecione `ShareGuard-Firefox-vX.Y.Z.xpi`.
8. Clique em **Add**.

Não carregue um zip e não use um complemento temporário numa instalação normal.

## Passo 3 — Conferir o ShareGuard

Abra a extensão do ShareGuard.

A linha **Native** deve mostrar **Connected**.

Se aparecer isso, a instalação está concluída.

Ligue a proteção. A linha mostra **Protection enabled**. Sharing mostra **Not sharing** até um compartilhamento de tela começar, e depois **Active**.

## Atualizar

Baixe o `ShareGuard-Setup-vX.Y.Z-x64.exe` mais novo na [última release](https://github.com/UnkoynX777/shareguard/releases/latest) e execute. Ele substitui o helper nativo e os arquivos em `%LOCALAPPDATA%\ShareGuard\Extension\Chromium`. Esse caminho não muda com o número da versão, então Chrome e Edge continuam apontando para a mesma pasta.

Depois abra `chrome://extensions` ou `edge://extensions`, encontre o ShareGuard e clique em **Reload**. Reinicie o navegador se ele já estava aberto durante o setup.

A lista de bloqueio permanece no navegador.

O Firefox não atualiza a partir dessa pasta. Baixe o `ShareGuard-Firefox-vX.Y.Z.xpi` mais novo da mesma release e instale a partir do arquivo, por cima do complemento anterior.

Não há um atualizador separado.

## Desinstalar

Use Configurações do Windows, Aplicativos, ShareGuard. Também dá para usar **ShareGuard → Uninstall ShareGuard** no menu Iniciar.

Isso remove o helper nativo, o registro de Native Messaging do Chrome, do Edge e do Firefox, os arquivos da extensão em `%LOCALAPPDATA%\ShareGuard\Extension` e os atalhos do menu Iniciar.

O Chrome ou o Edge ainda podem listar o ShareGuard depois disso, porque o setup não altera o perfil do navegador. Abra a página de extensões e remova o ShareGuard ali. No Firefox, remova em **Add-ons and themes**.

## Solução de problemas

Veja [Solução de problemas](./TROUBLESHOOTING.pt-BR.md).

## Por que o ShareGuard não está nas lojas dos navegadores?

O ShareGuard é distribuído pelo GitHub. O código da extensão e os pacotes da release são públicos neste repositório.
