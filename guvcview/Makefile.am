## Process this file with automake to produce Makefile.in

#AM_CFLAGS =

bin_PROGRAMS = guvcview

guvcview_SOURCES = guvcview.c \
				   video_capture.c \
				   core_io.c \
				   options.c \
				   config.c \
				   gui.c \
				   gui_gtk3.c \
				   gui_gtk3_menu.c \
				   gui_gtk3_v4l2ctrls.c \
				   gui_gtk3_videoctrls.c \
				   gui_gtk3_h264ctrls.c \
				   gui_gtk3_audioctrls.c \
				   gui_gtk3_callbacks.c

guvcview_CFLAGS = $(GUVCVIEW_CFLAGS) \
		  $(PTHREAD_CFLAGS) \
		  -D_REENTRANT\
		  -D_FILE_OFFSET_BITS=64\
		  -Wall\
		  -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
		  -DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
		  -DPACKAGE_DATA_DIR=\""$(datadir)"\" \
		  $(DEPS_CFLAGS) $(EXTRA_CFLAGS) $(PTHREAD_CFLAGS) \
		  $(EXTRA_CFLAGS) -I/usr/src/linux-headers-$(uname -r) -I$(top_srcdir) -I$(top_srcdir)/includes \
		  -I$(top_srcdir)/gview_v4l2core \
		  -I$(top_srcdir)/gview_render \
		  -I$(top_srcdir)/gview_audio \
		  -I$(top_srcdir)/gview_encoder

guvcview_LDFLAGS = $(LIBINTL)

guvcview_LDADD = ../gview_v4l2core/$(GVIEWV4L2CORE_LIBRARY_NAME).la \
                 ../gview_render/$(GVIEWRENDER_LIBRARY_NAME).la \
                 ../gview_audio/$(GVIEWAUDIO_LIBRARY_NAME).la \
                 ../gview_encoder/$(GVIEWENCODER_LIBRARY_NAME).la \
                 $(GUVCVIEW_LIBS) \
				 $(PTHREAD_LIBS) \
                 -lm

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@



