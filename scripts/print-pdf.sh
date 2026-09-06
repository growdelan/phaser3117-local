#!/bin/sh
# Trusted orchestration, outside the encoder sandbox. No network tools used.
set -eu
if [ "$#" != 1 ] || [ ! -f "$1" ]; then
    echo 'Usage: print-pdf.sh /absolute/path/to/document.pdf' >&2
    exit 1
fi
case "$1" in
    /*) input=$1 ;;
    *) input=$PWD/$1 ;;
esac
base=/Library/Printers/Phaser3117Local
for item in rasterto3117 phaser3117.ppd; do
    test -f "$base/$item" || { echo 'Install the driver first.' >&2; exit 1; }
done
# Private intermediates: the encoder itself never creates files.
umask 077
work=$(/usr/bin/mktemp -d /private/tmp/phaser3117-job.XXXXXX)
trap '/bin/rm -rf "$work"' EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
/usr/sbin/cupsfilter -i application/pdf -p "$base/phaser3117.ppd" \
    -m application/vnd.cups-raster -o media=A4 "$input" \
    > "$work/page.raster" 2> "$work/render.log" || {
    echo 'PDF rendering failed; nothing submitted to the printer.' >&2
    /bin/cat "$work/render.log" >&2; exit 1;
}
"$base/rasterto3117" 1 local 'Local PDF print' 1 '' "$work/page.raster" \
    > "$work/page.qpdl"
# Only a completely converted job is sent. Raw bypasses all CUPS filters;
# the standard Apple USB backend handles device access.
/usr/bin/lp -d Phaser3117_Local -o raw -t 'Local PDF print' "$work/page.qpdl"
