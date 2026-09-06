# Security scope

The goal is inspectable behavior, not a claim of "100% virus-free" software.
There has been no independent audit and no antivirus certification.

## Data access

The filter opens at most the raster path supplied as its final argument,
read-only, rejecting symlinks and non-regular files. Otherwise it uses stdin.
It then activates `(version 1)(deny default)` through Apple's sandbox API,
with no opt-out. It writes printer bytes to stdout and fixed diagnostics/page
counts to stderr. It does not read the PPD, job metadata, home directory,
keyboard, clipboard, or network. It does not create files or spawn processes.
There is no service installed at login or boot.

The sandbox does not revoke already-open descriptors. stdin/stdout/stderr and
the input descriptor are intentionally retained. The supplied PDF helper invokes the encoder as the current user with only
its standard streams and explicit input. The encoder is deliberately run
outside CUPS to avoid macOS rejecting a second sandbox. Do not invoke the
encoder or PDF helper with sudo. The installer, not the encoder,
needs administrator privileges to write the driver directory and configure CUPS.

## Remaining trust and limitations

Apple's OS, compiler, loader, C runtime, CUPS raster parser and PDF renderer,
USB backend, and printer firmware remain dependencies. The sandbox is applied
inside `main`, after the dynamic loader and explicit input open, and cannot
protect against a compromised toolchain or OS. CUPS raster parsing can allocate
memory before our page validation. Input limits are not a total CPU/memory
quota for those system libraries or an unbounded stdin stream.

Bounds checks, an independent decoder, a static analyzer, sanitizer checks,
and sandbox probes reduce specific risks; they do not prove universal safety.
No user documents are required in bug reports. Please reproduce problems with
synthetic input and inspect logs before posting them publicly.

If an issue may have security impact, use the repository's private vulnerability
reporting feature if enabled. Otherwise open a minimal issue asking for a
private contact, without exploit details, credentials, or personal documents.

## PDF helper boundary

The small shell helper is not itself confined by the encoder sandbox. It uses
absolute paths to Apple tools, quotes filenames, renders the explicitly chosen
PDF, stores intermediates in a mode-700 temporary directory (umask 077), invokes
the sandboxed encoder, then submits only successfully converted output with
`lp -o raw`. It calls no download or network tools; CUPS communicates locally
and the configured backend uses USB. This orchestration and Apple's renderer
are part of the trusted computing base. Temporary files are normally removed;
a force kill or power failure can prevent cleanup.

The queue is PPD-backed because macOS rejects raw queue creation, but the
helper submits raw jobs to bypass CUPS filters. Ordinary PDF submission is
unsupported and fails at the sandbox check. No CUPS sandbox is disabled.
