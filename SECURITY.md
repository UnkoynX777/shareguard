# Security

ShareGuard includes a native Windows executable, a browser extension, and a Native Messaging host. Please report vulnerabilities privately.

## Supported versions

Security fixes are considered for the latest release published at [github.com/UnkoynX777/shareguard/releases](https://github.com/UnkoynX777/shareguard/releases).

The native helper and the extension must be the same release. Mixing an older helper with a newer extension, or the reverse, can fail the protocol handshake (`PROTOCOL_VERSION_MISMATCH`).

## Reporting a vulnerability

Do not open a public GitHub issue for a security report.

There is no private email address for this project. The intended channel is GitHub private vulnerability reporting:

1. Open the repository **Security** tab.
2. Choose **Report a vulnerability**.

That button exists only after private vulnerability reporting is enabled in the repository settings. Until it is enabled, contact [UnkoynX777](https://github.com/UnkoynX777) directly and wait for a private follow-up. Do not include a proof of concept in a public issue, discussion, or pull request.

## What to include

- ShareGuard version
- Windows version and build number
- Browser and browser version
- What an attacker can do, and what they already need
- Steps to reproduce, or a minimal description if the details should stay private
- Whether the native helper, the extension, or both are involved

Remove personal data, audio recordings, and process lists that are not required to understand the bug.

## Disclosure

Please give the maintainer time to confirm the report and publish a fix before discussing it publicly. A release note will credit the report when the reporter wants to be named.
