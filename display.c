/* $Header: display.c,v 1.7 88/08/25 09:57:54 kent Exp $ */

/* 
 * display.c - Display management routines for MazeWar
 * 
 * Author:	Christopher A. Kent
 * 		Western Research Laboratory
 *		Digital Equipment Corporation
 * Date:	Wed Oct  1 1986
 */

/***********************************************************
Copyright 1986 by Digital Equipment Corporation, Maynard, Massachusetts,

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital not be
used in advertising or publicity pertaining to disstribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/*
 * $Log:	display.c,v $
 * Revision 1.7  88/08/25  09:57:54  kent
 * More bitmap portability hacks.
 * 
 * Revision 1.6  87/03/31  14:41:43  kent
 * Portability considerations, especially byteswapping to/from the net.
 * 
 * Revision 1.5  86/12/04  17:43:39  kent
 * A little efficiency in DisplayOthersPosition().
 * 
 * Revision 1.3  86/12/03  13:29:52  kent
 * Lint pass. 
 * 
 * Revision 1.2  86/12/01  23:45:36  kent
 * Housecleaning and documentation pass.
 * 
 * 
 * Revision 1.1  86/11/26  16:56:12  kent
 * Initial revision
 * 
 */

#ifndef	lint
static char rcs_ident[] = "$Header: display.c,v 1.7 88/08/25 09:57:54 kent Exp $";
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <netinet/in.h>

#include <stdio.h>

#include "mazewar.h"

/* 
 * Manage the display. This routine handles the three areas of the
 * screen: the perspective view, the maze overview, and the scores.
 * 
 * The coordinate system of the display is a x-reflected 1st quadrant:
 * (0,0) upper left, x increases to the right, y increases down. Just
 * like most CRT coordinate systems. North is to the right, for no
 * particular reason. Cells in the maze are 16x16.
 */

typedef	struct {
	Boolean		playing;
	int		x, y;
	Direction	dir;
} RatState;

static RatState	clearArray[MAXRATS];

static
BitCell	normalArrows[NDIRECTION] = {
      {	0,	0200,	0300,	0340,	/* right */
	0360,	0370,	0177774,01777776,
	0177777,0177776,0177774,0370,
	0360,	0340,	0300,	0200	},
      {	0400,	01400,	03400,	07400,	/* left */
	017400,	037777,	077777,	0177777,
	077777,	037777,	017400,	07400,
	03400,	01400,	0400,	0	},
      {	01740,	01740,	01740,	01740,	/* down */
	01740,	01740,	01740,	01740,
	077777,	037776,	017774,	07770,
	03760,	01740,	0700,	0200	},
      { 0200,	0700,	01740,	03760,	/* up */
	07770,	017774,	037776,	077777,
	01740,	01740,	01740,	01740,
	01740,	01740,	01740,	01740	}
};

static
BitCell	invincibleArrows[NDIRECTION] = {
      {	0,	0600,	0300,	0340,	/* right */
	0160,	0210,	0177464,0177102,
	0177043,0177022,0177544,0210,
	0160,	0340,	0300,	0600	},
      {	0700,	01400,	03400,	07000,	/* left */
	010400,	023377,	044177,	0142177,
	041177,	026377,	010400,	07000,
	03400,	01400,	0700,	0	},
      {	01740,	01740,	01740,	01740,	/* down */
	01740,	01740,	01740,	041041,
	072327,	034416,	012414,	04110,
	02620,	01040,	0700,	0200	},
      {	0200,	0700,	01040,	02320,	/* up */
	04410,	014214,	034116,	072627,
	041041,	01740,	01740,	01740,
	01740,	01740,	01740,	01740	}
};

static
BitCell	otherArrows[NDIRECTION] = {
      {	0,	0200,	0300,	0340,	/* right */
	0177660,0100030,0100014,0100006,
	0100003,0100006,0100014,0100030,
	0177660,0340,	0300,	0200	},
      {	0,	0400,	01400,	03400,	/* left */
	06777,	014001,	030001,	060001,
	0140001,060001,	030001,	014001,
	06777,	03400,	01400,	0400	},
      {	07770,	04010,	04010,	04010,	/* down */
	04010,	04010,	04010,	04010,
	074017,	034016,	014014,	06030,
	03060,	01540,	0700,	0200	},
      {	0200,	0700,	01540,	03060,	/* up */
	06030,	014014,	030006,	074017,	/* 074014 in original */
	04010,	04010,	04010,	04010,
	04010,	04010,	04010,	07770	}
};

static
BitCell	numbers[MAXRATS] = {
      {	0,	0,	0,	0,	/* 0 */
	0,	0,	0300,	0440,
	0440,	0440,	0300,	0,
	0,	0,	0,	0	},
      {	0,	0,	0,	0,	/* 1 */
	0,	0,	0600,	0200,
	0200,	0200,	0700,	0,
	0,	0,	0,	0	},
      {	0,	0,	0,	0,	/* 2 */
	0,	0,	0600,	01100,
	0200,	0400,	01700,	0,
	0,	0,	0,	0	},
      {	0,	0,	0,	0,	/* 3 */
	0,	0,	01600,	0100,
	0600,	0100,	01600,	0,
	0,	0,	0,	0	},
      {	0,	0,	0,	0,	/* 4 */
	0,	0,	01100,	01100,
	01740,	0100,	0100,	0,
	0,	0,	0,	0	},
      {	0,	0,	0,	0,	/* 5 */
	0,	0,	01700,	01000,
	01700,	0100,	01700,	0,
	0,	0,	0,	0	},
      {	0,	0,	0,	0,	/* 6 */
	0,	0,	01700,	01000,
	01700,	01100,	01700,	0,
	0,	0,	0,	0	},
      {	0,	0,	0,	0,	/* 7 */
	0,	0,	01700,	0100,
	0200,	0400,	01000,	0,
	0,	0,	0,	0	}
};

