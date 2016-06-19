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

#include <assert.h>
#include <math.h>

#include "gview.h"
#include "gviewrender.h"

extern int verbosity;

typedef struct _yuv_color_t
{
	int8_t y;
	int8_t u;
	int8_t v;
} yuv_color_t;


/*
 * plot a crosshair in a yuyv frame (packed)
 * args:
 *   frame - pointer to yuyv frame data
 *   size  - crossair size in pixels (width)
 *   width - width
 *   height - height
 *   color - box color
 *
 * asserts:
 *   none
 *
 * returns: none
 */
static plot_crosshair_yuyv(uint8_t *frame, int size, int width, int height, yuv_color_t *color)
{
	int linesize = width*2; /*two bytes per pixel*/
	
	
	/*1st vertical line*/
	int y = 0; 
	/*center - size/2 to center - 2*/	
	for (y = (height-size)/2; y < (height/2) - 2; y++)
  {
		int bi = (y * linesize) + width; /*center (2 bytes per pixel)*/

		/*we set two pixels in each loop*/
		frame[bi] = color->y;
	  frame[bi+1] = color->u;
		frame[bi+2] = color->y;
	  frame[bi+3] = color->v;
	}
	/*1st horizontal line*/
	int x = (width-size)/2;
	/*center - size/2 to center -4*/
	for (x = (width-size)/2; x < (width/2) - 4; x+=2)
  {
		int bi = (x * 2) + (height/2)*linesize; /*center (2 bytes per pixel)*/

		/*we set two pixels in each loop*/
		frame[bi] = color->y;
	  frame[bi+1] = color->u;
		frame[bi+2] = color->y;
	  frame[bi+3] = color->v;
	}
	/*2nd horizontal line*/
	/*center + 2 to center + size/2 -2*/
	for (x = (width/2) + 2; x < (width + size)/ 2 - 2; x+=2)
  {
		int bi = (x * 2) + (height/2)*linesize; /*center (2 bytes per pixel)*/

		/*we set two pixels in each loop*/
		frame[bi] = color->y;
	  frame[bi+1] = color->u;
		frame[bi+2] = color->y;
	  frame[bi+3] = color->v;
	}

	/*2st vertical line*/
	/*center + 2 to center + size/2*/
	for (y = (height/2) +2; y < (height+size)/2; y++)
  {
		int bi = (y * linesize) + width; /*center (2 bytes per pixel)*/

		/*we set two pixels in each loop*/
		frame[bi] = color->y;
	  frame[bi+1] = color->u;
		frame[bi+2] = color->y;
	  frame[bi+3] = color->v;
	}

}

/*
 * plot a crosshair in a yu12 frame (planar)
 * args:
 *   frame - pointer to yu12 frame data
 *   size  - frame line size in pixels (width)
 *   width - width
 *   height - height 
 *   color - line color
 *
 * asserts:
 *   none
 *
 * returns: none
 */
static plot_crosshair_yu12(uint8_t *frame, int size, int width, int height, yuv_color_t *color)
{
	uint8_t *py = frame;
	uint8_t *pu = frame + (width * height);
	uint8_t *pv = pu + ((width * height) / 4);

	/*y - 1st vertical line*/
	int h = (height-size)/2;
	for(h = (height-size)/2; h < height/2 - 2; h++)
	{
		py = frame + (h * width) + width/2;
		*py = color->y;
	}
	/*y - 1st horizontal line*/
	int w = (width-size)/2;
	for(w = (width-size)/2; w < width/2 - 2; w++)
	{
		py = frame + ((height/2) * width) + w;
		*py = color->y;
	}
	/*y - 2nd horizontal line*/
	for(w = width/2 + 2; w < (width+size)/2; w++)
	{
		py = frame + ((height/2) * width) + w;
		*py = color->y;
	}
	/*y - 2nd vertical line*/
	for(h = height/2 + 2; h < (height+size)/2; h++)
	{
		py = frame + (h * width) + width/2;
		*py = color->y;
	}

				
	/*u v - 1st vertical line*/
	for(h = (height-size)/4; h < height/4 - 1; h++) /*every two rows*/
	{
		pu = frame + (width * height) + (h * width/2) + width/4;
		*pu = color->u;
		pv = pu + (width * height)/4;
		*pv = color->v;
	}
	/*u v - 1st horizontal line*/
	for(w = (width-size)/4; w < width/4 - 1; w++) /*every two rows*/
	{
		pu = frame + (width * height) + ((height/4) * width/2) + w;
		*pu = color->u;
		pv = pu + (width * height)/4;
		*pv = color->v;
	}
	/*u v - 2nd horizontal line*/
	for(w = width/4 + 1; w < (width+size)/4; w++) /*every two rows*/
	{
		pu = frame + (width * height) + ((height/4) * width/2) + w;
		*pu = color->u;
		pv = pu + (width * height)/4;
		*pv = color->v;
	}
	/*u v - 2nd vertical line*/
	for(h = height/4 + 1; h < (height+size)/4; h++) /*every two rows*/
	{
		pu = frame + (width * height) + (h * width/2) + width/4;
		*pu = color->u;
		pv = pu + (width * height)/4;
		*pv = color->v;
	}
}

/*
 * render a crosshair
 * args:
 *   frame - pointer to yuyv frame data
 *   width - frame width
 *   height - frame height
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void render_osd_crosshair(uint8_t *frame, int width, int height)
{
			yuv_color_t color;
			color.y = 154;
			color.u = 72;
			color.v = 57;

	
#ifdef USE_PLANAR_YUV
				plot_crosshair_yu12(frame, 24, width, height, &color);
#else
  			plot_crosshair_yuyv(frame, 24, width, height, &color);
#endif
			
}
