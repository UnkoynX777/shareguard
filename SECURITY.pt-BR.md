[English](./SECURITY.md) | Português (Brasil)

# Segurança

O ShareGuard inclui um executável nativo do Windows, uma extensão de navegador e um host de Native Messaging. Reporte vulnerabilidades em particular.

A versão em inglês desta página é a política canônica. Esta tradução descreve a mesma política.

## Versões suportadas

Correções de segurança são consideradas para a última release publicada em [github.com/UnkoynX777/shareguard/releases](https://github.com/UnkoynX777/shareguard/releases).

O helper nativo e a extensão precisam ser a mesma release. Misturar um helper antigo com uma extensão nova, ou o contrário, pode falhar o handshake do protocolo (`PROTOCOL_VERSION_MISMATCH`).

## Reportar uma vulnerabilidade

Não abra uma issue pública no GitHub para um relato de segurança.

Não há um e-mail privado deste projeto. O canal previsto é o relato privado de vulnerabilidade do GitHub:

1. Abra a aba **Security** do repositório.
2. Escolha **Report a vulnerability**.

Esse botão só existe depois que o relato privado de vulnerabilidade está ligado nas configurações do repositório. Até lá, fale com [UnkoynX777](https://github.com/UnkoynX777) diretamente e espere um retorno em particular. Não inclua uma prova de conceito numa issue pública, numa discussion ou num pull request.

## O que incluir

- Versão do ShareGuard
- Versão e número de build do Windows
- Navegador e versão do navegador
- O que uma pessoa atacante consegue fazer, e o que ela já precisa ter
- Passos para reproduzir, ou uma descrição mínima se o detalhe precisar ficar privado
- Se o helper nativo, a extensão ou os dois estão envolvidos

Remova dados pessoais, gravações de áudio e listas de processo que não sejam necessárias para entender o bug.

## Divulgação

Dê tempo a quem mantém o projeto para confirmar o relato e publicar uma correção antes de discutir em público. Uma nota de release credita o relato quando a pessoa quiser ser nomeada.
