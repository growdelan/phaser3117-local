/* SPDX-License-Identifier: GPL-2.0-only
 * A deliberately narrow Phaser 3117 raster filter. See docs/PROTOCOL.md.
 * New implementation; protocol behavior studied in SpliX (see NOTICE).
 */
#include <cups/raster.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef __APPLE__
#include <sandbox.h>
#else
#error This filter requires the macOS sandbox.
#endif

enum { STRIDE = 620, WIDTH = 4960, HEIGHT = 7017, TOP = 125,
       LEFT = 12, BAND_ROWS = 128, BAND_BYTES = STRIDE * BAND_ROWS,
       MAX_PAGES = 100 };
static unsigned char page[STRIDE * HEIGHT], row[STRIDE], band[BAND_BYTES];
static unsigned char packed[BAND_BYTES + BAND_BYTES / 128 + 140];

static void die(const char *message) {
    fprintf(stderr, "ERROR: %s\n", message);
    exit(1);
}
static void emit(const void *data, size_t size) {
    if (fwrite(data, 1, size, stdout) != size) die("Output write failed");
}
static void be16(unsigned char *p, unsigned value) {
    p[0] = (unsigned char)(value >> 8); p[1] = (unsigned char)value;
}
static void be32(unsigned char *p, uint32_t value) {
    p[0] = (unsigned char)(value >> 24); p[1] = (unsigned char)(value >> 16);
    p[2] = (unsigned char)(value >> 8); p[3] = (unsigned char)value;
}

/* QPDL 0x11, little-endian table, with distance 1 only.
 * Repeated-byte runs use backreferences; all other bytes use literal packets.
 * No dictionary search, variable-size allocations, or unbounded output buffer.
 */
