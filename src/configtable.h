/*
 * configtable.h
 *
 *  Created on: 28 Jan 2013
 *      Author: Sjoerd
 */

//The table of config entries,
//Depending on the includer to define this
//Make sure to group similar sized entries for better alignment

#ifdef CONF_GLOBAL

#define CONF_ENTRY( _TYPE, _NAME, _RESET,  _MIN, _MAX )	\
		CONF_TYPEDEF_ ## _TYPE _NAME;
#else

#define CONF_ENTRY( _TYPE, _NAME, _RESET,  _MIN, _MAX )	\
	{ .type = CONF_TYPE_ ## _TYPE, .data = &cfg. _NAME, .name = #_NAME, .reset. _TYPE = _RESET, .min. _TYPE = _MIN, .max. _TYPE = _MAX, .flags = 0 },

#endif

//Baudrate for the gps
CONF_ENTRY( U32, gpsBaudrate, 115200, 0, 0 )
//width of the osd
CONF_ENTRY( S16, width, OSD_WIDTH_MAX, OSD_WIDTH_MAX /2, OSD_WIDTH_MAX )
//Height of the osd
CONF_ENTRY( S16, height, OSD_HEIGHT_MAX, 1, OSD_HEIGHT_MAX  )

//How many pixels to delay before drawing the osd
CONF_ENTRY( U8, delayX, 55, 1, 255 )
//How many lines to delay before drawing the osd
CONF_ENTRY( U8, delayY, 25, 1, 100 )
//Main 72mhz clock divided by this gives twice the pixelclock
CONF_ENTRY( U8, clockDivider, 9, 2, 16 )
//Show a debug border around the screen
CONF_ENTRY( U8, showBorder, 0, 0, 1 )

#undef CONF_ENTRY


