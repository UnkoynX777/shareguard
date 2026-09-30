[English](./TROUBLESHOOTING.md) | Português (Brasil)

# Solução de problemas

## Helper nativo não instalado

O popup diz **Native helper not installed. Install ShareGuard for Windows to continue.** A linha **Native** mostra **Unavailable**. A lista vazia diz **Native helper not installed**, e **Installation guide** aponta para os passos de instalação.

Isso aparece quando o navegador não consegue iniciar `com.shareguard.native`.

- Execute `ShareGuard-Setup-vX.Y.Z-x64.exe` da [última release](https://github.com/UnkoynX777/shareguard/releases/latest).
- Feche o navegador pela bandeja, não só a janela, e abra de novo.
- Chrome e Edge precisam de `com.shareguard.native.chromium.json`. O Firefox precisa de `com.shareguard.native.firefox.json`.

Quem compila a partir do código registra o helper com `scripts/install-native-host.ps1`. Esse não é o caminho de instalação do usuário.

## Extensão e helper nativo não combinam

A mensagem é **ShareGuard extension and native helper do not match. Install the same release for both.**

O setup e a extensão precisam ser a mesma versão do ShareGuard. Baixe os dois de uma única Release do GitHub, rode o setup, recarregue a extensão e reinicie o navegador.

`PROTOCOL_VERSION_MISMATCH` é a mesma situação quando o helper recusa a conexão. Não é uma instalação faltando.

## Extensão não aparece

Abra `chrome://extensions`, `edge://extensions` ou, no Firefox, **Add-ons and themes**, e confirme que o ShareGuard está na lista. Fixe-o pelo ícone de quebra-cabeça ou de extensões na barra.

Se a pasta `%LOCALAPPDATA%\ShareGuard\Extension\Chromium` foi apagada depois de **Load unpacked**, Chrome e Edge perdem os arquivos. Rode o setup de novo e carregue essa pasta de novo.

## Load unpacked não aparece no Chrome

**Load unpacked** só aparece com **Developer mode** ligado. Abra `chrome://extensions` e ative **Developer mode**. Depois selecione a pasta, não o zip.

## Load unpacked não aparece no Edge

Igual ao Chrome, em `edge://extensions`. Ative **Developer mode**, depois **Load unpacked**, e selecione `%LOCALAPPDATA%\ShareGuard\Extension\Chromium`.

## O Firefox diz que a extensão não está assinada

O Firefox estável não instala de forma permanente um pacote sem assinatura. Instale `ShareGuard-Firefox-vX.Y.Z.xpi` da Release do GitHub, com **Install Add-on From File**.

Se essa release disser que o pacote do Firefox não está disponível, não há arquivo assinado dessa versão. Não renomeie outro arquivo para `ShareGuard-Firefox-vX.Y.Z.xpi`.

Um complemento temporário serve só para desenvolvimento. Veja [Desenvolvimento](./DEVELOPMENT.pt-BR.md).

## Falha na conexão de Native Messaging

Confirme que estes valores de registro existem para o usuário atual e apontam para os arquivos JSON ao lado do helper:

- `HKCU\Software\Google\Chrome\NativeMessagingHosts\com.shareguard.native`
- `HKCU\Software\Microsoft\Edge\NativeMessagingHosts\com.shareguard.native`
- `HKCU\Software\Mozilla\NativeMessagingHosts\com.shareguard.native`

O JSON do Chromium precisa permitir `chrome-extension://bdkcdhphggeglifemnakdlcfbhcoempk/`. O JSON do Firefox precisa permitir `shareguard@shareguard.local`. Os dois apontam para o mesmo `shareguard-native.exe`.

Reinicie o navegador depois que o setup alterar esses arquivos. Um navegador que continua aberto mantém o processo anterior do helper.

## Esta versão do Firefox não é suportada

Esse texto aparece quando o Firefox não consegue usar Native Messaging para esta extensão. O ShareGuard exige Firefox 128 ou mais recente. O Firefox ESR 115 não oferece o gancho de página que o ShareGuard usa.

## Falta um aplicativo

A lista mostra processos que estão tocando áudio ou têm uma janela visível. Serviços sem janela ficam ocultos até você ativar **Show all processes**. A busca compara o nome de exibição e o nome do executável.

Um executável bloqueado que não está em execução permanece em **Blocked apps**.

## Quem assiste ainda ouve um aplicativo bloqueado

A proteção precisa estar ligada antes ou durante o compartilhamento. Com a proteção desligada, o stream original é usado. A linha mostra **Protection enabled** ou **Protection disabled**.

A exclusão por process loopback do Windows usa uma árvore de processo por captura. Vários aplicativos bloqueados usam um mix dos aplicativos permitidos que estão produzindo áudio. Uma raiz que também contém um processo bloqueado é deixada de fora. Se isso não puder ser feito, a extensão remove o áudio compartilhado em vez de enviar o mix do sistema.

A reprodução local não é silenciada. Ouvir o aplicativo no seu computador não significa que quem assiste também ouve.

## Sem áudio no compartilhamento

Se o site ou o navegador não pediu áudio, o ShareGuard não acrescenta uma faixa. Inclua áudio no diálogo de compartilhamento do navegador quando o site precisar.

Se a proteção estava ligada e o helper falhou, o ShareGuard remove a faixa de áudio compartilhada e mantém o vídeo. A linha de mensagem do popup mostra o erro do helper. A página também pode mostrar: `ShareGuard could not protect this share, so shared audio was removed. Playback on this computer was not changed.`

## O diálogo de compartilhamento foi cancelado

O site ainda deve receber a falha original de `getDisplayMedia`, inclusive `NotAllowedError`. O ShareGuard não troca esse erro por um stream silencioso.

## O ShareGuard pede uma versão mais nova do Windows

A ativação do process loopback falhou e a build do Windows é inferior a 20348. O helper tenta a partir da build 19041, que é anterior à build que a Microsoft documenta. Numa build abaixo de 20348, a falha vira `UNSUPPORTED_WINDOWS` com essa frase. Atualize o Windows, ou deixe a proteção desligada se você aceitar o áudio original do compartilhamento.

## A proteção de áudio falhou

`AUDIO_INITIALIZATION_FAILED` e `PROCESS_CAPTURE_FAILED` vêm do helper quando um dispositivo ou uma captura de processo não pode ser aberta. `CAPTURE_STOPPED_UNEXPECTEDLY` é enviado quando a captura reinicia vezes demais. O vídeo deve permanecer. O áudio compartilhado é removido.

## Popup fechado durante um compartilhamento

O compartilhamento continua. O popup não é a sessão de captura. Abra de novo para mudar a lista de bloqueio. O helper aplica a nova política sem um novo compartilhamento de tela.
