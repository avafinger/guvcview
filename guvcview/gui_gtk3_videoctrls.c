/*******************************************************************************#
#           guvcview              http://guvcview.sourceforge.net               #
#                                                                               #
#           Paulo Assis <pj.assis@gmail.com>                                    #
#           Nobuhiro Iwamatsu <iwamatsu@nigauri.org>                            #
#                             Add UYVY color support(Macbook iSight)            #
#           Flemming Frandsen <dren.dk@gmail.com>                               #
#                             Add VU meter OSD                                  #
#                                                                               #
# This program is free software; you can redistribute it and/or modify          #
# it under the terms of the GNU General Public License as published by          #
# the Free Software Foundation; either version 2 of the License, or             #
# (at your option) any later version.                                           #
#                                                                               #
# This program is distributed in the hope that it will be useful,               #
# but WITHOUT ANY WARRANTY; without even the implied warranty of                #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 #
# GNU General Public License for more details.                                  #
#                                                                               #
# You should have received a copy of the GNU General Public License             #
# along with this program; if not, write to the Free Software                   #
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA     #
#                                                                               #
********************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
/* support for internationalization - i18n */
#include <locale.h>
#include <libintl.h>

#include "gui_gtk3.h"
#include "gui_gtk3_callbacks.h"
#include "gui.h"
/*add this last to avoid redefining _() and N_()*/
#include "gview.h"
#include "gviewrender.h"
#include "video_capture.h"


extern int debug_level;

/*
 * attach v4l2 video controls tab widget
 * args:
 *   parent - tab parent widget
 *
 * asserts:
 *   parent is not null
 *
 * returns: error code (0 -OK)
 */
int gui_attach_gtk3_videoctrls(GtkWidget *parent)
{
	/*assertions*/
	assert(parent != NULL);

	if(debug_level > 1)
		printf("GUVCVIEW: attaching video controls\n");

	int format_index = v4l2core_get_frame_format_index(v4l2core_get_requested_frame_format());

	if(format_index < 0)
	{
		gui_error("Guvcview error", "invalid pixel format", 0);
		printf("GUVCVIEW: invalid pixel format\n");
	}

	int resolu_index = v4l2core_get_format_resolution_index(
		format_index,
		v4l2core_get_frame_width(),
		v4l2core_get_frame_height());

	if(resolu_index < 0)
	{
		gui_error("Guvcview error", "invalid resolution index", 0);
		printf("GUVCVIEW: invalid resolution index\n");
	}

	GtkWidget *video_controls_grid = gtk_grid_new();
	gtk_widget_show (video_controls_grid);

	gtk_grid_set_column_homogeneous (GTK_GRID(video_controls_grid), FALSE);
	gtk_widget_set_hexpand (video_controls_grid, TRUE);
	gtk_widget_set_halign (video_controls_grid, GTK_ALIGN_FILL);

	gtk_grid_set_row_spacing (GTK_GRID(video_controls_grid), 4);
	gtk_grid_set_column_spacing (GTK_GRID (video_controls_grid), 4);
	gtk_container_set_border_width (GTK_CONTAINER (video_controls_grid), 2);

	char temp_str[20];
	int line = 0;
	int i =0;

	/*---- Devices ----*/
	GtkWidget *label_Device = gtk_label_new(_("Device:"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_Device), 1);
	gtk_label_set_yalign(GTK_LABEL(label_Device), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_Device), 1, 0.5);