static size_t repeats(size_t pos) {
    size_t count = 0;
    while (pos + count < BAND_BYTES && count < 514 &&
           band[pos + count] == band[pos - 1]) ++count;
    return count;
}
static size_t encode_band(void) {
    memset(packed, 0, 132);
    packed[0] = 1;                       /* One initial literal byte, LE32. */
    for (size_t i = 0; i < 64; ++i) packed[4 + 2 * i] = 1;
    packed[132] = band[0];
    size_t pos = 1, out = 133;
    while (pos < BAND_BYTES) {
        size_t n = repeats(pos);
        if (n >= 3) {
            unsigned v = (unsigned)(n - 3);
            packed[out++] = (unsigned char)(0x80 | (v & 127));
            packed[out++] = (unsigned char)((v >> 1) & 0xc0);
            pos += n;
        } else {
            size_t start = pos++;
            while (pos < BAND_BYTES && pos - start < 128 && repeats(pos) < 3)
                ++pos;
            n = pos - start;
            packed[out++] = (unsigned char)(n - 1);
            memcpy(packed + out, band + start, n); out += n;
        }
    }
    return out;
}
static void write_page(unsigned copies) {
    unsigned char h[17] = {0, 6, 0, 0, 2, 0, 0, 0, 0, 1, 0, 1, 0, 0, 2, 1, 6};
    be16(h + 2, copies); be16(h + 5, WIDTH); be16(h + 7, HEIGHT - TOP);
    emit(h, sizeof h);
    for (unsigned base = 0, number = 0; base < HEIGHT - TOP;
         base += BAND_ROWS, ++number) {
        unsigned nonwhite = 0;
        for (unsigned x = 0; x < STRIDE; ++x) {
            for (unsigned y = 0; y < BAND_ROWS; ++y) {
                unsigned char ink = 0;
                if (base + y + TOP < HEIGHT && x + LEFT < STRIDE)
                    ink = page[(base + y + TOP) * STRIDE + x + LEFT];
                nonwhite |= ink;
                band[x * BAND_ROWS + y] = (unsigned char)~ink;
            }
        }
        if (!nonwhite) continue;
        size_t n = encode_band();
        unsigned char bh[11] = {12, (unsigned char)number};
        be16(bh + 2, WIDTH); be16(bh + 4, BAND_ROWS);
        bh[6] = 0x11; be32(bh + 7, (uint32_t)n + 8);
        const unsigned char signature[] = {0xef, 0xcd, 0xab, 0x09};
        uint32_t checksum = 0xef + 0xcd + 0xab + 0x09;
        for (size_t i = 0; i < n; ++i) checksum += packed[i];
        unsigned char sum[4]; be32(sum, checksum);
        emit(bh, sizeof bh); emit(signature, sizeof signature);
        emit(packed, n); emit(sum, sizeof sum);
    }
    unsigned char end[3] = {1, 0, 0}; be16(end + 1, copies); emit(end, sizeof end);
}
static void read_page(cups_raster_t *raster, const cups_page_header2_t *h) {
    if (h->HWResolution[0] != 600 || h->HWResolution[1] != 600 ||
        h->PageSize[0] != 595 || h->PageSize[1] != 842 ||
        h->cupsBitsPerColor != 1 || h->cupsBitsPerPixel != 1 ||
        h->cupsColorSpace != CUPS_CSPACE_K || h->cupsColorOrder != CUPS_ORDER_CHUNKED ||
        h->Duplex || h->Tumble || h->Orientation ||
        h->cupsWidth == 0 || h->cupsWidth > WIDTH ||
        h->cupsHeight == 0 || h->cupsHeight > HEIGHT ||
        h->cupsBytesPerLine != (h->cupsWidth + 7) / 8)
        die("Only portrait A4, 600 dpi, 1-bit black, simplex raster is supported");
    memset(page, 0, sizeof page);
    unsigned bytes = h->cupsBytesPerLine;
    unsigned x = (STRIDE - bytes) / 2, y = (HEIGHT - h->cupsHeight) / 2;
    for (unsigned i = 0; i < h->cupsHeight; ++i) {
        if (cupsRasterReadPixels(raster, row, bytes) != bytes)
            die("Truncated raster pixels");
        if (h->cupsWidth % 8)
            row[bytes - 1] &= (unsigned char)(0xff << (8 - h->cupsWidth % 8));
        memcpy(page + (y + i) * STRIDE + x, row, bytes);
    }
}
static void confine(void) {
    char *error = NULL;
    /* Already-open stdin/input and stdout remain usable. No new paths,
     * network connections, child processes, or input-device services allowed.
     * Failure is fatal: there is no unsandboxed fallback.
     */
    if (sandbox_init("(version 1)(deny default)", 0, &error) != 0) {
        if (error) sandbox_free_error(error);
        die("Cannot activate mandatory macOS sandbox");
    }
}
int main(int argc, char **argv) {
    if (argc != 6 && argc != 7)
        die("Usage: rasterto3117 job-id user title copies options [raster-file]");
    /* User, title and options are intentionally never interpolated into PJL. */
    unsigned copies = 0;
    for (const char *p = argv[4]; *p; ++p) {
        if (*p < '0' || *p > '9' || copies > 99) die("Invalid copy count");
        copies = copies * 10 + (unsigned)(*p - '0');
    }
    if (copies < 1 || copies > 99) die("Copy count must be 1..99");
    int fd = STDIN_FILENO;
    if (argc == 7) {
        fd = open(argv[6], O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
        struct stat st;
        if (fd < 0 || fstat(fd, &st) || !S_ISREG(st.st_mode) ||
            st.st_size > 512LL * 1024 * 1024)
            die("Input must be a regular raster file, at most 512 MiB");
    }
    confine();
    cups_raster_t *raster = cupsRasterOpen(fd, CUPS_RASTER_READ);
    if (!raster) die("Cannot open CUPS raster stream");
    unsigned pages = 0;
    cups_page_header2_t header;
    while (cupsRasterReadHeader2(raster, &header)) {
        if (++pages > MAX_PAGES) die("Job exceeds 100 pages");
        read_page(raster, &header);
        if (pages == 1) {
            const char start[] = "\033%-12345X@PJL SET JAMRECOVERY=OFF\n"
                "@PJL SET DUPLEX=OFF\n@PJL SET PAPERTYPE=NORMAL\n"
                "@PJL SET DENSITY=3\n@PJL SET RET=NORMAL\n"
                "@PJL ENTER LANGUAGE = QPDL\n";
            emit(start, sizeof start - 1);
        }
        write_page(copies);
        fprintf(stderr, "PAGE: %u %u\n", pages, copies);
    }
    const char *error = cupsRasterErrorString();
    if (error && *error) die("Invalid or truncated raster header");
    if (!pages) die("Empty raster job");
    const char end[] = "\t\033%-12345X"; emit(end, sizeof end - 1);
    if (fflush(stdout)) die("Output flush failed");
    cupsRasterClose(raster);
    if (fd != STDIN_FILENO) close(fd);
    return 0;
}
