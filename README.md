# Phaser 3117 Local

A small, source-built macOS driver for the **Xerox Phaser 3117 over USB**.
**Command-P → Xerox Phaser 3117 (Local) → Print** is supported as of version 0.2.
The encoder converts Apple's CUPS raster into QPDL/SPL for **A4, 600 dpi,
single-sided black-and-white printing**.

This is an experimental community driver, not an official Xerox product.
It was written with AI assistance and tested on an Apple Silicon Mac running
macOS 26.6.2. See [validation results](docs/VALIDATION.md) for the exact tests.

## Security approach

- One small C source file using Apple system libraries only.
- No downloaded executables, package dependencies, telemetry, updates,
  background agents, keyboard/clipboard/screen capture, or network code.
- Standalone use establishes a deny-by-default macOS sandbox.
- Native CUPS use accepts an inherited sandbox only after verifying the kernel
  reports a sandbox, the real/effective UID is macOS `_lp` (26), and the parent
  executable is `/usr/sbin/cupsd`. Environment variables cannot enable this mode.
- **CUPS's policy is broader than our standalone deny-default policy.** These
  two modes are not equivalent. We do not disable or change CUPS sandbox settings.
- Fixed-size encoder buffers, strict raster validation, and bounded copies/pages.
- Usernames, titles, and option strings are never interpolated into PJL commands.

These properties do not prove the absence of vulnerabilities. There has been
no independent audit or antivirus certification. Read [SECURITY.md](SECURITY.md)
for the trust boundaries and remaining limitations.

## Build and test

Requires macOS, Apple's Command Line Tools, `make`, and Python 3 for tests.
No Homebrew packages or downloads are used during build or installation.

```sh
make
make test
make sanitize
make analyze
```

The tests independently decode output pixels and checksums, exercise malformed
input and metadata injection, verify standalone sandbox denials, and reject
an unrelated inherited sandbox. Fixtures are synthetic.

## Install

Review the source and `scripts/install.sh`. Find the printer's USB URI:

```sh
lpinfo -v
```

Use the exact `usb://Xerox/Phaser%203117?...` URI from your Mac:

```sh
sudo /bin/sh scripts/install.sh 'usb://Xerox/Phaser%203117?serial=YOUR_SERIAL'
```

The installer creates queue `Phaser3117_Local` and three root-owned files under
`/Library/Printers/Phaser3117Local/`: `rasterto3117`, `phaser3117.ppd`, and the
optional `print-pdf.sh` helper. Sharing is disabled. The script does not change
your default printer. It can also update an existing installation.

If macOS prevents administrator access to a Documents/Desktop checkout, copy
the project and locally built binary to a temporary folder and run the same
installer there. Do not disable SIP or Gatekeeper.

## Print normally

1. Open a document and press **Command-P**.
2. Select **Xerox Phaser 3117 (Local)** and **A4**.
3. Click the ordinary **Print** button.

Close and reopen an existing print dialog after updating the driver. Applications
with a custom print preview may require choosing their system print dialog.

You can also submit a PDF without any special raw options:

```sh
lp -d Phaser3117_Local -o media=A4 your-document.pdf
lpstat -t
```

macOS renders the document, our filter converts the raster under CUPS's
inherited sandbox, and Apple's USB backend sends the result to the printer.
A completed software job alone does not prove correct physical output.

## Optional standalone and PDF-menu paths

The earlier helper remains available and runs the encoder under its stricter
standalone policy before submitting already-converted data with `lp -o raw`:

```sh
/Library/Printers/Phaser3117Local/print-pdf.sh /absolute/path/to/document.pdf
```

It uses private temporary files and removes them on normal exit or failure.
A forced kill or power loss may leave `/private/tmp/phaser3117-job.*` files.

Optionally install a per-user action, without sudo:

```sh
/bin/sh scripts/install-pdf-service.sh
```

It appears as **Command-P → PDF → Drukuj na Xerox Phaser 3117** ("Print on Xerox
Phaser 3117"). It is no longer required for normal printing. This helper path
prints one copy; print-dialog copy counts are not forwarded by the PDF action.

## Uninstall

```sh
sudo /bin/sh scripts/uninstall.sh
```

If installed, also remove the per-user menu action without sudo:

```sh
rm "$HOME/Library/PDF Services/Drukuj na Xerox Phaser 3117.scpt"
```

## Limitations

Only Phaser 3117, A4, 600 dpi, monochrome, simplex raster is targeted. Other
models/media/resolutions and duplex are unsupported. Jobs are limited to 100
pages and 1–99 copies; named raster inputs are limited to 512 MiB. A later bad
page cannot retract earlier output. CUPS raster parsing occurs in Apple's
library, which can allocate memory before our validation. The limits are not
complete resource quotas on Apple libraries or an unbounded stdin stream.

The native sandbox check uses an Apple SPI and macOS's `_lp` identity. Future
OS changes may require updates; failed checks stop printing. The standalone
sandbox API and classic PPD drivers are also deprecated by Apple/CUPS.

## Provenance and license

This is a new reduced implementation, **not clean-room code**. SpliX was
studied to understand the wire format and model constants. No SpliX executable,
object file, or driver source file is bundled. Our encoder uses a fixed
run/literal strategy instead of adaptive dictionary search. See [NOTICE](NOTICE)
and [protocol notes](docs/PROTOCOL.md).

GPL-2.0-only; see [LICENSE](LICENSE). Xerox and Phaser are trademarks of their
respective owners. No affiliation or endorsement is implied.
