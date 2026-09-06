#!/bin/sh
set -eu
if [ "$(id -u)" != 0 ]; then echo 'Run with sudo.' >&2; exit 1; fi
if /usr/bin/lpstat -p Phaser3117_Local >/dev/null 2>&1; then
    /usr/sbin/lpadmin -x Phaser3117_Local
fi
/bin/rm -f /Library/Printers/Phaser3117Local/rasterto3117 \
    /Library/Printers/Phaser3117Local/phaser3117.ppd \
    /Library/Printers/Phaser3117Local/print-pdf.sh
if [ -d /Library/Printers/Phaser3117Local ]; then
    /bin/rmdir /Library/Printers/Phaser3117Local
fi
