[English](./ARCHITECTURE.md) | Português (Brasil)

# Arquitetura

O ShareGuard filtra o áudio de um compartilhamento de tela no navegador. Ele não muda o que você ouve no computador. Nenhum nome de aplicativo é tratado como caso especial. Discord, Spotify ou qualquer outro executável aparece porque está em execução.

```mermaid
flowchart TD
  apps[Aplicativos do Windows]
  core[Windows Core Audio]
  helper[shareguard-native.exe]
  policy[AudioPolicyEngine]
  wire[Native Messaging]
  background[Background da extensão]
  page[Gancho da página e Web Audio]
  site[Site do compartilhamento]

  apps --> core
  core --> helper
  helper --> policy
  policy --> helper
  helper <--> wire
  wire <--> background
  background <--> page
  page --> site
```

A página cria a faixa de áudio protegida. Um `MediaStreamTrack` criado no service worker do Chromium, na página de background do Firefox ou num offscreen document não pode ser colocado no `MediaStream` devolvido por `getDisplayMedia`. PCM é o transporte. O background só encaminha os frames.

## Helper nativo

`main.cpp` constrói `Application` e chama `run()`. `Application` liga os módulos. Ele não captura áudio nem enumera processos sozinho.

| Módulo | Papel |
| --- | --- |
| `ProcessCatalog` | Monta o snapshot de processos |
| `ProcessTree` | Agrupa uma árvore que compartilha um executável num único aplicativo |
| `ProcessIdentity` | Chave persistente: nome do executável em minúsculas. O nome de exibição vem da versão do arquivo e fica em cache |
| `AudioSessionCatalog` | IDs de processo com sessão ativa no dispositivo de saída padrão |
| `ProcessMonitor` | Varre cerca de uma vez por segundo e emite um diff quando algo mudou |
| `AudioPolicy` | Proteção ligada ou desligada, e identidades bloqueadas |
| `AudioPolicyEngine` | O único lugar que escolhe a estratégia de captura |
| `AudioEngine` | Um thread de controle aplica a policy mais recente. O thread de mix só lê o grafo publicado e empacota frames de 20 ms quando um bloco inteiro está disponível |
| `ProcessCapturePool` | Abre e fecha capturas `INCLUDE` por árvore, sem capturar um filho de uma árvore que já está incluída |
| `AudioMixer` | Soma amostras float e limita a -1..1 |
| `NativeMessagingHost` | Transporte stdin/stdout, uma conexão |
| `MessageDispatcher` | Encaminha as mensagens do protocolo |

Estratégias:

- `SystemLoopback` quando a proteção está desligada, ou quando nenhum aplicativo bloqueado está em execução. É uma captura do mix de renderização do Windows.
- `SingleProcessExclusion` quando exatamente uma identidade bloqueada tem uma raiz de árvore de processo. A chamada é `PROCESS_LOOPBACK_MODE_EXCLUDE_TARGET_PROCESS_TREE` com essa raiz em `TargetProcessId`. Os processos filhos continuam dentro da árvore excluída. Nesse caso o helper não abre uma captura por aplicativo permitido. Uma identidade com mais de uma raiz não usa esse caminho.
- `AllowedProcessMix` quando o bloqueio cobre mais de uma árvore de processo. Abre `PROCESS_LOOPBACK_MODE_INCLUDE_TARGET_PROCESS_TREE` só para raízes permitidas que estão produzindo áudio e não são descendentes de outra raiz incluída. Uma raiz que contém um processo bloqueado fica de fora. Várias exclusões não são misturadas, porque cada exclusão ainda contém o áudio permitido e somá-las duplicaria esse áudio. Depois que um compartilhamento entrou nessa estratégia, um bloqueio só não a faz voltar.

A extensão envia um snapshot completo em `SET_AUDIO_POLICY` com um `revision` monotônico. O helper guarda só a policy desejada mais recente. Revisões antigas são coalescidas. O thread de controle é o único que abre, para ou substitui uma captura. O thread de mix lê um grafo publicado e não reconstrói o pipeline porque chegou um toggle. Uma fonte que continua permitida permanece aberta. Uma exclusão única é aberta antes de aposentar o grafo anterior, para o áudio permitido não ser trocado por silêncio enquanto essa captura inicia. A passagem de uma captura para `AllowedProcessMix` publica silêncio primeiro, porque a captura anterior ainda contém os aplicativos recém-bloqueados. Adicionar um aplicativo permitido inicia essa fonte antes da troca do grafo. Um pacote só mistura fontes que tenham, cada uma, um bloco cheio de 20 ms. Uma fonte atrasada permanece no buffer dela em vez de ser completada com silêncio dentro do pacote. Um stop esperado (mudança de policy, troca de estratégia ou fim do compartilhamento) não é erro. `Capture stopped unexpectedly.` só aparece quando a geração publicada termina enquanto o compartilhamento ainda deveria estar ativo.

