/* SPDX-License-Identifier: GPL-2.0-only
 * Synthetic input only; never reads user documents.
 */
#include <cups/raster.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
int main(int argc, char **argv) {
    if (argc != 2) return 1;
    cups_page_header2_t h = {0};
    h.HWResolution[0] = h.HWResolution[1] = 600;
    h.PageSize[0] = 595; h.PageSize[1] = 842;
    h.cupsWidth = 4960; h.cupsHeight = 7017; h.cupsBytesPerLine = 620;
    h.cupsBitsPerColor = h.cupsBitsPerPixel = 1;
    h.cupsColorSpace = CUPS_CSPACE_K; h.cupsNumColors = 1; h.NumCopies = 1;
    if (!strcmp(argv[1], "wide")) h.cupsWidth = 0xffffffff;
    if (!strcmp(argv[1], "tall")) h.cupsHeight = 0xffffffff;
    if (!strcmp(argv[1], "stride")) h.cupsBytesPerLine = 0xffffffff;
    if (!strcmp(argv[1], "color")) h.cupsColorSpace = CUPS_CSPACE_RGB;
    if (!strcmp(argv[1], "dpi")) h.HWResolution[0] = 1200;
    if (!strcmp(argv[1], "duplex")) h.Duplex = 1;
    if (!strcmp(argv[1], "letter")) h.PageSize[0] = 612;
    cups_raster_t *r = cupsRasterOpen(STDOUT_FILENO, CUPS_RASTER_WRITE);
    unsigned pages = !strcmp(argv[1], "two") ? 2 : 1;
    for (unsigned p = 0; p < pages; ++p) {
        if (!cupsRasterWriteHeader2(r, &h)) return 2;
        if (!strcmp(argv[1], "truncated")) break;
        unsigned char line[620];
        for (unsigned y = 0; y < 7017; ++y) {
            for (unsigned x = 0; x < 620; ++x)
                line[x] = !strcmp(argv[1], "blank") ? 0 :
                    (unsigned char)((x * 37 + y * 13 + (y >> 7)) & 255);
            /* Malformed headers need no pixels. */
            if (h.cupsBytesPerLine != 620 || h.cupsWidth > 4960 ||
                h.cupsHeight > 7017) break;
            if (cupsRasterWritePixels(r, line, 620) != 620) return 3;
        }
    }
    cupsRasterClose(r); return 0;
}