static XYpair	viewTable[] = {
	{{ 67,	333 },	{ 67,	67 }},	{{ 67,	333 },	{ 333,	333 }},
	{{ 67,	67 },	{ 333,	67 }},	{{ 0,	400 },	{ 67,	333 }},
	{{ 0,	0 },	{ 67,	67 }},	{{ 0,	333 },	{ 67,	333 }},
	{{ 0,	67 },	{ 67,	67 }},	{{ 333,	333 },	{ 333,	67 }},
	{{ 333,	333 },	{ 400,	333 }},	{{ 333,	67 },	{ 400,	67 }},
	{{ 333,	333 },	{ 400,	400 }},	{{ 333,	67 },	{ 400,	0 }},
	{{ 120,	280 },	{ 120,	120 }},	{{ 120,	280 },	{ 280,	280 }},
	{{ 120,	120 },	{ 280,	120 }},	{{ 67,	333 },	{ 120,	280 }},
	{{ 67,	67 },	{ 120,	120 }},	{{ 67,	280 },	{ 120,	280 }},
	{{ 67,	120 },	{ 120,	120 }},	{{ 280,	280 },	{ 280,	120 }},
	{{ 280,	280 },	{ 333,	280 }},	{{ 280,	120 },	{ 333,	120 }},
	{{ 280,	280 },	{ 333,	333 }},	{{ 280,	120 },	{ 333,	67 }},
	{{ 143,	257 },	{ 143,	143 }},	{{ 143,	257 },	{ 257,	257 }},
	{{ 143,	143 },	{ 257,	143 }},	{{ 120,	280 },	{ 143,	257 }},
	{{ 120,	120 },	{ 143,	143 }},	{{ 120,	257 },	{ 143,	257 }},
	{{ 120,	143 },	{ 143,	143 }},	{{ 257,	257 },	{ 257,	143 }},
	{{ 257,	257 },	{ 280,	257 }},	{{ 257,	143 },	{ 280,	143 }},
	{{ 257,	257 },	{ 280,	280 }},	{{ 257,	143 },	{ 280,	120 }},
	{{ 156,	244 },	{ 156,	156 }},	{{ 156,	244 },	{ 244,	244 }},
	{{ 156,	156 },	{ 244,	156 }},	{{ 143,	257 },	{ 156,	244 }},
	{{ 143,	143 },	{ 156,	156 }},	{{ 143,	244 },	{ 156,	244 }},
	{{ 143,	156 },	{ 156,	156 }},	{{ 244,	244 },	{ 244,	156 }},
	{{ 244,	244 },	{ 257,	244 }},	{{ 244,	156 },	{ 257,	156 }},
	{{ 244,	244 },	{ 257,	257 }},	{{ 244,	156 },	{ 257,	143 }},
	{{ 164,	236 },	{ 164,	164 }},	{{ 164,	236 },	{ 236,	236 }},
	{{ 164,	164 },	{ 236,	164 }},	{{ 156,	244 },	{ 164,	236 }},
	{{ 156,	156 },	{ 164,	164 }},	{{ 156,	236 },	{ 164,	236 }},
	{{ 156,	164 },	{ 164,	164 }},	{{ 236,	236 },	{ 236,	164 }},
	{{ 236,	236 },	{ 244,	236 }},	{{ 236,	164 },	{ 244,	164 }},
	{{ 236,	236 },	{ 244,	244 }},	{{ 236,	164 },	{ 244,	156 }},
	{{ 170,	230 },	{ 170,	170 }},	{{ 170,	230 },	{ 230,	230 }},
	{{ 170,	170 },	{ 230,	170 }},	{{ 164,	236 },	{ 170,	230 }},
	{{ 164,	164 },	{ 170,	170 }},	{{ 164,	230 },	{ 170,	230 }},
	{{ 164,	170 },	{ 170,	170 }},	{{ 230,	230 },	{ 230,	170 }},
	{{ 230,	230 },	{ 236,	230 }},	{{ 230,	170 },	{ 236,	170 }},
	{{ 230,	230 },	{ 236,	236 }},	{{ 230,	170 },	{ 236,	164 }},
	{{ 174,	226 },	{ 174,	174 }},	{{ 174,	226 },	{ 226,	226 }},
	{{ 174,	174 },	{ 226,	174 }},	{{ 170,	230 },	{ 174,	226 }},
	{{ 170,	170 },	{ 174,	174 }},	{{ 170,	226 },	{ 174,	226 }},
	{{ 170,	174 },	{ 174,	174 }},	{{ 226,	226 },	{ 226,	174 }},
	{{ 226,	226 },	{ 230,	226 }},	{{ 226,	174 },	{ 230,	174 }},
	{{ 226,	226 },	{ 230,	230 }},	{{ 226,	174 },	{ 230,	170 }},
	{{ 177,	223 },	{ 177,	177 }},	{{ 177,	223 },	{ 223,	223 }},
	{{ 177,	177 },	{ 223,	177 }},	{{ 174,	226 },	{ 177,	223 }},
	{{ 174,	174 },	{ 177,	177 }},	{{ 174,	223 },	{ 177,	223 }},
	{{ 174,	177 },	{ 177,	177 }},	{{ 223,	223 },	{ 223,	177 }},
	{{ 223,	223 },	{ 226,	223 }},	{{ 223,	177 },	{ 226,	177 }},
	{{ 223,	223 },	{ 226,	226 }},	{{ 223,	177 },	{ 226,	174 }},
	{{ 179,	221 },	{ 179,	179 }},	{{ 179,	221 },	{ 221,	221 }},
	{{ 179,	179 },	{ 221,	179 }},	{{ 177,	223 },	{ 179,	221 }},
	{{ 177,	177 },	{ 179,	179 }},	{{ 177,	221 },	{ 179,	221 }},
	{{ 177,	179 },	{ 179,	179 }},	{{ 221,	221 },	{ 221,	179 }},
	{{ 221,	221 },	{ 223,	221 }},	{{ 221,	179 },	{ 223,	179 }},
	{{ 221,	221 },	{ 223,	223 }},	{{ 221,	179 },	{ 223,	177 }},
	{{ 181,	219 },	{ 181,	181 }},	{{ 181,	219 },	{ 219,	219 }},
	{{ 181,	181 },	{ 219,	181 }},	{{ 179,	221 },	{ 181,	219 }},
	{{ 179,	179 },	{ 181,	181 }},	{{ 179,	219 },	{ 181,	219 }},
	{{ 179,	181 },	{ 181,	181 }},	{{ 219,	219 },	{ 219,	181 }},
	{{ 219,	219 },	{ 221,	219 }},	{{ 219,	181 },	{ 221,	181 }},
	{{ 219,	219 },	{ 221,	221 }},	{{ 219,	181 },	{ 221,	179 }},
	{{ 183,	217 },	{ 183,	183 }},	{{ 183,	217 },	{ 217,	217 }},
	{{ 183,	183 },	{ 217,	183 }},	{{ 181,	219 },	{ 183,	217 }},
	{{ 181,	181 },	{ 183,	183 }},	{{ 181,	217 },	{ 183,	217 }},
	{{ 181,	183 },	{ 183,	183 }},	{{ 217,	217 },	{ 217,	183 }},
	{{ 217,	217 },	{ 219,	217 }},	{{ 217,	183 },	{ 219,	183 }},
	{{ 217,	217 },	{ 219,	219 }},	{{ 217,	183 },	{ 219,	181 }},
	{{ 184,	216 },	{ 184,	184 }},	{{ 184,	216 },	{ 216,	216 }},
	{{ 184,	184 },	{ 216,	184 }},	{{ 183,	217 },	{ 184,	216 }},
	{{ 183,	183 },	{ 184,	184 }},	{{ 183,	216 },	{ 184,	216 }},
	{{ 183,	184 },	{ 184,	184 }},	{{ 216,	216 },	{ 216,	184 }},
	{{ 216,	216 },	{ 217,	216 }},	{{ 216,	184 },	{ 217,	184 }},
	{{ 216,	216 },	{ 217,	217 }},	{{ 216,	184 },	{ 217,	183 }},
	{{ 186,	214 },	{ 186,	186 }},	{{ 186,	214 },	{ 214,	214 }},
	{{ 186,	186 },	{ 214,	186 }},	{{ 184,	216 },	{ 186,	214 }},
	{{ 184,	184 },	{ 186,	186 }},	{{ 184,	214 },	{ 186,	214 }},
	{{ 184,	186 },	{ 186,	186 }},	{{ 214,	214 },	{ 214,	186 }},
	{{ 214,	214 },	{ 216,	214 }},	{{ 214,	186 },	{ 216,	186 }},
	{{ 214,	214 },	{ 216,	216 }},	{{ 214,	186 },	{ 216,	184 }},
	{{ 187,	213 },	{ 187,	187 }},	{{ 187,	213 },	{ 213,	213 }},
	{{ 187,	187 },	{ 213,	187 }},	{{ 186,	214 },	{ 187,	213 }},
	{{ 186,	186 },	{ 187,	187 }},	{{ 186,	213 },	{ 187,	213 }},
	{{ 186,	187 },	{ 187,	187 }},	{{ 213,	213 },	{ 213,	187 }},
	{{ 213,	213 },	{ 214,	213 }},	{{ 213,	187 },	{ 214,	187 }},
	{{ 213,	213 },	{ 214,	214 }},	{{ 213,	187 },	{ 214,	186 }},
	{{ 188,	212 },	{ 188,	188 }},	{{ 188,	212 },	{ 212,	212 }},
	{{ 188,	188 },	{ 212,	188 }},	{{ 187,	213 },	{ 188,	212 }},
	{{ 187,	187 },	{ 188,	188 }},	{{ 187,	212 },	{ 188,	212 }},
	{{ 187,	188 },	{ 188,	188 }},	{{ 212,	212 },	{ 212,	188 }},
	{{ 212,	212 },	{ 213,	212 }},	{{ 212,	188 },	{ 213,	188 }},
	{{ 212,	212 },	{ 213,	213 }},	{{ 212,	188 },	{ 213,	187 }},
	{{ 188,	212 },	{ 188,	188 }},	{{ 188,	212 },	{ 212,	212 }},
	{{ 188,	188 },	{ 212,	188 }},	{{ 188,	212 },	{ 188,	212 }},
	{{ 188,	188 },	{ 188,	188 }},	{{ 188,	212 },	{ 188,	212 }},
	{{ 188,	188 },	{ 188,	188 }},	{{ 212,	212 },	{ 212,	188 }},
	{{ 212,	212 },	{ 212,	212 }},	{{ 212,	188 },	{ 212,	188 }},
	{{ 212,	212 },	{ 212,	212 }},	{{ 212,	188 },	{ 212,	188 }},
	{{ 189,	211 },	{ 189,	189 }},	{{ 189,	211 },	{ 211,	211 }},
	{{ 189,	189 },	{ 211,	189 }},	{{ 188,	212 },	{ 189,	211 }},
	{{ 188,	188 },	{ 189,	189 }},	{{ 188,	211 },	{ 189,	211 }},
	{{ 188,	189 },	{ 189,	189 }},	{{ 211,	211 },	{ 211,	189 }},
	{{ 211,	211 },	{ 212,	211 }},	{{ 211,	189 },	{ 212,	189 }},
	{{ 211,	211 },	{ 212,	212 }},	{{ 211,	189 },	{ 212,	188 }},
	{{ 190,	210 },	{ 190,	190 }},	{{ 190,	210 },	{ 210,	210 }},
	{{ 190,	190 },	{ 210,	190 }},	{{ 189,	211 },	{ 190,	210 }},
	{{ 189,	189 },	{ 190,	190 }},	{{ 189,	210 },	{ 190,	210 }},
	{{ 189,	190 },	{ 190,	190 }},	{{ 210,	210 },	{ 210,	190 }},
	{{ 210,	210 },	{ 211,	210 }},	{{ 210,	190 },	{ 211,	190 }},
	{{ 210,	210 },	{ 211,	211 }},	{{ 210,	190 },	{ 211,	189 }},
	{{ 190,	210 },	{ 190,	190 }},	{{ 190,	210 },	{ 210,	210 }},
	{{ 190,	190 },	{ 210,	190 }},	{{ 190,	210 },	{ 190,	210 }},
	{{ 190,	190 },	{ 190,	190 }},	{{ 190,	210 },	{ 190,	210 }},
	{{ 190,	190 },	{ 190,	190 }},	{{ 210,	210 },	{ 210,	190 }},
	{{ 210,	210 },	{ 210,	210 }},	{{ 210,	190 },	{ 210,	190 }},
	{{ 210,	210 },	{ 210,	210 }},	{{ 210,	190 },	{ 210,	190 }},
	{{ 191,	209 },	{ 191,	191 }},	{{ 191,	209 },	{ 209,	209 }},
	{{ 191,	191 },	{ 209,	191 }},	{{ 190,	210 },	{ 191,	209 }},
	{{ 190,	190 },	{ 191,	191 }},	{{ 190,	209 },	{ 191,	209 }},
	{{ 190,	191 },	{ 191,	191 }},	{{ 209,	209 },	{ 209,	191 }},
	{{ 209,	209 },	{ 210,	209 }},	{{ 209,	191 },	{ 210,	191 }},
	{{ 209,	209 },	{ 210,	210 }},	{{ 209,	191 },	{ 210,	190 }},
	{{ 191,	209 },	{ 191,	191 }},	{{ 191,	209 },	{ 209,	209 }},
	{{ 191,	191 },	{ 209,	191 }},	{{ 191,	209 },	{ 191,	209 }},
	{{ 191,	191 },	{ 191,	191 }},	{{ 191,	209 },	{ 191,	209 }},
	{{ 191,	191 },	{ 191,	191 }},	{{ 209,	209 },	{ 209,	191 }},
	{{ 209,	209 },	{ 209,	209 }},	{{ 209,	191 },	{ 209,	191 }},
	{{ 209,	209 },	{ 209,	209 }},	{{ 209,	191 },	{ 209,	191 }},
	{{ 192,	208 },	{ 192,	192 }},	{{ 192,	208 },	{ 208,	208 }},
	{{ 192,	192 },	{ 208,	192 }},	{{ 191,	209 },	{ 192,	208 }},
	{{ 191,	191 },	{ 192,	192 }},	{{ 191,	208 },	{ 192,	208 }},
	{{ 191,	192 },	{ 192,	192 }},	{{ 208,	208 },	{ 208,	192 }},
	{{ 208,	208 },	{ 209,	208 }},	{{ 208,	192 },	{ 209,	192 }},
	{{ 208,	208 },	{ 209,	209 }},	{{ 208,	192 },	{ 209,	191 }},
	{{ 192,	208 },	{ 192,	192 }},	{{ 192,	208 },	{ 208,	208 }},
	{{ 192,	192 },	{ 208,	192 }},	{{ 192,	208 },	{ 192,	208 }},
	{{ 192,	192 },	{ 192,	192 }},	{{ 192,	208 },	{ 192,	208 }},
	{{ 192,	192 },	{ 192,	192 }},	{{ 208,	208 },	{ 208,	192 }},
	{{ 208,	208 },	{ 208,	208 }},	{{ 208,	192 },	{ 208,	192 }},
	{{ 208,	208 },	{ 208,	208 }},	{{ 208,	192 },	{ 208,	192 }},
	{{ 192,	208 },	{ 192,	192 }},	{{ 192,	208 },	{ 208,	208 }},
	{{ 192,	192 },	{ 208,	192 }},	{{ 192,	208 },	{ 192,	208 }},
	{{ 192,	192 },	{ 192,	192 }},	{{ 192,	208 },	{ 192,	208 }},
	{{ 192,	192 },	{ 192,	192 }},	{{ 208,	208 },	{ 208,	192 }},
	{{ 208,	208 },	{ 208,	208 }},	{{ 208,	192 },	{ 208,	192 }},
	{{ 208,	208 },	{ 208,	208 }},	{{ 208,	192 },	{ 208,	192 }},
	{{ 193,	207 },	{ 193,	193 }},	{{ 193,	207 },	{ 207,	207 }},
	{{ 193,	193 },	{ 207,	193 }},	{{ 192,	208 },	{ 193,	207 }},
	{{ 192,	192 },	{ 193,	193 }},	{{ 192,	207 },	{ 193,	207 }},
	{{ 192,	193 },	{ 193,	193 }},	{{ 207,	207 },	{ 207,	193 }},
	{{ 207,	207 },	{ 208,	207 }},	{{ 207,	193 },	{ 208,	193 }},
	{{ 207,	207 },	{ 208,	208 }},	{{ 207,	193 },	{ 208,	192 }},
	{{ 193,	207 },	{ 193,	193 }},	{{ 193,	207 },	{ 207,	207 }},
	{{ 193,	193 },	{ 207,	193 }},	{{ 193,	207 },	{ 193,	207 }},
	{{ 193,	193 },	{ 193,	193 }},	{{ 193,	207 },	{ 193,	207 }},
	{{ 193,	193 },	{ 193,	193 }},	{{ 207,	207 },	{ 207,	193 }},
	{{ 207,	207 },	{ 207,	207 }},	{{ 207,	193 },	{ 207,	193 }},
	{{ 207,	207 },	{ 207,	207 }},	{{ 207,	193 },	{ 207,	193 }},
	{{ 193,	207 },	{ 193,	193 }},	{{ 193,	207 },	{ 207,	207 }},
	{{ 193,	193 },	{ 207,	193 }},	{{ 193,	207 },	{ 193,	207 }},
	{{ 193,	193 },	{ 193,	193 }},	{{ 193,	207 },	{ 193,	207 }},
	{{ 193,	193 },	{ 193,	193 }},	{{ 207,	207 },	{ 207,	193 }},
	{{ 207,	207 },	{ 207,	207 }},	{{ 207,	193 },	{ 207,	193 }},
	{{ 207,	207 },	{ 207,	207 }},	{{ 207,	193 },	{ 207,	193 }},
	{{ 193,	207 },	{ 193,	193 }},	{{ 193,	207 },	{ 207,	207 }},
	{{ 193,	193 },	{ 207,	193 }},	{{ 193,	207 },	{ 193,	207 }},
	{{ 193,	193 },	{ 193,	193 }},	{{ 193,	207 },	{ 193,	207 }},
	{{ 193,	193 },	{ 193,	193 }},	{{ 207,	207 },	{ 207,	193 }},
	{{ 207,	207 },	{ 207,	207 }},	{{ 207,	193 },	{ 207,	193 }},
	{{ 207,	207 },	{ 207,	207 }},	{{ 207,	193 },	{ 207,	193 }},
	{{ 194,	206 },	{ 194,	194 }},	{{ 194,	206 },	{ 206,	206 }},
	{{ 194,	194 },	{ 206,	194 }},	{{ 193,	207 },	{ 194,	206 }},
	{{ 193,	193 },	{ 194,	194 }},	{{ 193,	206 },	{ 194,	206 }},
	{{ 193,	194 },	{ 194,	194 }},	{{ 206,	206 },	{ 206,	194 }},
	{{ 206,	206 },	{ 207,	206 }},	{{ 206,	194 },	{ 207,	194 }},
	{{ 206,	206 },	{ 207,	207 }},	{{ 206,	194 },	{ 207,	193 }},
	{{ 194,	206 },	{ 194,	194 }},	{{ 194,	206 },	{ 206,	206 }},
	{{ 194,	194 },	{ 206,	194 }},	{{ 194,	206 },	{ 194,	206 }},
	{{ 194,	194 },	{ 194,	194 }},	{{ 194,	206 },	{ 194,	206 }},
	{{ 194,	194 },	{ 194,	194 }},	{{ 206,	206 },	{ 206,	194 }},
	{{ 206,	206 },	{ 206,	206 }},	{{ 206,	194 },	{ 206,	194 }},
	{{ 206,	206 },	{ 206,	206 }},	{{ 206,	194 },	{ 206,	194 }},
};

