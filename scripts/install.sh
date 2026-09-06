#!/bin/sh
# Install only a locally built filter and its PPD. Run after make test.
set -eu
if [ "$(id -u)" != 0 ]; then
    echo 'Run with sudo; see README.md for the complete build/install steps.' >&2
    exit 1
fi
if [ "$#" != 1 ]; then
    echo 'Usage: install.sh usb://Xerox/Phaser%203117?serial=YOUR_SERIAL' >&2
    exit 1
fi
case "$1" in
    usb://Xerox/Phaser%203117*) ;;
    *) echo 'Only a Xerox Phaser 3117 USB URI is accepted.' >&2; exit 1 ;;
esac
cd "$(dirname "$0")/.."
test -f build/rasterto3117
test -f phaser3117.ppd
test -f scripts/print-pdf.sh
/usr/bin/install -d -o root -g wheel -m 755 /Library/Printers/Phaser3117Local
/usr/bin/install -o root -g wheel -m 755 build/rasterto3117 /Library/Printers/Phaser3117Local/rasterto3117
/usr/bin/install -o root -g wheel -m 644 phaser3117.ppd /Library/Printers/Phaser3117Local/phaser3117.ppd
/usr/bin/install -o root -g wheel -m 755 scripts/print-pdf.sh /Library/Printers/Phaser3117Local/print-pdf.sh
/usr/sbin/lpadmin -p Phaser3117_Local -D 'Xerox Phaser 3117 (Local)' \
    -v "$1" -P phaser3117.ppd -E -o PageSize=A4 -o printer-is-shared=false
