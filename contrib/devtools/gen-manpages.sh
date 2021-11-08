#!/bin/sh

TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
SRCDIR=${SRCDIR:-$TOPDIR/src}
MANDIR=${MANDIR:-$TOPDIR/doc/man}

FABCOIND=${FABCOIND:-$SRCDIR/fabcoind}
FABCOINCLI=${FABCOINCLI:-$SRCDIR/fabcoin-cli}
FABCOINTX=${FABCOINTX:-$SRCDIR/fabcoin-tx}
FABCOINQT=${FABCOINQT:-$SRCDIR/qt/fabcoin-qt}

[ ! -x $FABCOIND ] && echo "$FABCOIND not found or not executable." && exit 1

# The autodetected version git tag can screw up manpage output a little bit
FABVER=($($FABCOINCLI --version | head -n1 | awk -F'[ -]' '{ print $6, $7 }'))

# Create a footer file with copyright content.
# This gets autodetected fine for fabcoind if --version-string is not set,
# but has different outcomes for fabcoin-qt and fabcoin-cli.
echo "[COPYRIGHT]" > footer.h2m
$FABCOIND --version | sed -n '1!p' >> footer.h2m

for cmd in $FABCOIND $FABCOINCLI $FABCOINTX $FABCOINQT; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${FABVER[0]} --include=footer.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${FABVER[1]}//g" ${MANDIR}/${cmdname}.1
done

rm -f footer.h2m