#endif

	gtk_grid_attach (GTK_GRID(video_controls_grid), label_Device, 0, line, 1, 1);

	gtk_widget_show (label_Device);

	set_wgtDevices_gtk3(gtk_combo_box_text_new());
	gtk_widget_set_halign (get_wgtDevices_gtk3(), GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (get_wgtDevices_gtk3(), TRUE);

	v4l2_device_list *device_list = v4l2core_get_device_list();

	if (device_list->num_devices < 1)
	{
		/*use current*/
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(get_wgtDevices_gtk3()),
			v4l2core_get_videodevice());
		gtk_combo_box_set_active(GTK_COMBO_BOX(get_wgtDevices_gtk3()),0);
	}
	else
	{
		for(i = 0; i < (device_list->num_devices); i++)
		{
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(get_wgtDevices_gtk3()),
				device_list->list_devices[i].name);
			if(device_list->list_devices[i].current)
				gtk_combo_box_set_active(GTK_COMBO_BOX(get_wgtDevices_gtk3()),i);
		}
	}
	gtk_grid_attach (GTK_GRID(video_controls_grid), get_wgtDevices_gtk3(), 1, line, 1 ,1);
	gtk_widget_show (get_wgtDevices_gtk3());
	g_signal_connect (GTK_COMBO_BOX_TEXT(get_wgtDevices_gtk3()), "changed",
		G_CALLBACK (devices_changed), NULL);

	/*---- Frame Rate ----*/
	line++;

	GtkWidget *label_FPS = gtk_label_new(_("Frame Rate:"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_FPS), 1);
	gtk_label_set_yalign(GTK_LABEL(label_FPS), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_FPS), 1, 0.5);
#endif
	gtk_widget_show (label_FPS);

	gtk_grid_attach (GTK_GRID(video_controls_grid), label_FPS, 0, line, 1, 1);

	GtkWidget *wgtFrameRate = gtk_combo_box_text_new ();
	gtk_widget_set_halign (wgtFrameRate, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (wgtFrameRate, TRUE);

	gtk_widget_show (wgtFrameRate);
	gtk_widget_set_sensitive (wgtFrameRate, TRUE);

	int deffps=0;

	v4l2_stream_formats_t *list_stream_formats = v4l2core_get_formats_list();
	
	if (debug_level > 0)
		printf("GUVCVIEW: frame rates of resolution index %d = %d \n",
			resolu_index+1,
			list_stream_formats[format_index].list_stream_cap[resolu_index].numb_frates);
	for ( i = 0 ; i < list_stream_formats[format_index].list_stream_cap[resolu_index].numb_frates ; ++i)
	{
		g_snprintf(
			temp_str,
			18,
			"%i/%i fps",
			list_stream_formats[format_index].list_stream_cap[resolu_index].framerate_denom[i],
			list_stream_formats[format_index].list_stream_cap[resolu_index].framerate_num[i]);

		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(wgtFrameRate), temp_str);

		if (( v4l2core_get_fps_num() == list_stream_formats[format_index].list_stream_cap[resolu_index].framerate_num[i]) &&
			( v4l2core_get_fps_denom() == list_stream_formats[format_index].list_stream_cap[resolu_index].framerate_denom[i]))
				deffps=i;//set selected
	}

	gtk_grid_attach (GTK_GRID(video_controls_grid), wgtFrameRate, 1, line, 1 ,1);

	gtk_combo_box_set_active(GTK_COMBO_BOX(wgtFrameRate), deffps);

	if (deffps==0)
	{
		if (list_stream_formats[format_index].list_stream_cap[resolu_index].framerate_denom)
			v4l2core_define_fps(-1, list_stream_formats[format_index].list_stream_cap[resolu_index].framerate_denom[0]);

		if (list_stream_formats[format_index].list_stream_cap[resolu_index].framerate_num)
			v4l2core_define_fps(list_stream_formats[format_index].list_stream_cap[resolu_index].framerate_num[0], -1);
	}

	g_signal_connect (GTK_COMBO_BOX_TEXT(wgtFrameRate), "changed",
		G_CALLBACK (frame_rate_changed), NULL);

	/*try to sync the device fps (capture thread must have started by now)*/
	v4l2core_request_framerate_update ();

	/*---- Resolution ----*/
	line++;

	GtkWidget *label_Resol = gtk_label_new(_("Resolution:"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_Resol), 1);
	gtk_label_set_yalign(GTK_LABEL(label_Resol), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_Resol), 1, 0.5);