#define ratBits_width 384
#define ratBits_height 64
static short	ratBits[] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,

    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,

    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0007, 0xe000, 0x0007, 0xe000, 0x003c, 0x0000, 0x3c00, 0x0df0,

    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x001f, 0xf800, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x003f, 0xfc00, 0x003f, 0xfc00, 0x008f, 0x0000, 0x9f00, 0x13f8,

    0x0000, 0x001f, 0xf800, 0x0000, 0x0000, 0x01ff, 0xff80, 0x0000,
    0x0000, 0x001f, 0xf800, 0x0000, 0x0000, 0x001f, 0xf800, 0x0000,
    0x00f3, 0xff00, 0x00f3, 0xff00, 0x037f, 0xc003, 0x7fc0, 0x2ffc,

    0x0000, 0x01ff, 0xff80, 0x0000, 0x0000, 0x0fff, 0xfff0, 0x0000,
    0x0000, 0x01ff, 0xff80, 0x0000, 0x0000, 0x01ff, 0xff80, 0x0000,
    0x01c7, 0xff80, 0x01c7, 0xff80, 0x04ff, 0xe004, 0xffe0, 0x2ffc,

    0x0000, 0x0fff, 0xfff0, 0x0000, 0x0000, 0x3fff, 0xfffc, 0x0000,
    0x0000, 0x0fff, 0xfff0, 0x0000, 0x0000, 0x0fff, 0xfff0, 0x0000,
    0x031f, 0xffc0, 0x031f, 0xffc0, 0x0bff, 0xf00b, 0xfff0, 0x47fe,

    0x0000, 0x3fff, 0xfffc, 0x0000, 0x0000, 0xffe3, 0xffff, 0x0000,
    0x0000, 0x3fff, 0xfffc, 0x0000, 0x0000, 0x3fff, 0xfffc, 0x0000,
    0x067f, 0xffe0, 0x067f, 0xffe0, 0x0bff, 0xf00f, 0xfff0, 0x63fe,

    0x0000, 0xffe3, 0xffff, 0x0000, 0x0001, 0xe79f, 0xffff, 0x8000,
    0x0000, 0xffe3, 0xffff, 0x0000, 0x0000, 0xff8f, 0xffff, 0x0000,
    0x0cff, 0xfff0, 0x0cff, 0xfff0, 0x11ff, 0xf81f, 0xfff8, 0x63fe,

    0x0001, 0xe79f, 0xffff, 0x8000, 0x0003, 0xf03f, 0xffff, 0xc000,
    0x0001, 0xe79f, 0xffff, 0x8000, 0x0001, 0xe60f, 0xffff, 0x8000,
    0x0bff, 0xfff0, 0x0bff, 0xfff0, 0x10ff, 0xf81f, 0xfff8, 0x47fe,

    0x0003, 0xf03f, 0xffff, 0xc000, 0x0007, 0xc07f, 0xffff, 0xe000,
    0x0003, 0xf03f, 0xffff, 0xc000, 0x0003, 0xf83f, 0xffff, 0xc000,
    0x19ff, 0xfff8, 0x1fff, 0xfff8, 0x307f, 0xfc3f, 0xfffc, 0x2ffc,

    0x0007, 0xc07f, 0xffff, 0xe000, 0x000d, 0x01ff, 0xffff, 0xf000,
    0x0007, 0xc07f, 0xffff, 0xe000, 0x0007, 0xc03f, 0xffff, 0xe000,
    0x10ff, 0xfff8, 0x1fff, 0xfff8, 0x383f, 0xfc3f, 0xfffc, 0x3ffc,

    0x000d, 0x01ff, 0xffff, 0xf000, 0x0018, 0x0dff, 0xffff, 0xf800,
    0x000d, 0x01ff, 0xffff, 0xf000, 0x000f, 0x01ff, 0xffff, 0xf000,
    0x087f, 0xfff8, 0x1fff, 0xfff8, 0x383f, 0xfc3f, 0xfffc, 0x1ff8,

    0x0018, 0x0dff, 0xffff, 0xf800, 0x003c, 0x3eff, 0xffff, 0xfc00,
    0x0018, 0x0dff, 0xffff, 0xf800, 0x001c, 0x0dff, 0xffff, 0xf800,
    0x183f, 0xfffc, 0x3fff, 0xfffc, 0x307f, 0xfc3f, 0xfffc, 0x1bdc,

    0x003c, 0x3eff, 0xffff, 0xfc00, 0x0078, 0xfcff, 0xffff, 0xfe00,
    0x003c, 0x3eff, 0xffff, 0xfc00, 0x003c, 0x3eff, 0xffff, 0xfc00,
    0x1c1f, 0xfffc, 0x3fff, 0xfffc, 0x10ff, 0xf81f, 0xfff8, 0x2e3a,

    0x0078, 0xfcff, 0xffff, 0xfe00, 0x00f1, 0x7eff, 0xffff, 0xff00,
    0x0078, 0xfcff, 0xffff, 0xfe00, 0x0078, 0xfcff, 0xffff, 0xfe00,
    0x3c0f, 0xfffc, 0x3fff, 0xfffc, 0x11ff, 0xf81f, 0xfff8, 0x17f4,

    0x00f1, 0x7eff, 0xffff, 0xff00, 0x00e3, 0x9fff, 0xffff, 0xff00,
    0x00f1, 0x7eff, 0xffff, 0xff00, 0x00f1, 0x7eff, 0xffff, 0xff00,
    0x3c0f, 0xfffc, 0x3fff, 0xfffc, 0x0fff, 0xf00f, 0xfff0, 0x0000,

    0x00e3, 0x9fff, 0xffff, 0xff00, 0x01ef, 0xcfff, 0xffff, 0xff80,
    0x00e3, 0x9fff, 0xffff, 0xff00, 0x00e3, 0x9fff, 0xffff, 0xff00,
    0x3c1f, 0xfffc, 0x3fff, 0xfffc, 0x0fff, 0xf00f, 0xfff0, 0x03c0,

    0x01ef, 0xcfff, 0xffff, 0xff80, 0x01df, 0xffff, 0xffff, 0xfc80,
    0x01ef, 0xcfff, 0xffff, 0xff80, 0x01e7, 0xcfff, 0xffff, 0xff80,
    0x383f, 0xfffc, 0x3fff, 0xfffc, 0x07ff, 0xe007, 0xffe0, 0x0df0,

    0x013f, 0xffff, 0xffff, 0xff80, 0x03ff, 0xffff, 0xffff, 0xc0c0,
    0x01df, 0xffff, 0xffff, 0xff80, 0x01df, 0xffff, 0xffff, 0xff80,
    0x187f, 0xfff8, 0x1fff, 0xfff8, 0x03ff, 0xc003, 0xffc0, 0x13f8,

    0x0303, 0xffff, 0xffff, 0xffc0, 0x03ff, 0xffff, 0xfffe, 0x0040,
    0x03ff, 0xffff, 0xffff, 0xffc0, 0x03ff, 0xffe0, 0x07ff, 0xffc0,
    0x10ff, 0xfff8, 0x1fff, 0xfff8, 0x0b7e, 0x680b, 0x7e68, 0x2ffc,

    0x0200, 0x7fff, 0xffff, 0xffc0, 0x03ff, 0xffff, 0xfff8, 0x0040,
    0x03ff, 0xffff, 0xffff, 0xffc0, 0x03ff, 0xff00, 0x00ff, 0xffc0,
    0x19ff, 0xfff8, 0x1fff, 0xfff8, 0x15f9, 0xd415, 0xf9d4, 0x3ff4,

    0x0200, 0x1fff, 0xffff, 0xffc0, 0x07ff, 0xffff, 0xffe0, 0x0060,
    0x03ff, 0xffff, 0xffff, 0xffc0, 0x03ff, 0xf800, 0x001f, 0xffc0,
    0x0fff, 0xfff0, 0x0fff, 0xfff0, 0x0aae, 0xa80a, 0xaea8, 0x7fe2,

    0x0600, 0x07ff, 0xffff, 0xffe0, 0x07ff, 0xffff, 0xffc0, 0x00e0,
    0x07ff, 0xffff, 0xffff, 0xffe0, 0x07ff, 0xc000, 0x0003, 0xffe0,
    0x0fff, 0xfff0, 0x0fff, 0xfff0, 0x0155, 0x4001, 0x5540, 0x7fc6,

    0x0700, 0xcfff, 0xffff, 0xffe0, 0x07ff, 0xffff, 0xff0a, 0x00e0,
    0x07ff, 0xffff, 0xffff, 0xffe0, 0x07ff, 0x0007, 0xe000, 0xffe0,
    0x07ff, 0xff60, 0x07ff, 0xff60, 0x0000, 0x0000, 0x0000, 0x7fc6,

    0x0700, 0x30ff, 0xffff, 0xffe0, 0x07ff, 0xffff, 0xfe0c, 0x00e0,
    0x07ff, 0xffff, 0xffff, 0xffe0, 0x07fc, 0x000d, 0xb800, 0x3fe0,
    0x03ff, 0xfcc0, 0x03ff, 0xfcc0, 0x0000, 0x0000, 0x0000, 0x7fe2,

    0x0700, 0x407f, 0xffff, 0xffe0, 0x0fff, 0xffff, 0xfc38, 0x01f0,
    0x07ff, 0xffff, 0xffff, 0xffe0, 0x07f0, 0x0005, 0xbc00, 0x0fe0,
    0x07ff, 0xff68, 0x07ff, 0xff68, 0x003c, 0x0000, 0x3c00, 0x3ff4,

    0x0f81, 0x003f, 0xffff, 0xfff0, 0x0fff, 0xffff, 0xf875, 0x81b0,
    0x0fff, 0xffff, 0xffff, 0xfff0, 0x0fc0, 0x0003, 0x7602, 0x03f0,
    0x15ff, 0xf9d4, 0x15ff, 0xf9d4, 0x009f, 0x0000, 0x9f00, 0x3ffc,

    0x0d80, 0x021f, 0xffff, 0xfff0, 0x0fff, 0xffff, 0xf382, 0x41f0,
    0x0fff, 0xffff, 0xffff, 0xfff0, 0x0fc1, 0x0043, 0xce00, 0x8df0,
    0x2adf, 0xe3aa, 0x2adf, 0xe3aa, 0x037f, 0xc003, 0x7fc0, 0x1ff8,

    0x0f80, 0x01cf, 0xffff, 0xfff0, 0x0fff, 0xffff, 0xee00, 0x0170,
    0x0fff, 0xffff, 0xffff, 0xfff0, 0x0f2a, 0x00c3, 0xef00, 0x70f0,
    0x157c, 0x3f54, 0x157c, 0x3f54, 0x04ff, 0xe004, 0xffe0, 0x1bdc,

    0x0e80, 0x6677, 0xffff, 0xfff0, 0x0fff, 0xffff, 0xfc00, 0x0170,
    0x0fff, 0xffff, 0xffff, 0xfff0, 0x0e6d, 0x0091, 0xf900, 0x8070,
    0x0aaf, 0xfaa8, 0x0aaf, 0xfaa8, 0x0bff, 0xf00b, 0xfff0, 0x2e3a,

    0x0e80, 0x183f, 0xffff, 0xfff0, 0x0fff, 0xffff, 0xc000, 0x0170,
    0x0fff, 0xffff, 0xffff, 0xfff0, 0x0c10, 0x00f8, 0xff00, 0x03b0,
    0x0155, 0x5540, 0x0155, 0x5540, 0x0fff, 0xd00f, 0x81f0, 0x17f4,

    0x0e80, 0x0003, 0xffff, 0xfff0, 0x0fff, 0xffff, 0xe000, 0x01f0,
    0x0fff, 0xffff, 0xffff, 0xfff0, 0x0c0c, 0x00ff, 0xf700, 0x25f0,
    0x0000, 0x0000, 0x0000, 0x0000, 0x1fff, 0x881e, 0x0078, 0x0000,

    0x0f80, 0x0007, 0xffff, 0xfff0, 0x0fff, 0xffff, 0xf000, 0x01f0,
    0x0fff, 0xffff, 0xffff, 0xfff0, 0x0e02, 0x0097, 0xf900, 0x1c70,
    0x0000, 0x0000, 0x0000, 0x0000, 0x1fff, 0x0818, 0x1818, 0x03c0,

    0x0f80, 0x000f, 0xffff, 0xfff0, 0x0fff, 0xffff, 0xf800, 0x01f0,
    0x0fff, 0xffff, 0xffff, 0xfff0, 0x0f01, 0x00fb, 0xe700, 0x00f0,
    0x0007, 0xe000, 0x0007, 0xe000, 0x3ffe, 0x0c30, 0x1c0c, 0x0df0,

    0x0f80, 0x601f, 0xffff, 0xfff0, 0x0fff, 0xffff, 0xfc00, 0x01b0,
    0x0fff, 0xffff, 0xffff, 0xfff0, 0x0f80, 0x0075, 0xde00, 0x51f0,
    0x003f, 0xfc00, 0x003f, 0xfc00, 0x3ffc, 0x1c20, 0x4e04, 0x13f8,

    0x0d80, 0x91ff, 0xffff, 0xfff0, 0x07ff, 0xffff, 0xfe00, 0x01e0,
    0x0fff, 0xffff, 0xffff, 0xfff0, 0x0fc0, 0x006f, 0x1604, 0x8bf0,
    0x00f3, 0xff00, 0x00f3, 0xff00, 0x3ffc, 0x1c20, 0x6e04, 0x2ffc,

    0x0780, 0x0f7f, 0xffff, 0xffe0, 0x07ff, 0xffff, 0xff00, 0x00e0,
    0x07ff, 0xffff, 0xffff, 0xffe0, 0x07f0, 0x0039, 0x7c03, 0x07e0,
    0x01c7, 0xff80, 0x01c7, 0xff80, 0x3ffe, 0x0c30, 0x7e0c, 0x3ffc,

    0x0700, 0x00ff, 0xffff, 0xffe0, 0x07ff, 0xffff, 0xffc0, 0x00e0,
    0x07ff, 0xffff, 0xffff, 0xffe0, 0x07fc, 0x001d, 0xb810, 0x1fe0,
    0x031f, 0xffc0, 0x031f, 0xffc0, 0x1fff, 0x0818, 0x3c18, 0x7ffe,

    0x0710, 0x03ff, 0xffff, 0xffe0, 0x07ff, 0xffff, 0xffe0, 0x00e0,
    0x07ff, 0xffff, 0xffff, 0xffe0, 0x07ff, 0x0007, 0xe208, 0x7fe0,
    0x067f, 0xffe0, 0x067f, 0xffe0, 0x1fff, 0x881e, 0x1878, 0x7ffe,

    0x0708, 0x07ff, 0xffff, 0xffe0, 0x03ff, 0xffff, 0xfff8, 0x00c0,
    0x07ff, 0xffff, 0xffff, 0xffe0, 0x07ff, 0xc000, 0x0191, 0xffe0,
    0x0cff, 0xfff0, 0x0cff, 0xfff0, 0x0fff, 0xf00f, 0x81f0, 0x7ffe,

    0x0228, 0x1fff, 0xffff, 0xffc0, 0x03ff, 0xffff, 0xfffe, 0x01c0,
    0x03ff, 0xffff, 0xffff, 0xffc0, 0x03ff, 0xf800, 0x0077, 0xffc0,
    0x0bff, 0xfff0, 0x0bff, 0xfff0, 0x0fff, 0xf00f, 0xfff0, 0x7ffe,

    0x0214, 0x7fff, 0xffff, 0xffc0, 0x03ff, 0xffff, 0xffff, 0xc1c0,
    0x03ff, 0xffff, 0xffff, 0xffc0, 0x03ff, 0xff00, 0x001f, 0xffc0,
    0x1fff, 0xff98, 0x1ff0, 0x0ff8, 0x07ff, 0xe007, 0xffe0, 0x3ffc,

    0x0203, 0xffff, 0xffff, 0xffc0, 0x01ff, 0xffff, 0xffff, 0xf980,
    0x03ff, 0xffff, 0xffff, 0xffc0, 0x03ff, 0xff00, 0x00ff, 0xffc0,
    0x1fff, 0xff08, 0x1f80, 0x01f8, 0x03ff, 0xc003, 0xffc0, 0x3ffc,

    0x011f, 0xffff, 0xffff, 0xff80, 0x01ff, 0xffff, 0xffff, 0xff80,
    0x01ff, 0xffff, 0xffff, 0xff80, 0x01ff, 0xffe0, 0x07ff, 0xff80,
    0x1fff, 0xfe18, 0x1e03, 0xc078, 0x0b7e, 0x680b, 0x7e68, 0x1ff8,

    0x01ff, 0xffff, 0xffff, 0xff80, 0x00ff, 0xffff, 0xffff, 0xdf00,
    0x01ff, 0xffff, 0xffff, 0xff80, 0x01ff, 0xffff, 0xffff, 0xff80,
    0x3fff, 0xfc1c, 0x3801, 0xe01c, 0x15f9, 0xd415, 0xf9d4, 0x1bd4,

    0x00fb, 0xffff, 0xffff, 0xff00, 0x00ff, 0xffff, 0xffff, 0xff00,
    0x00ff, 0xffff, 0xffff, 0xdf00, 0x00ff, 0xffff, 0xffff, 0xdf00,
    0x3fff, 0xf83c, 0x3009, 0xf00c, 0x0aae, 0xa80a, 0xaea8, 0x2e2a,

    0x00ff, 0xffff, 0xffff, 0xff00, 0x007f, 0xffff, 0xffff, 0xbe00,
    0x00ff, 0xffff, 0xffff, 0xff00, 0x00ff, 0xffff, 0xffff, 0xff00,
    0x3fff, 0xf03c, 0x200c, 0xf004, 0x0155, 0x4001, 0x5540, 0x17d4,

    0x007f, 0xffff, 0xffff, 0xbe00, 0x003f, 0xffff, 0xffff, 0x7c00,
    0x007f, 0xffff, 0xffff, 0xbe00, 0x007f, 0xffff, 0xffff, 0xbe00,
    0x3fff, 0xf03c, 0x200f, 0xf004, 0x3e1f, 0x0f87, 0xc000, 0x0000,

    0x003f, 0xffff, 0xffff, 0x7c00, 0x001f, 0xffff, 0xfff8, 0xf800,
    0x003f, 0xffff, 0xffff, 0x7c00, 0x003f, 0xffff, 0xffff, 0x7c00,
    0x3fff, 0xf83c, 0x300f, 0xf00c, 0x7f3f, 0x9fcf, 0xe000, 0x03c0,

    0x001f, 0xffff, 0xfff8, 0xf800, 0x000f, 0xffff, 0xffe1, 0xf000,
    0x001f, 0xffff, 0xfff8, 0xf800, 0x001f, 0xffff, 0xfff8, 0xf800,
    0x3fff, 0xfc1c, 0x3807, 0xe01c, 0xffff, 0xfffe, 0xf000, 0x0df0,

    0x000f, 0xffff, 0xffe1, 0xf000, 0x0003, 0xffff, 0xff07, 0xa000,
    0x000f, 0xffff, 0xffe1, 0xf000, 0x000f, 0xffff, 0xffe1, 0xf000,
    0x1fff, 0xfe18, 0x1e03, 0xc078, 0x3fff, 0x3ff8, 0x3000, 0x13f8,

    0x0003, 0xffff, 0xff07, 0xa000, 0x0007, 0xffff, 0xf00e, 0x4000,
    0x0003, 0xffff, 0xff07, 0xa000, 0x0003, 0xffff, 0xff07, 0xa000,
    0x1fff, 0xff08, 0x1f80, 0x01f8, 0x8ffc, 0x7ff1, 0x1ad2, 0x2ffc,

    0x0007, 0xffff, 0xf00e, 0x4000, 0x00ab, 0xffff, 0xfc7d, 0xff00,
    0x0007, 0xffff, 0xf00e, 0x4000, 0x0007, 0xffff, 0xf00e, 0x4000,
    0x1fff, 0xff98, 0x1ff0, 0x0ff8, 0x3fff, 0x3ff8, 0x3000, 0x381c,

    0x00ab, 0xffff, 0xfc7d, 0xff00, 0x0155, 0xffff, 0xffe3, 0xd540,
    0x00ab, 0xffff, 0xfc7d, 0xff00, 0x00ab, 0xffff, 0xfc7d, 0xff00,
    0x0fff, 0xfff0, 0x0fff, 0xfff0, 0xffff, 0xfffe, 0xf000, 0x6186,

    0x0155, 0xffff, 0xffe3, 0xd540, 0x02aa, 0xff7f, 0xffcf, 0xaaa0,
    0x0155, 0xffff, 0xffe3, 0xd540, 0x0155, 0xffff, 0xffe3, 0xd540,
    0x0fff, 0xfff0, 0x0fff, 0xfff0, 0x7f3f, 0x9fcf, 0xe000, 0x42c2,

    0x02aa, 0xff7f, 0xffcf, 0xaaa0, 0x0555, 0x5f3f, 0xfc7f, 0x5550,
    0x02aa, 0xff7f, 0xffcf, 0xaaa0, 0x02aa, 0xff7f, 0xffcf, 0xaaa0,
    0x07ff, 0xff60, 0x07ff, 0xff60, 0x3e1f, 0x0f87, 0xc000, 0x43c2,

    0x0555, 0x5f3f, 0xfc7f, 0x5550, 0x0aaa, 0xaff3, 0xcffe, 0xaaa0,
    0x0555, 0x5f3f, 0xfc7f, 0x5550, 0x0555, 0x5f3f, 0xfc7f, 0x5550,
    0x03ff, 0xfcc0, 0x03ff, 0xfcc0, 0x30c3, 0x0c49, 0x2000, 0x6186,

    0x0aaa, 0xaff3, 0xcffe, 0xaaa0, 0x0555, 0x57f8, 0x1ff5, 0x5540,
    0x0aaa, 0xaff3, 0xcffe, 0xaaa0, 0x0aaa, 0xaff3, 0xcffe, 0xaaa0,
    0x07ff, 0xff68, 0x07ff, 0xff68, 0x79e7, 0x9e33, 0xd000, 0x381c,

    0x0555, 0x57f8, 0x1ff5, 0x5540, 0x02aa, 0xaaff, 0xffaa, 0xaa80,
    0x0555, 0x57f8, 0x1ff5, 0x5540, 0x0555, 0x57f8, 0x1ff5, 0x5540,
    0x15ff, 0xf9d4, 0x15ff, 0xf9d4, 0x3fcf, 0xf349, 0x2000, 0x3ffc,

    0x02aa, 0xaaff, 0xffaa, 0xaa80, 0x0055, 0x555f, 0xf555, 0x5500,
    0x02aa, 0xaaff, 0xffaa, 0xaa80, 0x02aa, 0xaaff, 0xffaa, 0xaa80,
    0x2adf, 0xe3aa, 0x2adf, 0xe3aa, 0x3fcf, 0xf366, 0x6600, 0x1ff8,

    0x0055, 0x555f, 0xf555, 0x5500, 0x000a, 0xaaaa, 0xaaaa, 0xa000,
    0x0055, 0x555f, 0xf555, 0x5500, 0x0055, 0x555f, 0xf555, 0x5500,
    0x157c, 0x3f54, 0x157c, 0x3f54, 0x79e7, 0x9e3c, 0xf900, 0x13d4,

    0x000a, 0xaaaa, 0xaaaa, 0xa000, 0x0000, 0x0555, 0x5550, 0x0000,
    0x000a, 0xaaaa, 0xaaaa, 0xa000, 0x000a, 0xaaaa, 0xaaaa, 0xa000,
    0x0aaf, 0xfaa8, 0x0aaf, 0xfaa8, 0x30c3, 0x0c3c, 0xf900, 0x2a2a,

    0x0000, 0x0555, 0x5550, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0555, 0x5550, 0x0000, 0x0000, 0x0555, 0x5550, 0x0000,
    0x0155, 0x5540, 0x0155, 0x5540, 0x0000, 0x0066, 0x6600, 0x1554
};

