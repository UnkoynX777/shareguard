[English](./CONTRIBUTING.md) | Português (Brasil)

# Como contribuir

O ShareGuard é gratuito e open source sob a [licença MIT](./LICENSE). Criado e mantido por [UnkoynX777](https://github.com/UnkoynX777). O arquivo [LICENSE](./LICENSE) é o texto oficial e permanece em inglês.

## No que trabalhar

- Relatos de bug e pedidos de recurso usam os [formulários de issue](https://github.com/UnkoynX777/shareguard/issues). Dá para preenchê-los em português.
- Relatos de segurança seguem [Segurança](./SECURITY.pt-BR.md). Não abra isso como issue pública.
- O comportamento com outras pessoas segue o [Código de conduta](./CODE_OF_CONDUCT.pt-BR.md).

## Requisitos

- Windows 10 ou 11, 64 bits
- Visual Studio 2022 com a carga de trabalho Desktop development with C++ e o Windows SDK
- Node.js 22 ou mais recente, só para compilar e testar a extensão
- Inno Setup 6.7, só para compilar o instalador

O helper nativo compila com MSVC. O CMake recusa outros compiladores.

## Repositório

```text
native/        helper em C++20
extension/     código da WebExtension e manifestos dos navegadores
installer/     script do Inno Setup
scripts/       build, conferência de versão, registro do host de desenvolvimento
docs/          instalação, compilação, arquitetura, desenvolvimento, solução de problemas, documentação
```

## Preparar o ambiente

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-native.ps1
cd extension
npm ci
npm run typecheck
npm test
npm run build
npm run lint:firefox
```

O detalhe está em [Compilação](./docs/BUILDING.pt-BR.md) e [Desenvolvimento](./docs/DEVELOPMENT.pt-BR.md).

## Versão

`VERSION` é a fonte da verdade. `scripts/apply-version.ps1` copia esse valor para o manifesto, o package, o projeto CMake, a versão do protocolo nativo e o instalador. `scripts/check-version.ps1` falha quando eles divergem.

Tags de release têm a forma `v0.3.0` e precisam bater com esse número. `v0.3.0-beta.1` é uma pre-release do mesmo `VERSION`.

## Estilo

Siga o arquivo que você está editando. Fontes C++ e TypeScript neste repositório não usam comentários. Explique a mudança no pull request. Não acrescente ramos específicos de um site, como um caso especial de Google Meet ou Discord. Não acrescente telemetria.

## Fluxo

1. Faça um fork do repositório.
2. Crie um branch: `feat/...`, `fix/...`, `docs/...` ou `refactor/...`.
3. Faça a mudança e rode a build e os testes que a cobrem.
4. Abra um pull request contra `main`.

Os commits não precisam seguir Conventional Commits. Uma frase curta que diga por que a mudança existe basta.

O modelo de pull request fica em inglês. O checklist inclui a linha "Documentation translations were updated when applicable."

## Pull requests

Diga o que mudou, por quê e como você testou. Atualize a documentação quando os passos de instalação ou de compilação mudarem. Atualize o `.pt-BR.md` correspondente na mesma alteração. Atualize `CHANGELOG.md` e `CHANGELOG.pt-BR.md` em `Unreleased` quando a mudança for visível para quem usa o programa.

## Documentação

O inglês é o idioma canônico. O português brasileiro é a tradução oficial.

Ao alterar documentação em inglês, atualize também o arquivo `.pt-BR.md` correspondente. Ao alterar a documentação em português, confira se a versão canônica em inglês também precisa mudar. Um arquivo Markdown novo, voltado a usuários ou contribuidores, precisa do `.pt-BR.md` antes do merge. As regras e as exceções estão em [Documentação](./docs/DOCUMENTATION.pt-BR.md).

`scripts/check-docs.mjs` confere se cada tradução obrigatória existe, se a linha de idioma está presente e se os links relativos apontam para arquivos que existem. Ele não julga o texto da tradução.