O formato interno é 48 kHz, estéreo, float. O WASAPI pode entregar outro formato de mix; a conversão para esse formato canônico acontece uma vez, e o resampler guarda o resto entre pacotes. A conversão para `s16le` acontece quando o `AudioFrame` é montado.

O thread de captura só copia amostras para um ring. Ele não codifica nem escreve em stdout. O thread de mix emite um pacote quando há pelo menos 960 frames na fila. Ele não completa uma leitura curta com silêncio. Enquanto um grafo publicado não tem entradas, ele envia silêncio para a mesma sequência de saída continuar. Um thread writer faz Base64 e Native Messaging. A página mantém um AudioWorklet e uma `MediaStreamTrack` durante o compartilhamento. Mudanças de policy não recriam esses objetos. A página reproduz a partir de um ring do AudioWorklet com cerca de 160 ms de prebuffer, então o jitter do transporte não é o clock de reprodução. `SHAREGUARD_DEBUG=1` registra uma linha de áudio nativo a cada dois segundos, incluindo revisões de policy, atualizações coalescidas e stops esperados versus inesperados. A página escreve a linha correspondente com `console.debug` durante um compartilhamento.

Se uma captura que deveria aplicar um bloqueio falhar, o helper não volta para o mix completo do sistema. A extensão remove a faixa de áudio compartilhada e deixa o vídeo.

O helper não ramifica em Chrome, Edge ou Firefox. O navegador só muda qual manifesto de Native Messaging e qual chave de registro podem iniciá-lo.

## Extensão

Chrome e Edge carregam o mesmo bundle como service worker do Manifest V3. O Firefox 128 ou mais recente carrega esse bundle como script de background. O acesso à API do navegador fica em `extension/src/platform/browser`. As checagens de recurso ficam em `detectCapabilities.ts`. Decisões de áudio não ramificam pelo nome do navegador.

As regras ficam em `storage.local`, na chave `shareguardRules`: `protectionEnabled`, `blockedApplications` e `showAllProcesses`. A chave é a identidade do executável, não o PID.

O popup observa esse estado. Fechar o popup não encerra um compartilhamento ativo. Sem compartilhamento e sem popup, a conexão nativa pode cair para o helper ficar ocioso.

`page/display-media-hook.ts` envolve `getDisplayMedia` no mundo MAIN da página. O content script é só uma ponte. A página não pode enviar comandos arbitrários ao helper nativo. As mensagens aceitas da página são `get-enabled`, `begin-share`, `end-share` e `protect-failed`.

Os content scripts combinam com `http://*/*` e `https://*/*` para o gancho existir em sites comuns. Os manifestos não pedem `<all_urls>`.

Com a proteção desligada, ou quando o stream não tem faixa de áudio, o stream original é devolvido. O ShareGuard não inventa uma faixa de áudio. Se a pessoa cancelar o seletor, o erro original de `getDisplayMedia`, inclusive `NotAllowedError`, volta para o site. Se a proteção estiver ligada e o helper falhar, as faixas de áudio originais são removidas e o vídeo permanece.

## Protocolo

Versão 2. O handshake é `HELLO` / `HELLO_ACK` com `protocolVersion`. `HELLO` pode incluir `client.browser` e `client.extensionVersion` para diagnóstico. O helper não muda a captura por causa desse campo. Uma versão diferente devolve `PROTOCOL_VERSION_MISMATCH`.

Mensagens: `GET_PROCESS_SNAPSHOT`, `PROCESS_SNAPSHOT`, `PROCESS_DIFF`, `SET_AUDIO_POLICY`, `AUDIO_POLICY_APPLIED`, `START_CAPTURE`, `CAPTURE_STARTED`, `STOP_CAPTURE`, `CAPTURE_STOPPED`, `AUDIO_FRAME`, `GET_STATUS`, `STATUS`, `ERROR`, `PING`, `PONG`.

Um snapshot lista aplicativos agrupados: `id`, `rootPid`, `name`, `executable`, `audioActive`, `processCount`. `id` é o executável em minúsculas. PIDs não são gravados como regras.

Códigos de erro do helper: `UNSUPPORTED_WINDOWS`, `AUDIO_INITIALIZATION_FAILED`, `PROCESS_CAPTURE_FAILED`, `PROTOCOL_VERSION_MISMATCH`, `CAPTURE_STOPPED_UNEXPECTEDLY`. Se o host não estiver registrado, o popup mostra `Native helper unavailable`. O processo do helper não está em execução, então ele não pode enviar essa frase.

`UNSUPPORTED_WINDOWS` é usado quando a ativação do process loopback falha e a build do Windows é inferior a 20348. A mensagem é `ShareGuard requires a newer version of Windows.` O helper ainda tenta a ativação a partir da build 19041. A Microsoft documenta a API a partir da build 20348.
