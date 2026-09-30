<p align="center">
  <a href="./README.md">English</a> · <a href="./README.pt-BR.md">Português (Brasil)</a>
</p>

<p align="center">
  <img src="extension/public/icons/icon128.png" width="96" alt="Ícone do ShareGuard">
</p>

<h1 align="center">ShareGuard</h1>

<p align="center">
  Compartilhe a tela. Deixe os aplicativos privados fora do áudio.
</p>

<p align="center">
  Filtre o áudio dos aplicativos no compartilhamento do navegador sem silenciar o que você ouve no computador.
</p>

<p align="center">
  <a href="https://github.com/UnkoynX777/shareguard/releases/latest"><img alt="Release no GitHub" src="https://img.shields.io/github/v/release/UnkoynX777/shareguard?display_name=tag"></a>
  <a href="https://github.com/UnkoynX777/shareguard/actions/workflows/ci.yml"><img alt="CI" src="https://github.com/UnkoynX777/shareguard/actions/workflows/ci.yml/badge.svg"></a>
  <a href="./LICENSE"><img alt="Licença MIT" src="https://img.shields.io/badge/license-MIT-blue"></a>
  <img alt="Windows x64" src="https://img.shields.io/badge/platform-Windows%20x64-blue">
  <img alt="Chrome, Edge e Firefox" src="https://img.shields.io/badge/browsers-Chrome%20%C2%B7%20Edge%20%C2%B7%20Firefox-blue">
</p>

<p align="center">
  <a href="https://github.com/UnkoynX777/shareguard/releases/latest"><strong>Baixar para Windows</strong></a>
  ·
  <a href="./docs/INSTALLATION.md">Instalação</a>
  ·
  <a href="https://github.com/UnkoynX777/shareguard/releases">Todas as releases</a>
</p>

O ShareGuard escolhe quais aplicativos do Windows entram no áudio de um compartilhamento de tela no navegador. O que você bloqueia continua tocando para você e sai do áudio enviado ao site. O vídeo permanece.

É gratuito e open source. O processamento fica no seu computador. Não há conta nem envio de áudio para um servidor do ShareGuard.

## Download

Baixe a última release do ShareGuard para Windows.

[Baixar o ShareGuard](https://github.com/UnkoynX777/shareguard/releases/latest)

Execute `ShareGuard-Setup-vX.Y.Z-x64.exe`. Ele instala o helper nativo e copia a extensão do Chrome e do Edge para uma pasta fixa do usuário. Depois, adicione a extensão no navegador:

Chrome · Edge · Firefox

[Guia de instalação](./docs/INSTALLATION.md)

Windows 10 ou 11, 64 bits. Chrome 116 ou mais recente, Edge atual, ou Firefox 128 ou mais recente. Os arquivos oficiais estão só nessa Release do GitHub. `SHA256SUMS.txt` serve para conferir o download. O instalador não tem assinatura de código.

## O que você ouve e o que os outros ouvem

| Aplicativo | Você ouve | Quem assiste ouve |
| --- | --- | --- |
| Discord | Sim | Não, se estiver Blocked |
| Spotify | Sim | Não, se estiver Blocked |
| Jogo | Sim | Sim, enquanto estiver Allowed |
| Navegador | Sim | Sim, enquanto estiver Allowed |

Esses nomes são exemplos. O ShareGuard não trata nenhum executável de forma especial. A lista é o que está em execução.

## Recursos

- Bloquear ou permitir cada aplicativo pelo nome do executável
- O áudio local não muda
- A lista de processos acompanha o compartilhamento
- A regra continua valendo se o programa abrir de novo com outro PID
- Um único helper para Chrome, Edge e Firefox
- Loopback de processo via WASAPI, sem cabo de áudio virtual
- Sem processamento na nuvem

## Começar

1. Baixe e instale o ShareGuard na [última release](https://github.com/UnkoynX777/shareguard/releases/latest).
2. Adicione a extensão no navegador. [Guia de instalação](./docs/INSTALLATION.md).
3. Abra o ShareGuard. **Native** deve mostrar **Connected**.
4. Ligue a proteção e marque como Blocked o que os outros não devem ouvir.
5. Inicie um compartilhamento que inclua áudio.

## Navegadores

| Navegador | Extensão | Instalação |
| --- | --- | --- |
| Google Chrome | Chromium | Manual |
| Microsoft Edge | Chromium | Manual |
| Mozilla Firefox | Firefox | XPI assinado |

Chrome e Edge usam o modo de desenvolvedor e **Load unpacked** na pasta que o instalador copia. O Firefox usa o XPI assinado da mesma release, quando esse arquivo é publicado. O Firefox ESR 115 não serve. O passo a passo está no [guia de instalação](./docs/INSTALLATION.md).

## Uso

O popup tem o interruptor de proteção, uma busca e Allow ou Blocked em cada linha. Quem está tocando aparece primeiro. Um executável bloqueado que está fechado fica em Blocked apps. Show all processes mostra o que fica oculto por não ter janela nem áudio.

Um aplicativo novo entra como Allowed até o executável ser bloqueado. Mudar a lista durante o compartilhamento reconfigura a captura. Fechar o popup não encerra o compartilhamento.

Se a proteção não puder ser garantida, o ShareGuard tira o áudio compartilhado e mantém o vídeo. Se o compartilhamento não tiver faixa de áudio, ele não inventa uma. Cancelar o seletor de tela devolve ao site o erro original do navegador.

## Como funciona

Uma extensão não captura o áudio de cada processo do Windows. O ShareGuard usa um helper pequeno e open source, `shareguard-native.exe`, para falar com o Windows Core Audio. O navegador inicia esse helper por Native Messaging. A extensão troca a faixa de áudio de `getDisplayMedia` por uma faixa criada na própria página. Os alto-falantes e o mixer do Windows não são alterados.

```mermaid
flowchart LR
  apps[Aplicativos do Windows] --> helper[Helper nativo]
  helper --> extension[Extensão]
  extension --> share[Compartilhamento]
```

O detalhe está em [Arquitetura](./docs/ARCHITECTURE.md).

## Privacidade

O ShareGuard trata nomes de aplicativos e o roteamento do áudio no seu computador. Não há conta, analytics nem telemetria. O áudio compartilhado segue para o site em que você está compartilhando, como aconteceria sem o ShareGuard, depois do filtro. Ele não é enviado a um servidor do ShareGuard.

O log do helper, quando o debug está ligado, não contém amostras de áudio.

## Compilar

Quem só vai usar o programa pode pular esta parte.

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-native.ps1
cd extension
npm ci
npm run build
```

[Compilação](./docs/BUILDING.md) · [Desenvolvimento](./docs/DEVELOPMENT.md) · [Como contribuir](./CONTRIBUTING.md)

## Problemas

Se **Native** continuar **Unavailable**, comece por [Solução de problemas](./docs/TROUBLESHOOTING.md). Vulnerabilidades vão para [SECURITY.md](./SECURITY.md), não para uma issue pública.

## Roteiro

Já existe: filtro por aplicativo, instalador por usuário no Windows e pacotes da extensão nas Releases do GitHub.

Ainda não existe: assinatura de código do instalador. O pacote do Firefox só entra na release depois que a Mozilla assina essa versão.

## Por que o ShareGuard não está nas lojas dos navegadores?

O ShareGuard é distribuído pelo GitHub. O código da extensão e os pacotes da release são públicos neste repositório.

## Licença

[MIT](./LICENSE). Criado e mantido por [UnkoynX777](https://github.com/UnkoynX777).
