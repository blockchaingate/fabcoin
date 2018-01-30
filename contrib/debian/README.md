
Debian
====================
This directory contains files used to package fabcoind/fabcoin-qt
for Debian-based Linux systems. If you compile fabcoind/fabcoin-qt yourself, there are some useful files here.

## fabcoin: URI support ##


fabcoin-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install fabcoin-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your fabcoin-qt binary to `/usr/bin`
and the `../../share/pixmaps/fabcoin128.png` to `/usr/share/pixmaps`

fabcoin-qt.protocol (KDE)