static RelativeTokens	relativeTokens = {
	{ REAR,	FRONT, RIGHT, LEFT },	/* me north, him [n, s, e, w] */
	{ FRONT, REAR, LEFT, RIGHT },	/* me south, him [n, s, e, w] */
	{ LEFT, RIGHT, REAR, FRONT },	/* me east ... */
	{ RIGHT, LEFT, FRONT, REAR }	/* me west ... */
};

/* 
 * Initialize the display manager.
 */

InitDisplay(argc, argv)
char **argv;
{
	register int	i;
	register RatState	*rsp;

	for (i = 0; i < MAXRATS; i++) {
		rsp = &clearArray[i];
		rsp->playing = FALSE;
		rsp->x = 1;
		rsp->y = 1;
		rsp->dir = NORTH;
	}

	InitWindow(argc, argv);
}

/*
 * all in the name of portability... actually put the display on
 * the screen. Some window systems need to do initialization before
 * the net is started, but don't want to display until after.
 */

StartDisplay()
{
	StartWindow(ratBits, ratBits_width, ratBits_height);
}

/* 
 * Manage the top portion of the screen, the perspective view of the
 * maze, with the eyeballs. What is seen is controlled by the position
 * in the maze and what players are visible. This latter is recorded
 * in the R2d2 array (the significance of the name is lost to
 * history), which is recomputed every time someone moves.
 *
 * The perspective view is calculated on the fly from the viewTable array.
 * viewTable contains a set of 12 line segments for 30 views. The longest
 * corridor is 30 spaces, and for each cell in the maze, there are 12
 * distinct lines (wall edges) that could be visible (never all at once).
 *
 * Each time, march down the hall in the direction faced and figure
 * out which lines to actually draw. Then look in R2d2 and draw in opponents.
 */

