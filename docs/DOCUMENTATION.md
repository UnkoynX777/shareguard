English | [Português (Brasil)](./DOCUMENTATION.pt-BR.md)

# Documentation

## Documentation policy

English is the canonical documentation language.

Every user-facing or contributor-facing Markdown document must have an official Brazilian Portuguese translation.

The English file is the source of truth. The `.pt-BR.md` file is a first-class translation of that same document, not a summary and not an older copy.

## Primary language

Write the change in English first. Then update the Brazilian Portuguese file in the same pull request, before merge.

If a sentence exists only in Portuguese, add it to the English file as well. The two files have to carry the same information.

## Translation requirements

The Portuguese text should sound natural in Brazilian Portuguese. Keep commands, paths, file names, API names, JSON, registry keys, and URLs unchanged.

Technical terms stay in English when that is how the field uses them. Examples: Native Messaging, WASAPI, WebExtensions, MediaStream, AudioWorklet, Manifest V3, GitHub Actions, Pull Request.

UI strings shown by ShareGuard, Chrome, Edge, or Firefox stay in backticks exactly as the product shows them. Explain them in Portuguese around the literal text.

## File naming

Keep translations beside the English file. Do not add `docs/en/` or `docs/pt-BR/` folders.

```text
docs/INSTALLATION.md
docs/INSTALLATION.pt-BR.md
```

The suffix is `.pt-BR.md`. Do not use `-BR`, `-PT`, or `.portuguese`.

A new document such as `docs/AUDIO_PIPELINE.md` needs `docs/AUDIO_PIPELINE.pt-BR.md` before merge when the page is for users or contributors.

## Language switcher

The first lines of an English document:

```text
English | [Português (Brasil)](./INSTALLATION.pt-BR.md)
```

The first lines of the Portuguese document:

```text
[English](./INSTALLATION.md) | Português (Brasil)
```

Use the real file name. The current language is plain text. The other language is a link. Do not add a second, louder switcher.

## Keeping translations synchronized

A change to install steps, build steps, errors, versions, or behavior updates both files. Do not leave the Portuguese page on an older procedure.

`CHANGELOG.md` is canonical. `CHANGELOG.pt-BR.md` follows the same releases and does not add entries of its own.

## Code examples

Code blocks are identical in both languages. That includes shell, PowerShell, JSON, C++, and TypeScript. Translate the prose around the block, not the block.

## Technical terminology

Headings are translated. GitHub builds a different anchor from a Portuguese heading, so a Portuguese page links to its own headings. Do not reuse an English anchor that the Portuguese page does not have.

## Links

English pages link to English pages. Portuguese pages link to the `.pt-BR.md` page when it exists. Do not add a link to a translation that has not been written yet.

External URLs stay as they are. `LICENSE` stays the official English text. A Portuguese page may link to it and say that the file is the official license.

## Images

Share images that have no embedded text. Screenshots of the English UI can stay in English. The Portuguese page explains what to click. Do not duplicate an image only to translate a caption that already lives in the Markdown.

## Contribution rules

[CONTRIBUTING.md](../CONTRIBUTING.md) requires the matching `.pt-BR.md` update in the same pull request. The pull request template asks: "Documentation translations were updated when applicable."

GitHub Issue Forms stay in English. People can fill them in Portuguese. The pull request template stays in English. Release notes on GitHub stay in English. Those are not a second copy of the Markdown docs.

## Exceptions

`LICENSE` is the MIT license. Do not add `LICENSE.pt-BR`. Explaining the license in these docs does not replace that file.

`.github/PULL_REQUEST_TEMPLATE.md` stays in English.

Third-party text is not rewritten. `CODE_OF_CONDUCT.pt-BR.md` is the official Contributor Covenant 2.1 Brazilian Portuguese translation from [contributor-covenant.org](https://www.contributor-covenant.org/pt-br/version/2/1/code_of_conduct/), with the same project contact used in `CODE_OF_CONDUCT.md`. Do not replace that translation with a new one.

Generated reports, temporary notes, and machine-generated artifacts are out of scope. Do not use that exception to skip a page people are expected to read.

## Validation

`scripts/check-docs.mjs` fails when a required `.pt-BR.md` file is missing, when the language line is missing, or when a relative link points at a file that does not exist. It does not decide whether the translation is accurate.

```text
node ./scripts/check-docs.mjs
```

Continuous integration runs that command on every push to `main` and on pull requests.

## Index

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
