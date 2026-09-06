# Validation record

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