#define	NDIR	NDIRECTION
static Boolean	prevEdge3, prevEdge7;	/* plotter smarts */
static Boolean	edge1, edge2, edge3, edge4, edge5, edge6, edge7;
static XYpair	edge3Lines[2], edge7Lines[2];
static XY	l1Delta[NDIR] = { {0, -1}, {0, 1}, {1, 0}, {-1, 0} };
static XY	l2Delta[NDIR] = { {1, -1}, {-1, 1}, {1, 1}, {-1, -1} };
static XY	c2Delta[NDIR] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
static XY	r1Delta[NDIR] = { {0, 1}, {0, -1}, {-1, 0}, {1, 0} };
static XY	r2Delta[NDIR] = { {1, 1}, {-1, -1}, {-1, 1}, {1, -1} };
#undef	NDIR

ShowView(x, y, dir)
int		x, y;
Direction	dir;
{
	register XYpair		*tp = viewTable;
	register int		tx = x;
	register int		ty = y;
	RatId			ratId;
	RatLook			ratLook;
	Boolean			oldVisible;
	XYpair			*hidden(), *plotLine();

	ClearView();
	prevEdge3 = prevEdge7 = FALSE;
	while (!M.maze[tx].y[ty]) {
		tp = hidden(tx, ty, dir, tp);	/* draw a cell */
		switch (dir) {
		case NORTH:	tx++; break;
		case SOUTH:	tx--; break;
		case EAST:	ty++; break;
		case WEST:	ty--; break;
		}
	}
	if (prevEdge3)
		(void) plotLine(edge3Lines, TRUE);
	if (prevEdge7)
		(void) plotLine(edge7Lines, TRUE);

	/* show the tokens */

	for (ratId = 0; ratId < MAXRATS; ratId++) {
		if (ratId == M.myRatId)
			continue;
		ratLook = &R2d2[ratId];
		oldVisible = ratLook->visible;
		TokenVisible(ratId);
		if (ratLook->visible == TRUE)
			XORToken(ratId);
		if (ratLook->visible != oldVisible)
			UpdateScoreCard(ratId);
	}
}

