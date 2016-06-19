GTK UVC VIEWER for OPI with CMOS Camera (guvcview)
***************************************************

This is my modified version (2.0.2) that works with OPI (Orange Pi) PC / One / 2Plus / 2E
with CMOS camera.

Known issues:
 * Changing resolution on the fly may lock gucview and you will need to kill the process
 * Encoding is done by software and not hardware as it is not ready for the H3 platform yet
 * Encoding video stream may end with only 2 fps and sound out of sync, better use a heatsink on your board
 * Start guvcview via command line with the correct parameters
 * Cannot change FPS
 


Basic Configuration
===================
Dependencies:
-------------

Guvcview depends on the following:
 - intltool,
 - autotools, 
 - libsdl2 or libsdl, 
 - libgtk-3, 
 - portaudio19, 
 - libpng, 
 - libavcodec, 
 - libavutil, 
 - libv4l, 
 - libudev,
 - libusb-1.0,
 - libpulse (optional)
 - libgsl (optional)

On most distributions you can just install the development 
packages:
 intltool, autotools-dev, libsdl2-dev, libgtk-3-dev, 
 portaudio19-dev, libpng12-dev, libavcodec-dev, libavutil-dev,
 libv4l-dev, libudev-dev, libusb-1.0-0-dev, libpulse-dev, libgsl-dev

Build configuration:
--------------------
(./bootstrap.sh; ./configure)

The configure script is generated from configure.ac by autoconf,
the helper script ./bootstrap.sh can be used for this, it will also
run the generated configure with the command line options passed.
After configuration a simple 'make && make install' will build and
install guvcview and all the associated data files.

Data Files:
------------
(language files; image files; gnome menu entry)

guvcview data files are stored by default to /usr/local/share
setting a different prefix (--prefix=BASEDIR) during configuration
will change the installation path to BASEDIR/share.

Built files, src/guvcview and data/gnome.desktop, are dependent 
on this path, so if a new prefix is set a make clean is required 
before issuing the make command. 

After running the configure script the normal, make && make install 
should build and install all the necessary files.    
    
 
guvcview bin:
-------------
(guvcview)

The binarie file installs to the standart location,
/usr/local/bin, to change the install path, configure
must be executed with --prefix=DIR set, this will cause
the bin file to be installed in DIR/bin, make sure 
DIR/bin is set in your PATH variable, or the gnome 
menu entry will fail.

guvcview libraries:
-------------------
(libgviewv4l2core, libgviewrender, libgviewaudio, libgviewencoder)

The core functionality of guvcview is now split into 4 libraries
these will install to ${prefix}/lib and development headers to
${prefix}/include/guvcview-2/libname. 
pkg-config should be use to determine the compile flags.


guvcview.desktop:
-----------------

(data/guvcview.desktop)

The desktop file (gnome menu entry) is built from the
data/guvcview.desktop.in definition and is dependent on the 
configure --prefix setting, any changes to this, must 
be done in data/guvcview.desktop.in.

configuration files:
--------------------
(~/.config/guvcview2/video0)

The configuration file is saved into the $HOME dir when 
exiting guvcview. If a video device with index > 0,
e.g: /dev/video1 is used then the file stored will be
named ~/.config/guvcview2/video1

Executing guvcview
================== 

For instructions on the command line args 
execute "guvcview --help".
