/*******************************************************************************#
#           guvcview              http://guvcview.sourceforge.net               #
#                                                                               #
#           Paulo Assis <pj.assis@gmail.com>                                    #
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

#ifndef V4L2_DEVICES_H
#define V4L2_DEVICES_H

#include "gviewv4l2core.h"
#include "v4l2_core.h"

/*
 * enumerate available v4l2 devices
 * and creates list in vd->list_devices
 * args:
 *   none
 *
 * asserts:
 *   my_device_list.videodevice is not null
 *   my_device_list.udev is valid ( > 0 )
 *   my_device_list.list_devices is null
 *
 * returns: error code
 */
int enum_v4l2_devices();

/*
 * check for new devices
 * args:
 *   vd - pointer to device data (can be null)
 *
 * asserts:
 *   my_device_list.udev is not null
 *   my_device_list.udev_fd is valid (> 0)
 *   my_device_list.udev_mon is not null
 *
 * returns: true(1) if device list was updated, false(0) otherwise
 */
int check_device_list_events(v4l2_dev_t *vd);

#endif