XYpair	*
plotLine(p, two)
XYpair	*p;
Boolean	two;
{
	while (1) {
		DrawViewLine(p->p1.x, p->p1.y, p->p2.x, p->p2.y);
		p++;
		if (!two)
			return p;
		two = FALSE;
	}
}

XYpair *
hidden(x, y, dir, p)
int		x, y;
Direction	dir;
XYpair		*p;
{
	int	l1x, l1y, l2x, l2y;
	int	r1x, r1y, r2x, r2y;
	int	c2x, c2y;

	/* first calculate the coordinates of the neighboring cubes */

	l1x = x + l1Delta[dir].xcor;	/* find left cube */
	l1y = y + l1Delta[dir].ycor;
	l2x = x + l2Delta[dir].xcor;	/* find left forward cube */
	l2y = y + l2Delta[dir].ycor;
	r1x = x + r1Delta[dir].xcor;	/* find right cube */
	r1y = y + r1Delta[dir].ycor;
	r2x = x + r2Delta[dir].xcor;	/* find right forward cube */
	r2y = y + r2Delta[dir].ycor;
	c2x = x + c2Delta[dir].xcor;	/* find forward cube */
	c2y = y + c2Delta[dir].ycor;

	/* next calculate which of the 7 possible cube edges are visible */

	edge2 = M.maze[c2x].y[c2y];	/* c2 */
	edge3 = M.maze[l1x].y[l1y];	/* l1 */
	edge4 = !edge3;			/* !l1 */

	edge7 = M.maze[r1x].y[r1y];	/* r1 */
	edge6 = !edge7;			/* !r1 */

	edge1 = edge3 && (edge2 || !M.maze[l2x].y[l2y])
		|| ((!edge2) && edge4);
	edge5 = edge7 && (edge2 || !M.maze[r2x].y[r2y])
		|| ((!edge2) && edge6);

	/* 
	 * Should be matching the following:
	 *	x1 = l1 (c2 + !l2) + !c2 !l1
	 *	x2 = c2
	 *	x3 = l1
	 *	x4 = !l1
	 *	x5 = r1 (c2 + !r2) + !c2 !r1
	 *	x6 = !r1
	 *	x7 = r1
	 */

	if (edge1)
		p = plotLine(p, FALSE);
	else
		p++;
	if (edge2)
		p = plotLine(p, TRUE);
	else
		p += 2;
	if (edge3) {
		if (prevEdge3) {
			edge3Lines[0].p2 = (p++)->p2;
			edge3Lines[1].p2 = p->p2;
		} else {
			edge3Lines[0] = *p++;
			edge3Lines[1] = *p;
			prevEdge3 = TRUE;
		}
		p++;
	} else {
		if (prevEdge3) {
			(void) plotLine(edge3Lines, TRUE);
			prevEdge3 = FALSE;
		}
		p += 2;
	}
	if (edge4)
		p = plotLine(p, TRUE);
	else
		p += 2;
	if (edge5)
		p = plotLine(p, FALSE);
	else
		p++;
	if (edge6)
		p = plotLine(p, TRUE);
	else
		p += 2;
	if (edge7) {
		if (prevEdge7) {
			edge7Lines[0].p1 = (p++)->p1;
			edge7Lines[1].p1 = p->p1;
		} else {
			edge7Lines[0] = *p++;
			edge7Lines[1] = *p;
			prevEdge7 = TRUE;
		}
		p++;
	} else {
		if (prevEdge7) {
			(void) plotLine(edge7Lines, TRUE);
			prevEdge7 = FALSE;
		}
		p += 2;
	}
	return p;
}

