[English](./DOCUMENTATION.md) | Português (Brasil)

# Documentação

## Política de documentação

O inglês é o idioma canônico da documentação.

Todo documento Markdown voltado a usuários ou contribuidores deve possuir uma tradução oficial para português brasileiro.

O arquivo em inglês é a fonte. O arquivo `.pt-BR.md` é a tradução oficial desse mesmo documento, não um resumo e não uma cópia antiga.

## Idioma principal

Escreva a alteração em inglês primeiro. Depois atualize o arquivo em português brasileiro no mesmo pull request, antes do merge.

Se uma frase existir só em português, ela também entra no arquivo em inglês. Os dois arquivos precisam trazer a mesma informação.

## Requisitos da tradução

O texto em português deve soar natural em português brasileiro. Comandos, caminhos, nomes de arquivo, nomes de API, JSON, chaves de registro e URLs permanecem iguais.

Termos técnicos ficam em inglês quando a área usa assim. Exemplos: Native Messaging, WASAPI, WebExtensions, MediaStream, AudioWorklet, Manifest V3, GitHub Actions, Pull Request.

Textos que o ShareGuard, o Chrome, o Edge ou o Firefox mostram na tela ficam entre crases, exatamente como aparecem. A explicação ao redor é em português.

## Nomes de arquivo

A tradução fica ao lado do arquivo em inglês. Não crie pastas `docs/en/` nem `docs/pt-BR/`.

```text
docs/INSTALLATION.md
docs/INSTALLATION.pt-BR.md
```

O sufixo é `.pt-BR.md`. Não use `-BR`, `-PT` nem `.portuguese`.

Um documento novo, como `docs/AUDIO_PIPELINE.md`, precisa de `docs/AUDIO_PIPELINE.pt-BR.md` antes do merge quando a página for para usuários ou contribuidores.

## Seletor de idioma

As primeiras linhas de um documento em inglês:

```text
English | [Português (Brasil)](./INSTALLATION.pt-BR.md)
```

As primeiras linhas do documento em português:

```text
[English](./INSTALLATION.md) | Português (Brasil)
```

Use o nome real do arquivo. O idioma atual é texto simples. O outro idioma é um link. Não acrescente um segundo seletor mais chamativo.

## Manter as traduções sincronizadas

Uma mudança em instalação, compilação, erros, versões ou comportamento atualiza os dois arquivos. Não deixe a página em português com um procedimento antigo.

`CHANGELOG.md` é canônico. `CHANGELOG.pt-BR.md` acompanha as mesmas releases e não inventa entradas próprias.

## Exemplos de código

Blocos de código são iguais nos dois idiomas. Isso inclui shell, PowerShell, JSON, C++ e TypeScript. Traduza o texto ao redor do bloco, não o bloco.

## Terminologia técnica

Os títulos são traduzidos. O GitHub gera uma âncora diferente a partir de um título em português, então a página em português aponta para os próprios títulos. Não reutilize uma âncora em inglês que a página em português não tem.

## Links

Páginas em inglês apontam para páginas em inglês. Páginas em português apontam para o `.pt-BR.md` quando ele existe. Não crie link para uma tradução que ainda não foi escrita.

URLs externas permanecem como estão. `LICENSE` continua sendo o texto oficial em inglês. Uma página em português pode apontar para ele e dizer que aquele arquivo é a licença oficial.

## Imagens

Compartilhe imagens sem texto embutido. Capturas da interface em inglês podem continuar em inglês. A página em português explica o que clicar. Não duplique uma imagem só para traduzir uma legenda que já está no Markdown.

## Regras de contribuição

[Como contribuir](../CONTRIBUTING.pt-BR.md) pede a atualização do `.pt-BR.md` correspondente no mesmo pull request. O modelo de pull request inclui: "Documentation translations were updated when applicable."

Os Issue Forms do GitHub ficam em inglês. Dá para preenchê-los em português. O modelo de pull request fica em inglês. As notas de release no GitHub ficam em inglês. Isso não é uma segunda cópia da documentação Markdown.

## Exceções

`LICENSE` é a licença MIT. Não crie `LICENSE.pt-BR`. Explicar a licença nesta documentação não substitui aquele arquivo.

`.github/PULL_REQUEST_TEMPLATE.md` fica em inglês.

Texto de terceiros não é reescrito. `CODE_OF_CONDUCT.pt-BR.md` é a tradução oficial em português brasileiro do Contributor Covenant 2.1, publicada em [contributor-covenant.org](https://www.contributor-covenant.org/pt-br/version/2/1/code_of_conduct/), com o mesmo contato do projeto usado em `CODE_OF_CONDUCT.md`. Não troque essa tradução por outra.

Relatórios gerados, notas temporárias e artefatos produzidos por máquina ficam de fora. Não use essa exceção para pular uma página que as pessoas devem ler.

## Validação

`scripts/check-docs.mjs` falha quando falta um `.pt-BR.md` obrigatório, quando falta a linha de idioma ou quando um link relativo aponta para um arquivo que não existe. Ele não decide se a tradução está boa.

```text
node ./scripts/check-docs.mjs
```

A integração contínua roda esse comando em todo push para `main` e em pull requests.

## Índice

| English | Português (Brasil) |
| --- | --- |
| [README](../README.md) | [README](../README.pt-BR.md) |
| [Installation](./INSTALLATION.md) | [Instalação](./INSTALLATION.pt-BR.md) |
| [Building](./BUILDING.md) | [Compilação](./BUILDING.pt-BR.md) |
| [Development](./DEVELOPMENT.md) | [Desenvolvimento](./DEVELOPMENT.pt-BR.md) |
| [Architecture](./ARCHITECTURE.md) | [Arquitetura](./ARCHITECTURE.pt-BR.md) |
| [Troubleshooting](./TROUBLESHOOTING.md) | [Solução de problemas](./TROUBLESHOOTING.pt-BR.md) |
| [Contributing](../CONTRIBUTING.md) | [Como contribuir](../CONTRIBUTING.pt-BR.md) |
| [Security](../SECURITY.md) | [Segurança](../SECURITY.pt-BR.md) |
| [Support](../SUPPORT.md) | [Suporte](../SUPPORT.pt-BR.md) |
| [Changelog](../CHANGELOG.md) | [Registro de alterações](../CHANGELOG.pt-BR.md) |
| [Code of Conduct](../CODE_OF_CONDUCT.md) | [Código de conduta](../CODE_OF_CONDUCT.pt-BR.md) |
| [Native helper](../native/README.md) | [Helper nativo](../native/README.pt-BR.md) |
| [Documentation](./DOCUMENTATION.md) | [Documentação](./DOCUMENTATION.pt-BR.md) |