#endif
	gtk_grid_attach (GTK_GRID(video_controls_grid), label_Resol, 0, line, 1, 1);
	gtk_widget_show (label_Resol);

	GtkWidget *wgtResolution = gtk_combo_box_text_new ();
	gtk_widget_set_halign (wgtResolution, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (wgtResolution, TRUE);

	gtk_widget_show (wgtResolution);
	gtk_widget_set_sensitive (wgtResolution, TRUE);

	int defres=0;

	if (debug_level > 0)
		printf("GUVCVIEW: resolutions of format(%d) = %d \n",
			format_index+1,
			list_stream_formats[format_index].numb_res);
	for(i = 0 ; i < list_stream_formats[format_index].numb_res ; i++)
	{
		if (list_stream_formats[format_index].list_stream_cap[i].width > 0)
		{
			g_snprintf(
				temp_str,
				18,
				"%ix%i",
				list_stream_formats[format_index].list_stream_cap[i].width,
				list_stream_formats[format_index].list_stream_cap[i].height);

			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(wgtResolution), temp_str);

			if ((v4l2core_get_frame_width() == list_stream_formats[format_index].list_stream_cap[i].width) &&
				(v4l2core_get_frame_height() == list_stream_formats[format_index].list_stream_cap[i].height))
					defres=i;//set selected resolution index
		}
	}

	gtk_combo_box_set_active(GTK_COMBO_BOX(wgtResolution), defres);

	gtk_grid_attach (GTK_GRID(video_controls_grid), wgtResolution, 1, line, 1 ,1);

	g_object_set_data (G_OBJECT (wgtResolution), "control_fps", wgtFrameRate);

	g_signal_connect (wgtResolution, "changed",
		G_CALLBACK (resolution_changed), NULL);


	/*---- Input Format ----*/
	line++;

	GtkWidget *label_InpType = gtk_label_new(_("Camera Output:"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_InpType), 1);
	gtk_label_set_yalign(GTK_LABEL(label_InpType), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_InpType), 1, 0.5);
#endif

	gtk_widget_show (label_InpType);

	gtk_grid_attach (GTK_GRID(video_controls_grid), label_InpType, 0, line, 1, 1);

	GtkWidget *wgtInpType= gtk_combo_box_text_new ();
	gtk_widget_set_halign (wgtInpType, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (wgtInpType, TRUE);

	gtk_widget_show (wgtInpType);
	gtk_widget_set_sensitive (wgtInpType, TRUE);

	int fmtind=0;
	for (fmtind=0; fmtind < v4l2core_get_number_formats(); fmtind++)
	{
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(wgtInpType), list_stream_formats[fmtind].fourcc);
		if(v4l2core_get_requested_frame_format() == list_stream_formats[fmtind].format)
			gtk_combo_box_set_active(GTK_COMBO_BOX(wgtInpType), fmtind); /*set active*/
	}

	gtk_grid_attach (GTK_GRID(video_controls_grid), wgtInpType, 1, line, 1 ,1);

	//g_object_set_data (G_OBJECT (wgtInpType), "control_fps", wgtFrameRate);
	g_object_set_data (G_OBJECT (wgtInpType), "control_resolution", wgtResolution);

	g_signal_connect (GTK_COMBO_BOX_TEXT(wgtInpType), "changed",
		G_CALLBACK (format_changed), NULL);


	/* ----- Filter controls -----*/
	line++;
	GtkWidget *label_videoFilters = gtk_label_new(_("---- Video Filters ----"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_videoFilters), 0.5);
	gtk_label_set_yalign(GTK_LABEL(label_videoFilters), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_videoFilters), 0.5, 0.5);