/*
 * sets R2d2[hisRatId] variables
 * uses M.ratcb[hisRatId] as input for his position and direction
 */

TokenVisible(hisRatId)
RatId	hisRatId;
{
	RatLook		ratLook = &R2d2[hisRatId];
	RatInfo		ratInfo = &M.ratcb.rats[hisRatId];
	Loc		tx, ty;
	Direction	td;
	int		ix, ix12;

	ratLook->visible = FALSE;
	if (!ratInfo->playing)
		return;
	if (M.peeking) {
		tx = M.xPeek;
		ty = M.yPeek;
		td = M.dirPeek;
	} else {
		tx = M.xloc;
		ty = M.yloc;
		td = M.dir;
	}
	ix = 0;
	while (!M.maze[tx].y[ty]) {
		switch(td) {
		case NORTH:	tx++; break;
		case SOUTH:	tx--; break;
		case EAST:	ty++; break;
		case WEST:	ty--; break;
		}
		ix++;
		if ((tx == ratInfo->xLoc) && (ty == ratInfo->yLoc)) {
			ratLook->visible = TRUE;
			ix12 = ix * 12;
			ratLook->x = (viewTable[ix12+3].p2.x +
				     viewTable[ix12+10].p1.x)/2;
			ratLook->y = (viewTable[ix12+3].p1.y +
				     viewTable[ix12+3].p2.y)/2;
			ratLook->tokenId = relativeTokens[td].t[ratInfo->dir];
			ratLook->distance = ix;
			break;
		}
	}
}

/* 
 * draw him into the maze, with the right size for his distance away
 * and facing the right way.
 */

XORToken(hisRatId)
RatId	hisRatId;
{
	int	size;
	int	srcX, srcY;

	getRat(hisRatId, &srcX, &srcY, &size);
	DisplayRatBitmap(R2d2[hisRatId].x - size/2,
			R2d2[hisRatId].y  - size/2,
			size, size, srcX, srcY);
}

/* 
 * Dig the appropriate bitmap out of the master bitmap, which has all
 * possible views neatly packed into place.
 */

getRat(hisRatId, x, y, size)
RatId	hisRatId;
int	*x, *y, *size;
{
	int	viewT = LEFT;
	int	view = R2d2[hisRatId].tokenId;

	switch (R2d2[hisRatId].distance) {
		case 1:
			*size = 64;
			*x = 0;
			*y = 0;
			for (viewT = 0; viewT != view; *x += 64)
				viewT++;
			return;

		case 2:
			*size = 32;
			for (*x = 4*64; *x != 64*5; *x += 32)
				for (*y = 0; *y != 64; *y += 32)
					if (viewT == view)
						return;
					else
						viewT++;

		case 3:
			*size = 24;
			for (*x = 5*64; *x != 64*5+48; *x += 24)
				for (*y = 0; *y != 48; *y += 24)
					if (viewT == view)
						return;
					else
						viewT++;

		case 4:
		case 5:
			*size = 16;
			*x = 64*5+48;
			for (*y = 0; *y != 64; *y += 16)
				if (viewT == view)
					return;
				else
					viewT++;

		case 6:
		case 7:
		case 8:
			*size = 9;
			*y = 48;
			for (*x = 64*5; *x != 64*5 + 4*9; *x += 9)
				if (viewT == view)
					return;
				else
					viewT++;

		case 9:
		case 10:
		case 11:
		case 12:
			*size = 6;
			*y = 48+9;
			for (*x = 64*5; *x != 64*5 + 4*6; *x += 6)
				if (viewT == view)
					return;
				else
					viewT++;

		case 13:
		case 14:
		case 15:
		case 16:
		case 17:
		case 18:
			*size = 4;
			*y = 48+9+3;
			for (*x = 64*5 + 4*6; *x != 64*5 +4*6 + 4*4; *x += 4)
				if (viewT == view)
					return;
				else
					viewT++;

		default:
			*size = 3;
			*y = 48+9;
			for (*x = 64*5 + 4*6; *x != 64*5 + 4*6 + 4*3; *x += 3)
				if (viewT == view)
					return;
				else
					viewT++;
	}
}

