# Phaser 3117 Local

A small, source-built macOS raster encoder and PDF print helper for the
**Xerox Phaser 3117 over USB**.
It converts monochrome CUPS raster into the printer's QPDL/SPL stream. The
implementation intentionally supports only **A4, 600 dpi, portrait raster,
single-sided black-and-white printing**.

This is an experimental community driver, not an official Xerox product.
It was written with AI assistance and tested on an Apple Silicon Mac running
macOS 26.6.2. See [validation results](docs/VALIDATION.md) for the distinction
between software tests, USB delivery, and physical print verification.

## Security approach

- One C source file; Apple system CUPS and C runtime libraries only.
- No downloaded executables, package dependencies, install-time downloads,
  telemetry, update checker, background agent, or kernel extension.
- No keyboard, clipboard, screen-capture, network, or child-process API calls
  in the filter.
- A mandatory macOS **deny-by-default sandbox** is activated after opening the
  explicitly supplied input file. Failure to activate it aborts the job.
- Fixed-size buffers and strict raster dimensions, color, resolution, copy,
  and page-count limits. No document cache or temporary files in the filter.
- Usernames, job titles, and option strings are not put into printer commands.
- The build and installation scripts are short and included for review.

**These properties do not prove the absence of vulnerabilities.** This project
has not received an independent security audit. A new implementation is not
automatically safer than a mature driver. Read [SECURITY.md](SECURITY.md) for
what is and is not covered, including dependencies and inherited descriptors.

## Build and test

Requires macOS, Apple's Command Line Tools (including Clang and CUPS headers),
`make`, and Python 3 for tests. The build does not use Homebrew or fetch code.

```sh
make
make test
```

`make test` checks decoded pixels, packet checksums, blank and multipage jobs,
malformed input, invalid copies, metadata injection, symlinks, binary imports,
and actual sandbox denials. All fixtures are synthetic.

## Why a PDF helper instead of Command-P?

On the tested macOS version, CUPS already sandboxes its filters. macOS rejects
our attempt to apply a second sandbox inside that environment. We deliberately
keep the strict sandbox requirement: the helper runs our encoder **before**
submitting the job to CUPS. Only completely converted QPDL is submitted with
`lp -o raw`; Apple’s USB backend then sends it to the printer.

**Direct printing from application print dialogs is not supported by this
version.** Export to PDF and use the helper below. The PPD-backed queue is a
transport endpoint because macOS rejects creation of truly raw queues; raw
job submission to this queue is supported. Submitting a PDF directly to that
queue fails closed when the encoder cannot apply its sandbox.

## Install

Review `src/rasterto3117.c` and `scripts/install.sh` first. Find the printer URI:

```sh
lpinfo -v
```

Then use the exact `usb://Xerox/Phaser%203117?...` URI shown on your Mac:

```sh
sudo /bin/sh scripts/install.sh 'usb://Xerox/Phaser%203117?serial=YOUR_SERIAL'
```

The installer creates only:

- `/Library/Printers/Phaser3117Local/rasterto3117` (root-owned, mode 755)
- `/Library/Printers/Phaser3117Local/print-pdf.sh` (root-owned, mode 755)
- `/Library/Printers/Phaser3117Local/phaser3117.ppd` (root-owned, mode 644)
- CUPS queue `Phaser3117_Local`, with its PPD and A4 settings

It does not enable printer sharing or change your default printer. If macOS
blocks administrator access to a Documents/Desktop checkout, copy this project
and the locally built binary to a temporary directory and run the same script
there. Never disable SIP or Gatekeeper to use this project.

## Print

Export your document to PDF, then run:

```sh
/Library/Printers/Phaser3117Local/print-pdf.sh /absolute/path/to/your-document.pdf
lpstat -t
lpstat -W completed -o
```

The helper uses Apple’s `cupsfilter` to render the PDF, then our sandboxed
encoder for raster-to-printer conversion. Private temporary raster/QPDL files
are removed by a shell trap after submission or failure. A forced kill or power
loss may leave them in `/private/tmp/phaser3117-job.*`.
For Markdown, export a PDF first. A CUPS job marked completed means the job
was handed off successfully; inspect the paper to confirm the actual result.

## Uninstall

```sh
sudo /bin/sh scripts/uninstall.sh
```

## Limitations

Only Phaser 3117 is targeted. Other Xerox/Samsung models, color raster,
non-A4 media, duplex, other resolutions, and direct PDF/Markdown input to the
filter are unsupported. Jobs are limited to 100 pages and 1–99 copies; named
input files are limited to 512 MiB. A later malformed page aborts the job but
cannot retract pages already emitted. CUPS and the printer handle USB transport.
Classic PPD filters and Apple's deprecated sandbox API may stop working in
future macOS versions; this filter fails closed if its sandbox cannot start.

## Provenance and license

This is a new, reduced implementation, **not a clean-room implementation**:
SpliX source was studied to understand the undocumented wire format and model
constants. No SpliX executable, object file, or driver source file is bundled.
The new encoder uses a fixed distance-one run/literal strategy instead of
SpliX's adaptive dictionary search. See [NOTICE](NOTICE) and
[protocol notes](docs/PROTOCOL.md).

GPL-2.0-only; see [LICENSE](LICENSE). Xerox and Phaser are trademarks of their
respective owners. No affiliation or endorsement is implied.