#endif

	gtk_grid_attach (GTK_GRID(video_controls_grid), label_videoFilters, 0, line, 3, 1);
	gtk_widget_show (label_videoFilters);

	/*filters grid*/
	line++;
	GtkWidget *table_filt = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (table_filt), 4);
	gtk_grid_set_column_spacing (GTK_GRID (table_filt), 4);
	gtk_container_set_border_width (GTK_CONTAINER (table_filt), 4);
	gtk_widget_set_size_request (table_filt, -1, -1);

	gtk_widget_set_halign (table_filt, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (table_filt, TRUE);
	gtk_grid_attach (GTK_GRID(video_controls_grid), table_filt, 0, line, 3, 1);
	gtk_widget_show (table_filt);

	/* Mirror FX */
	GtkWidget *FiltMirrorEnable = gtk_check_button_new_with_label (_(" Mirror"));
	g_object_set_data (G_OBJECT (FiltMirrorEnable), "filt_info", GINT_TO_POINTER(REND_FX_YUV_MIRROR));
	gtk_widget_set_halign (FiltMirrorEnable, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (FiltMirrorEnable, TRUE);
	gtk_grid_attach(GTK_GRID(table_filt), FiltMirrorEnable, 0, 0, 1, 1);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FiltMirrorEnable),
		(get_render_fx_mask() & REND_FX_YUV_MIRROR) > 0);
	gtk_widget_show (FiltMirrorEnable);
	g_signal_connect (GTK_CHECK_BUTTON(FiltMirrorEnable), "toggled",
		G_CALLBACK (render_fx_filter_changed), NULL);
	/* Upturn FX */
	GtkWidget *FiltUpturnEnable = gtk_check_button_new_with_label (_(" Invert"));
	g_object_set_data (G_OBJECT (FiltUpturnEnable), "filt_info", GINT_TO_POINTER(REND_FX_YUV_UPTURN));
	gtk_widget_set_halign (FiltUpturnEnable, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (FiltUpturnEnable, TRUE);
	gtk_grid_attach(GTK_GRID(table_filt), FiltUpturnEnable, 1, 0, 1, 1);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FiltUpturnEnable),
		(get_render_fx_mask() & REND_FX_YUV_UPTURN) > 0);
	gtk_widget_show (FiltUpturnEnable);
	g_signal_connect (GTK_CHECK_BUTTON(FiltUpturnEnable), "toggled",
		G_CALLBACK (render_fx_filter_changed), NULL);
	/* Negate FX */
	GtkWidget *FiltNegateEnable = gtk_check_button_new_with_label (_(" Negative"));
	g_object_set_data (G_OBJECT (FiltNegateEnable), "filt_info", GINT_TO_POINTER(REND_FX_YUV_NEGATE));
	gtk_widget_set_halign (FiltNegateEnable, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (FiltNegateEnable, TRUE);
	gtk_grid_attach(GTK_GRID(table_filt), FiltNegateEnable, 2, 0, 1, 1);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FiltNegateEnable),
		(get_render_fx_mask() & REND_FX_YUV_NEGATE) >0 );
	gtk_widget_show (FiltNegateEnable);
	g_signal_connect (GTK_CHECK_BUTTON(FiltNegateEnable), "toggled",
		G_CALLBACK (render_fx_filter_changed), NULL);
	/* Mono FX */
	GtkWidget *FiltMonoEnable = gtk_check_button_new_with_label (_(" Mono"));
	g_object_set_data (G_OBJECT (FiltMonoEnable), "filt_info", GINT_TO_POINTER(REND_FX_YUV_MONOCR));
	gtk_widget_set_halign (FiltMonoEnable, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (FiltMonoEnable, TRUE);
	gtk_grid_attach(GTK_GRID(table_filt), FiltMonoEnable, 3, 0, 1, 1);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FiltMonoEnable),
		(get_render_fx_mask() & REND_FX_YUV_MONOCR) > 0);
	gtk_widget_show (FiltMonoEnable);
	g_signal_connect (GTK_CHECK_BUTTON(FiltMonoEnable), "toggled",
		G_CALLBACK (render_fx_filter_changed), NULL);

	/* Pieces FX */
	GtkWidget *FiltPiecesEnable = gtk_check_button_new_with_label (_(" Pieces"));
	g_object_set_data (G_OBJECT (FiltPiecesEnable), "filt_info", GINT_TO_POINTER(REND_FX_YUV_PIECES));
	gtk_widget_set_halign (FiltPiecesEnable, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (FiltPiecesEnable, TRUE);
	gtk_grid_attach(GTK_GRID(table_filt), FiltPiecesEnable, 4, 0, 1, 1);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FiltPiecesEnable),
		(get_render_fx_mask() & REND_FX_YUV_PIECES) > 0);
	gtk_widget_show (FiltPiecesEnable);
	g_signal_connect (GTK_CHECK_BUTTON(FiltPiecesEnable), "toggled",
		G_CALLBACK (render_fx_filter_changed), NULL);

	/* Particles */
	GtkWidget *FiltParticlesEnable = gtk_check_button_new_with_label (_(" Particles"));
	g_object_set_data (G_OBJECT (FiltParticlesEnable), "filt_info", GINT_TO_POINTER(REND_FX_YUV_PARTICLES));
	gtk_widget_set_halign (FiltParticlesEnable, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (FiltParticlesEnable, TRUE);
	gtk_grid_attach(GTK_GRID(table_filt), FiltParticlesEnable, 5, 0, 1, 1);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FiltParticlesEnable),
		(get_render_fx_mask() & REND_FX_YUV_PARTICLES) > 0);
	gtk_widget_show (FiltParticlesEnable);
	g_signal_connect (GTK_CHECK_BUTTON(FiltParticlesEnable), "toggled",
		G_CALLBACK (render_fx_filter_changed), NULL);

	/* ----- OSD controls -----*/
	line++;
	GtkWidget *label_osd = gtk_label_new(_("---- OSD ----"));