/* 
 * The maze display. This is controlled by the clearArray (another
 * historically significant name, I'm afraid). Each player has his
 * loaction and playing information stored here as well as in M.ratcb.
 * This is to tell who was playing before the last update/status/location
 * packet came in, so we can do the right thing for adding/deleting players.
 *
 * There are various hooks for setting the contents of clearArray
 * from the outside, like SetMyRatId().
 */

SetMyRatId(ratId)
RatId	ratId;
{
	clearArray[ratId].playing = TRUE;
}

/* 
 * Someone joined us; add him to the display.
 */

AddNewPlayer(ratId, xloc, yloc, dir)
RatId	ratId;
Direction dir;
{
	clearArray[ratId].playing = TRUE;
	clearArray[ratId].x = xloc;
	clearArray[ratId].y = yloc;
	clearArray[ratId].dir = dir;
	DisplayOthersPosition(ratId, xloc, yloc, dir);
	NotifyPlayer();
}

/*
 * Someone left.
 */
ExitPlayer(ratId)
RatId	ratId;
{
	clearPosition(ratId, clearArray[ratId].x, clearArray[ratId].y);
	clearArray[ratId].playing = FALSE;
}

/* 
 * Tell the display controller I moved.
 */

ShowPosition(xloc, yloc, invincible, tdir)
int		xloc, yloc;
Boolean		invincible;
Direction	tdir;
{
	clearPosition(M.myRatId, clearArray[M.myRatId].x,
		clearArray[M.myRatId].y);
	showMe(xloc, yloc, invincible, tdir);
}

/* 
 * Show where everybody is (only for wizards).
 */

ShowAllPositions()
{
	RatId	ratId;

	for (ratId = 0; ratId < MAXRATS; ratId++) {
		if (ratId == M.myRatId)
			continue;
		if (clearArray[ratId].playing && !M.ratcb.rats[ratId].playing)
			ExitPlayer(ratId);
		else if (!clearArray[ratId].playing &&
			 M.ratcb.rats[ratId].playing)
			AddNewPlayer(ratId, M.ratcb.rats[ratId].xLoc,
					    M.ratcb.rats[ratId].yLoc,
					    M.ratcb.rats[ratId].dir);
		else if (clearArray[ratId].playing)
			DisplayOthersPosition(ratId, M.ratcb.rats[ratId].xLoc,
						     M.ratcb.rats[ratId].yLoc,
						     M.ratcb.rats[ratId].dir);
	}
}

/* 
 * Someone else moved; update his info and display the new position.
 * Again, only for wizards who are exercising their powers.
 */

DisplayOthersPosition(ratId, xloc, yloc, dir)
RatId	ratId;
Direction dir;
{
	if (!clearArray[ratId].playing && M.ratcb.rats[ratId].playing) {
		AddNewPlayer(ratId, xloc, yloc, dir);	/* calls me back */
		return;
	}
		
	clearPosition(ratId, clearArray[ratId].x, clearArray[ratId].y);
	if (clearArray[ratId].playing && M.omniscient)
		showOther(ratId, xloc, yloc, dir);
}

/* 
 * Do the actual work of showing me, with the right sort of arrow (in
 * case I'm playing wizard).
 */

showMe(xloc, yloc, invincible, dir)
int		xloc, yloc;
Boolean		invincible;
Direction	dir;
{
	register BitCell	*bp;
	register RatState	*rsp;

	if (invincible)
		bp = invincibleArrows;
	else
		bp = normalArrows;

	HackMazeBitmap(xloc, yloc, &bp[dir]);

	rsp = &clearArray[M.myRatId];
	rsp->playing = TRUE;
	rsp->x = xloc;
	rsp->y = yloc;
	rsp->dir = dir;
	M.invincible = invincible;
}

/* 
 * Bang the bits to show the other guy.
 */

showOther(ratId, xloc, yloc, dir)
Direction dir;
{
	register int		i;
	register RatState	*rsp;
	BitCell			bits;

	for (i = 0; i < 16; i++)
		bits.bits[i] = otherArrows[dir].bits[i] |
		               numbers[ratId].bits[i]; 

	HackMazeBitmap(xloc, yloc, &bits);

	rsp = &clearArray[ratId];
	rsp->playing = TRUE;
	rsp->x = xloc;
	rsp->y = yloc;
	rsp->dir = dir;
}

/* 
 * When someone moves off a position, clear it out. If there's someone
 * "under" him, display the second guy. 
 */

clearPosition(ratId, xClear, yClear)
RatId	ratId;
{
	register RatState	*rsp;
	register int		i;
	
	clearSquare(xClear, yClear);
	for (i = 0; i < MAXRATS; i++) {
		rsp = &clearArray[i];
		if ((i == ratId) || !(rsp->playing))
			continue;
		if ((rsp->x == xClear) && (rsp->y == yClear))
			if (i == M.myRatId)
				showMe(rsp->x, rsp->y, M.invincible, rsp->dir);
			else
				if (M.omniscient)
					showOther(i, rsp->x, rsp->y, rsp->dir);
	}
}

static BitCell EMPTY = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* clear out a square of the maze */
clearSquare(xClear, yClear)
{
	HackMazeBitmap(xClear, yClear, &EMPTY);
}

/* 
 * Handle the score card area. Most of the work is done in window system
 * dependent routines.
 */

NewScoreCard()
{
	register RatId	ratId;

	for( ratId = 0; ratId < MAXRATS; ratId++)
		UpdateScoreCard(ratId);
	if (M.omniscient)
		ShowAllPositions();
}

UpdateScoreCard(ratId)
RatId	ratId;
{
	ClearScoreLine(ratId);
	if (M.ratcb.rats[ratId].playing)
		WriteScoreString(ratId);
	if (R2d2[ratId].visible == TRUE)
		InvertScoreLine(ratId);
}

/*
 * Some window systems (notably X10) choose to represent bitmaps in
 * little-endian order. That is, the least significant bit in the word is
 * the leftmost bit on the screen. This means that bitmaps look
 * "backwards" on the screen from the way they look in source code. So, we
 * flip them all here, once, at initialization time, so they can be edited
 * "by hand".
 *
 * It is up to the window system initialization code to call this, if
 * deemed necessary.
 *
 * Hack hack.
 */

FlipBitmaps()
{
	register i;

	bitFlip(normalArrows);
	bitFlip(invincibleArrows);
	bitFlip(otherArrows);
	bitFlip(numbers);		/* only gets the first 4 */
	bitFlip(&numbers[4]);		/* get the rest */
	for (i = 0; i < ratBits_width * ratBits_height / 16; i += 64)
		bitFlip((BitCell *) &ratBits[i]);
}

/*
 * quick bit flipper -- divide and conquer due to Jeff Mogul.
 * unrolled to do 4 16 bit words, but generally extensible.
 */

bitFlip(bits)
register BitCell *bits;
{
	register int	num, top, bot, i, j;

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 16; j++) {
			num = bits->bits[j] & 0xffff;
			top = num & 0xff00;
			bot = num & 0x00ff;
			top >>= 8;
			bot <<= 8;
			num = top|bot;

			top = num & 0xf0f0;
			bot = num & 0x0f0f;
			top >>= 4;
			bot <<= 4;
			num = top|bot;

			top = num & 0xcccc;
			bot = num & 0x3333;
			top >>= 2;
			bot <<= 2;
			num = top|bot;

			top = num & 0xaaaa;
			bot = num & 0x5555;
			top >>= 1;
			bot <<= 1;
			bits->bits[j] = top|bot;
		}
		bits++;
	}
}


/*
 * Then there are the systems that are perfectly happy with MSBFirst data, but
 * byte swap the shorts that are used.  We fix that here.
 *
 * The htons() routine is a defined to be a no-op on machines that don't have
 * this problem.
 *
 */

SwapBitmaps()
{
	register i;

	byteSwap(normalArrows);
	byteSwap(invincibleArrows);
	byteSwap(otherArrows);
	byteSwap(numbers);	/* only gets first 4 */
	byteSwap(&numbers[4]);	/* get the rest */
	for (i = 0; i < ratBits_width * ratBits_height / 16; i += 64)
		byteSwap((BitCell *) &ratBits[i]);
}

/*
 * Byte swap 4 16x16 bitmaps.
 */

byteSwap(bits)
register BitCell *bits;
{
	register int i, j;
	
	for (i = 0; i < 4; i++, bits++)
		for (j = 0; j < 16; j++)
			bits->bits[j] = htons(bits->bits[j] & 0xffff);
}
