# Security scope

The goal is inspectable behavior, not a promise of "100% virus-free" software.
No independent security audit or antivirus certification has been performed.

## Two execution contexts

**Standalone:** after opening the explicit input file, the encoder activates
`(version 1)(deny default)`. That policy blocks new file access, connections,
process execution, and input-device services. Existing input/output descriptors
remain usable. The PDF helper and optional PDF menu action use this path.

**Native printing:** CUPS already places filters in a sandbox, so macOS rejects
a second `sandbox_init`. In version 0.2 the encoder accepts that existing policy
only if all three checks pass:

1. `sandbox_check(getpid(), NULL, 0)` reports an active sandbox (exactly 1).
2. Real and effective UID are both macOS `_lp` (26).
3. `proc_pidpath(getppid(), ...)` reports `/usr/sbin/cupsd`.

No environment variable, job title, or user option can select an unsandboxed
mode. Missing APIs or identity/sandbox checks cause rejection. This is an
identity/sandbox-presence check, **not a proof of every rule in the inherited
policy**. We trust Apple's protected cupsd executable and administrator-managed
CUPS configuration. CUPS's policy is broader than deny-default and may permit
some filesystem, process, IOKit and networking operations. Therefore standalone
sandbox-denial tests are not claims about native CUPS restrictions.

We do not disable CUPS sandboxing, SIP, Gatekeeper, or privacy controls. The
encoder contains no keyboard, clipboard, screen capture, network, child-process,
or dynamically loaded plugin functionality in either context. It is invoked
only for printing; no service is installed at login or boot.

## Data and privileges

The encoder reads stdin or opens the explicitly supplied raster path read-only,
rejecting symlinks and non-regular/oversized files. It writes QPDL to stdout
and fixed diagnostics/page counts to stderr. It does not read the PPD, home
directory, job metadata, or network and creates no temporary files. It checks
current/parent process identity only when standalone confinement fails.

The sandbox does not revoke inherited descriptors. CUPS or the helper is
trusted to provide appropriate input/output descriptors. Run the standalone
encoder/helper as a normal user, not with sudo. Only installation needs admin
rights. Installed driver files are root-owned and not group/world writable.

## Remaining trust

Apple's compiler, loader, C runtime, CUPS parser/renderer, sandbox implementation,
USB backend, printer firmware and CUPS configuration remain dependencies.
Sandboxing happens after the loader and explicit input open. It cannot protect
against a compromised OS, compiler or malicious administrator. Parsing in
Apple's CUPS library may allocate memory before our page validation; our limits
are not universal CPU/memory quotas. An error after one page cannot retract it.

Bounds checks, independent decoding, static analysis, sanitizers and sandbox
probes reduce specific risks; they do not prove universal safety. `sandbox_check`
is an Apple SPI not in the public SDK headers. It is weak-linked and fails
closed if absent. The identity check is specific to the tested macOS setup.

## Helper and PDF menu boundary

The optional shell helper and AppleScript menu action run as the current user,
outside the encoder sandbox. The helper quotes filenames, uses absolute Apple
tool paths, creates mode-700 temporary storage (umask 077), renders the selected
PDF, runs the encoder, and submits only successfully converted QPDL through
local CUPS to USB. Cleanup traps cannot run after a forced kill or power loss.

The locally compiled PDF action forwards the generated PDF with a shell-quoted
path. It does not request Accessibility or Input Monitoring permissions,
synthesize keystrokes, monitor events, or control other applications.

## Reporting

Use synthetic input in reports and inspect logs before posting. Never include
personal documents or credentials. Use private vulnerability reporting if
available, or open a minimal issue requesting a private contact without exploit
details or sensitive data.

References:
- https://apple.github.io/cups/doc/api-filter.html
- https://github.com/apple/cups/blob/master/scheduler/process.c
- https://github.com/WebKit/WebKit/blob/main/Source/WTF/wtf/spi/darwin/SandboxSPI.h
