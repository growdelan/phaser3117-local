#!/usr/bin/env python3
"""Independent QPDL decoder and adversarial integration tests (stdlib only)."""
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
FILTER = Path(os.environ.get('PHASER_TEST_FILTER', str(ROOT / 'build/rasterto3117')))

def decode(data):
    marker = b'@PJL ENTER LANGUAGE = QPDL\n'
    pos = data.index(marker) + len(marker)
    pages = []
    while data[pos:pos+1] == b'\x00':
        head = data[pos:pos+17]; pos += 17
        assert head[1] == head[16] == 6 and head[4] == 2 and head[14] == 2
        assert struct.unpack('>HH', head[5:9]) == (4960, 6892)
        bands = {}
        while data[pos:pos+1] == b'\x0c':
            tag, number, width, height, compression, size = struct.unpack('>BBHHBI', data[pos:pos+11])
            pos += 11
            block = data[pos:pos+size]; pos += size
            assert width == 4960 and height == 128 and compression == 17
            assert block[:4] == bytes.fromhex('efcdab09')
            assert sum(block[:-4]) & 0xffffffff == struct.unpack('>I', block[-4:])[0]
            payload = block[4:-4]
            initial, = struct.unpack('<I', payload[:4])
            distances = struct.unpack('<64H', payload[4:132])
            raw = bytearray(payload[132:132+initial]); k = 132+initial
            while k < len(payload):
                control = payload[k]; k += 1
                if control & 128:
                    extra = payload[k]; k += 1
                    length = (control & 127) + ((extra & 192) << 1) + 3
                    distance = distances[extra & 63]
                    assert 0 < distance <= len(raw)
                    for _ in range(length): raw.append(raw[-distance])
                else:
                    length = control + 1
                    assert k + length <= len(payload)
                    raw.extend(payload[k:k+length]); k += length
            assert len(raw) == 620 * 128 and number not in bands
            bands[number] = bytes(raw)
        assert data[pos] == 1 and data[pos+1:pos+3] == head[2:4]
        pos += 3
        pages.append(bands)
    assert data[pos:] == b'\t\x1b%-12345X'
    return pages

class DriverTests(unittest.TestCase):
    def fixture(self, kind):
        return subprocess.check_output([str(ROOT/'build/raster-fixture'), kind])
    def run_filter(self, data, copies='1', title='synthetic', extra=()):
        return subprocess.run([str(FILTER), '1', 'synthetic', title, copies, '', *extra],
                              input=data, capture_output=True, timeout=20)
    def test_pixels_packet_lengths_and_checksums(self):
        result = self.run_filter(self.fixture('pattern'))
        self.assertEqual(result.returncode, 0, result.stderr)
        pages = decode(result.stdout); self.assertEqual(len(pages), 1)
        for number, band in pages[0].items():
            expected = bytearray()
            for x in range(620):
                for y in range(128):
                    source_y = number*128+y+125
                    ink = ((x+12)*37+source_y*13+(source_y>>7)) & 255 if x+12<620 and source_y<7017 else 0
                    expected.append(ink ^ 255)
            self.assertEqual(band, expected)
        self.assertEqual(len(pages[0]), 54)
    def test_blank_page(self):
        r = self.run_filter(self.fixture('blank'))
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(decode(r.stdout), [{}])
    def test_two_pages_and_copies(self):
        r = self.run_filter(self.fixture('two'), copies='2')
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(len(decode(r.stdout)), 2)
    def test_bad_headers(self):
        for kind in ['wide', 'tall', 'stride', 'color', 'dpi', 'duplex', 'letter', 'truncated']:
            with self.subTest(kind=kind):
                r = self.run_filter(self.fixture(kind))
                self.assertNotEqual(r.returncode, 0)
                self.assertEqual(r.stdout, b'')
    def test_junk_and_truncated_headers(self):
        for data in [b'', b'not a raster', b'RaS2', b'RaS2'+b'\0'*50]:
            r = self.run_filter(data)
            self.assertNotEqual(r.returncode, 0)
            self.assertEqual(r.stdout, b'')
    def test_copy_validation(self):
        for value in ['', '0', '-1', '100', '999999999999999', '1;id']:
            self.assertNotEqual(self.run_filter(b'', copies=value).returncode, 0)
    def test_metadata_cannot_inject_pjl(self):
        data = self.fixture('blank')
        normal = self.run_filter(data)
        hostile = self.run_filter(data, title='"\n@PJL DEFAULT PASSWORD=1234\n')
        self.assertEqual(normal.stdout, hostile.stdout)
    def test_file_and_symlink(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'test.raster'; path.write_bytes(self.fixture('blank'))
            r = self.run_filter(b'', extra=(str(path),))
            self.assertEqual(r.returncode, 0, r.stderr)
            link = Path(directory)/'link'; link.symlink_to(path)
            self.assertNotEqual(self.run_filter(b'', extra=(str(link),)).returncode, 0)
    def test_sandbox_denials(self):
        for mode in ['read', 'write', 'network', 'exec']:
            with self.subTest(mode=mode):
                r = subprocess.run([str(ROOT/'build/sandbox-probe'), mode], capture_output=True)
                self.assertEqual(r.returncode, 0, r.stderr)
    def test_no_unexpected_imports(self):
        imports = subprocess.check_output(['/usr/bin/nm', '-u', str(FILTER)], text=True)
        forbidden = ['_socket', '_connect', '_exec', '_fork', '_system', '_popen',
                     '_CGEvent', '_IOHID', '_dlopen', '_getenv']
        for symbol in forbidden: self.assertNotIn(symbol, imports)

if __name__ == '__main__': unittest.main(verbosity=2)
