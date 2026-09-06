# Protocol notes

The Phaser 3117 is a host-rendered monochrome printer. These interoperability
facts were learned from SpliX's `qpdl.cpp`, `algo0x11.cpp`, `compress.cpp`,
`document.cpp`, and the Phaser 3117 PPD. They are not a vendor specification.

- A PJL envelope selects QPDL. The envelope here is constant and contains no
  user-controlled text, account name, filename, or persistent device settings.
- The page record is 17 bytes: opcode 0, vertical dpi/100, copies (BE16), paper
  code 2 (A4), width/height (BE16), automatic source 1, model/duplex constants,
  QPDL version 2, and horizontal dpi/100.
- A4 uses a 4960-pixel byte-aligned width, 7017 rows before the 125-row top
  hardware margin. The horizontal hardware margin is 12 bytes (96 pixels).
  CUPS raster is centered in that fixed page, then hardware margins are removed.
- Each band has 128 rows and 620 bytes per row. Bytes are transposed to
  column-major order and inverted. Wholly white bands are omitted but band
  numbers retain their physical position.
- A band record starts with 0x0c, index, BE16 width/height, compression 0x11,
  BE32 payload size. The payload begins `ef cd ab 09`, then compressed bytes,
  then a BE32 additive checksum of the signature and compressed bytes.
- Compression starts with an LE32 initial-byte count and 64 LE16 distances.
  This encoder sets every distance to 1 and starts with one raw byte.
- Literal packets have a 0..127 count byte followed by count+1 literals.
  A backreference has its high bit set; length is
  `(byte0 & 127) + ((byte1 & 192) << 1) + 3`, and the low six bits of byte1
  select a distance. This encoder only uses repeated-byte runs of 3..514.
- End of page: opcode 1 and BE16 copies. End of job: tab followed by the PJL
  universal exit sequence ESC `%-12345X`.

The tests decode this structure independently and compare every reconstructed
byte with an independently generated synthetic page. Compression is a new
simplified implementation, but the format necessarily matches the printer's
existing protocol.

References:
- https://github.com/OpenPrinting/splix
- https://github.com/janrueth/splix-2.0.0-macos