#if GTK_VER_AT_LEAST(3,15)
	gtk_label_set_xalign(GTK_LABEL(label_osd), 0.5);
	gtk_label_set_yalign(GTK_LABEL(label_osd), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (label_osd), 0.5, 0.5);
#endif

	gtk_grid_attach (GTK_GRID(video_controls_grid), label_osd, 0, line, 3, 1);
	gtk_widget_show (label_osd);

	/*osd grid*/
	line++;
	GtkWidget *table_osd = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (table_osd), 4);
	gtk_grid_set_column_spacing (GTK_GRID (table_osd), 4);
	gtk_container_set_border_width (GTK_CONTAINER (table_osd), 4);
	gtk_widget_set_size_request (table_osd, -1, -1);

	gtk_widget_set_halign (table_osd, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (table_osd, TRUE);
	gtk_grid_attach (GTK_GRID(video_controls_grid), table_osd, 0, line, 3, 1);
	gtk_widget_show (table_osd);

	/* Crosshair OSD */
	GtkWidget *OsdCrosshairEnable = gtk_check_button_new_with_label (_(" Cross-hair"));
	g_object_set_data (G_OBJECT (OsdCrosshairEnable), "osd_info", GINT_TO_POINTER(REND_OSD_CROSSHAIR));
	gtk_widget_set_halign (OsdCrosshairEnable, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (OsdCrosshairEnable, TRUE);
	gtk_grid_attach(GTK_GRID(table_osd), OsdCrosshairEnable, 0, 0, 1, 1);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(OsdCrosshairEnable),
		(render_get_osd_mask() & REND_OSD_CROSSHAIR) > 0);
	gtk_widget_show (OsdCrosshairEnable);
	g_signal_connect (GTK_CHECK_BUTTON(OsdCrosshairEnable), "toggled",
		G_CALLBACK (render_osd_changed), NULL);
	
	/*add control grid to parent container*/
	gtk_container_add(GTK_CONTAINER(parent), video_controls_grid);

	return 0;
}
