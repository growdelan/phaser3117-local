# Validation record

Earlier sections describe versions 0.1 and the PDF-menu workaround. The native
printing section at the end records the current version 0.2 behavior.

Local validation date: 2026-09-06.
Host: Apple Silicon arm64, macOS 26.6.2. This is one host, not a compatibility
matrix or an independent audit.

- Production build uses Apple Clang with warnings treated as errors and stack
  protection. No external build dependencies or downloads.
- `make test`: 10 tests passed, including independent pixel reconstruction,
  band lengths/checksums, blank/multipage output, malformed raster, copy limits,
  metadata injection resistance, symlink rejection, and binary import checks.
- Sandbox probes confirmed denial of opening `/etc/passwd`, creating a file,
  connecting to loopback TCP, and executing another program. Creating an empty
  socket alone is permitted by macOS; the connection is denied.
- Clang static analyzer: no diagnostics.

- All 10 tests also passed under AddressSanitizer and UndefinedBehaviorSanitizer
  (`detect_leaks=0`); no sanitizer diagnostics. Leak detection is not claimed.
- A local 11-page comparison with previously generated reference output
  reconstructed identical band pixels. No reference executable was installed
  or bundled in this project.
- `cupstestppd`: passed. The installed encoder SHA-256 matched the local build.
- Direct CUPS-filter integration failed closed because macOS rejects a second
  sandbox. The supported path is now PDF helper → sandboxed encoder → raw job
  submission to a PPD-backed CUPS queue → Apple USB backend.
- A one-page local PDF was submitted through that final path. IPP reported
  `job-state=completed`, `job-impressions-completed=1`, and printer reason `none`.
- Physical paper appearance: the user confirmed that the second attempt
  printed correctly through the PDF-helper path, including Polish characters.
  This is one confirmed one-page hardware test, not broad device certification.
No user's document content, USB serial number, account paths, or test print PDF
is committed to this repository.

## Command-P integration (2026-09-06)

- Locally compiled the PDF service and decompiled it to verify the installed
  script matches the readable source.
- Opened a one-page PDF in Preview, pressed Command-P, and verified the custom
  action appears in the PDF menu.
- Selected the action in that actual print dialog. CUPS reported a completed
  job, one impression, and no printer-state error. Physical paper appearance
  for this additional integration test has not been independently verified.
- The encoder binary and sandbox policy were not modified for this integration.

## Native Print button, version 0.2 (2026-09-06)

- 11 tests passed on the normal build and under ASan/UBSan (leak detection off).
- Added rejection test: an unrelated inherited sandbox cannot enable native mode.
- Static analyzer: no diagnostics. Installed binary matches the recorded SHA-256.
- Opened the one-page PDF in Preview, pressed Command-P, selected
  Xerox Phaser 3117 (Local), and clicked the ordinary Print button.
- The encoder accepted the verified `_lp`/cupsd sandbox context and produced a
  page through Apple's standard renderer and USB backend.
- The first native job was marked complete by CUPS, but the user reported no
  paper output. That was a failed physical test, not success.
- A temporary diagnostic build captured the native QPDL locally. It contained
  the expected nonblank bands; no user data or diagnostic binary is published.
- Added explicit `cupsFilter2` output type `application/vnd.xerox-qpdl`, avoiding
  the implicit PostScript type used by legacy filter declarations.
- The resulting native job completed with one impression and no printer error.
  After a printer power cycle to clear the previous failed session, the actual
  Preview Command-P → Print test completed with one impression and no errors.
  The user confirmed the physical page printed correctly.
- The temporary capture executable was removed; only the production encoder,
  PPD and optional helper remain installed.
- CUPS's inherited policy is broader than the standalone deny-default policy;
  native mode does not claim standalone sandbox-denial test guarantees.
