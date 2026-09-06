#!/bin/sh
# Per-user installation. Do NOT run with sudo.
set -eu
if [ "$(id -u)" = 0 ]; then echo 'Run as your regular user, without sudo.' >&2; exit 1; fi
cd "$(dirname "$0")/.."
test -x /Library/Printers/Phaser3117Local/print-pdf.sh || {
    echo 'Install the driver first.' >&2; exit 1;
}
services="$HOME/Library/PDF Services"
name='Drukuj na Xerox Phaser 3117.scpt'
mkdir -p "$services"
# Compilation is local; the source remains in the repository for review.
/usr/bin/osacompile -o "$services/$name" macos/Print-on-Phaser-3117.applescript
/usr/bin/SetFile -a E "$services/$name"
printf 'Installed PDF print-menu action: %s\n' "$services/$name"
