# hest
Fullscreen window manager

download and unzip hest, type at command prompt to compile:
$ cd ~/hest-master
$ aclocal
$ autoupdate
$ autoconf
$ automake --add-missing
$ ./configure
$ make
$ make install

add hest to ~/.xinitrc or create a /usr/share/xsessions/hest.desktop file.

to use:
Mod4-Control - Show an overview of the windows currently opened (while held).
Mod4-[a-z] - switch to an active window.
Mod4-Shift-[a-z] - swap position of active window.
Mod4-Delete - kill an active window.

If using multiple monitors:
Mod4-[0-9] - Focus another monitor.
Mod4-Shift-[0-9] - Swap the window in the currently focused monitor.
