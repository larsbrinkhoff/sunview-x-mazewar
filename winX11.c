/* $Header: winX11.c,v 1.30 89/02/24 16:29:44 kent Exp $ */

/* 
 * window_X11.c - Window-system specific routines for X11
 * 
 * Author:	Mike Yang
 * 		Western Research Laboratory
 *	 	Digital Equipment Corporation
 * Date:	Mon Jul 25 1988
 */

/***********************************************************
Copyright 1988 by Digital Equipment Corporation, Maynard, Massachusetts,

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
 * It is intended that this file presents a window system-independent
 * set of routines to MazeWar; that is, if you want to run MW under
 * another window system, this is the only file that needs to be
 * changed. It isn't yet clear how well I've succeeded, but it's a start.
 * 
 * Routines that are presented to the outside are capitalized; local
 * routines are not.
 */

/*#define	USE_BITMAPS	/* turn on if you know CopyPlanes works */

/*
 * $Log:	winX11.c,v $
 * Revision 1.30  89/02/24  16:29:44  kent
 * Changed inversion code to use GXxor instead of GXcopyInverted. Makes
 * the code a little cleaner, and Joel says it'll run slightly 
 * faster (big deal).
 * 
 * Revision 1.29  89/02/24  16:18:51  kent
 * Fixed a number of problems with the icon flashing code.
 * 
 * Revision 1.28  89/02/23  10:10:25  kent
 * Changed to X11 format bitmaps.
 * 
 * Revision 1.27  89/02/22  17:28:08  kent
 * Make the cursor bitmap files internal, instead of trying to read
 * them at runtime (!). Requires some byteswapping on MSBFirst machines.
 * 
 * Revision 1.26  89/01/18  17:26:46  kent
 * Set class hints for the window so the window manager does
 * the right thing.
 * 
 * Revision 1.25  88/09/07  10:49:07  kent
 * Many changes in the name of portability.
 * 
 * Revision 1.24  88/08/12  11:01:24  kent
 * Cleaned up image code.
 * 
 * Revision 1.22  88/08/11  08:48:44  kent
 * Put Bitmap/CopyPlane code in for those that can use it.
 * 
 * Revision 1.21  88/08/10  16:41:08  kent
 * Changed rats to be an XImage that I put. Fixed xor so it works properly
 * on ANY sex display.
 * 
 * Revision 1.20  88/08/10  11:16:55  kent
 * Eliminated use of keycodes in input event mapping. 
 * 
 * Revision 1.19  88/08/09  20:19:33  kent
 * Changed the mazePixmap to use an XImage and always put the bits
 * remotely. Amazingly enough, the performance is a lot better, and
 * the startup delay is a lot less.
 * 
 * Revision 1.18  88/08/09  19:42:32  kent
 * Sped up arrow display code (HackMazeBitmap). Previously was creating
 * a Pixmap (and not destroying it!) every time -- costing a round trip
 * for each call. Changed it to Create/Put/Destroy an XImage, which
 * costs a little locally, but no round trips. Big difference!
 * 
 * Also some general code cleanup.
 * 
 * Revision 1.17  88/07/25  18:18:27  kent
 * Switch to zero-width lines. Much faster.
 * 
 * Revision 1.16  88/07/25  09:12:40  mikey
 * cleanup source
 * 
 * Revision 1.15  88/07/25  09:01:54  mikey
 * Coalesced line drawing requests, fixed depth bugs
 * 
 * Revision 1.14  88/07/20  11:53:26  mikey
 * moved code from StartWindow to InitWindow so as to eliminate timeout
 * problems with ratDoctor.
 * 
 * Revision 1.13  88/06/21  09:24:51  mikey
 * Fixed some bugs in the previous version.  FocusIn doesn't
 * do anything now if an Expose event is in the queue at all.
 * 
 * Revision 1.12  88/06/20  16:52:35  mikey
 * Use XCheckIfEvent to reduce duplication if focusin and expose events
 * follow each other, eliminated use of GXcopyInverted and instead
 * set foreground and background, tried to eliminate buttonpress
 * effect when selecting window.
 * 
 * Revision 1.11  88/06/20  10:01:06  mikey
 * Inverts display when input focus is not in the window.  A lot of
 * updating... maybe it can be reduced.
 * 
 * Revision 1.10  88/06/19  15:16:18  mikey
 * Handles 32x32 bitmap for X11 and eliminated kludges for wm.
 * Currently flashes icon depending on input focus.  Should
 * change to depending on open/closed.
 * 
 * Revision 1.9  88/06/17  14:53:12  mikey
 * Flashing icon on focus events
 * 
 * Revision 1.8  88/06/17  12:48:52  mikey
 * Icon fixed for color (gpx) displays.
 * 
 * Revision 1.7  88/06/17  11:35:41  mikey
 * fixed inverted bitmaps on vs100, eliminated blob, created icon
 * for monochrome wm.  icon still messed for gpx.
 * 
 * Revision 1.6  88/06/17  10:47:31  mikey
 * undid invertDisplay because of problems with screen saver and with
 * window exposure/update.  other problems: blob in upperleft of\
 * maze on vs100, inverted bitmaps on vs100, unsuccessful creation
 * of 32x32 icon [oxmap.
 * 
 * Revision 1.5  88/06/16  16:09:30  mikey
 * replaced most uses of XCreateBitmap with XCreatePixmapFromBitmapData
 * to solve vs100 problems.  still have negated-black/white problems
 * with bitmaps and vs100.
 * 
 * Revision 1.4  88/06/16  15:29:55  mikey
 * disabled icon flashing, fixed inverting problems, still have to
 * fix vs100 bitmap display
 * 
 * Revision 1.3  88/06/15  11:40:05  mikey
 * before trying to fix icon window
 * 
 */

#ifndef	lint
static char rcs_ident[] = "$Header: winX11.c,v 1.30 89/02/24 16:29:44 kent Exp $";
#endif

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/StringDefs.h>

#include <sys/types.h>
#include <sys/time.h>

#include <netinet/in.h>

#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <strings.h>

#include "mazewar.h"
#undef	Boolean

#include <X11/Intrinsic.h>

#define	VIEW_X_DIM	400
#define	VIEW_Y_DIM	400
#define	VIEW_X_ORIGIN	100
#define	VIEW_Y_ORIGIN	50
#define	MAZE_X_DIM	(MAZEXMAX*16)
#define	MAZE_Y_DIM	(MAZEYMAX*16)
#define	MAZE_X_ORIGIN	48
#define	MAZE_Y_ORIGIN	451
#define	SCORE_X_DIM	192
#define	SCORE_Y_DIM	((scoreFontInfo->max_bounds.ascent + scoreFontInfo->max_bounds.descent) * MAXRATS)
#define	SCORE_X_ORIGIN	208
#define	SCORE_Y_ORIGIN	708
#define	MIN_X_DIM	608		/* see comments for InitWindow() */
#define	MIN_Y_DIM	SCORE_Y_ORIGIN	/* see InitWindow() */

#define DEFAULT_FONT "8x13"
#define DEFAULT_POSITION "%dx%d+0+0"	/* upper left hand corner */

#define ICON_FLASH_PERIOD 4

short	mazeBits[MAZEXMAX*MAZEYMAX*16];	/* shorts better be 16 bits! */
Window	mwWindow;			/* parent window */
#ifdef	USE_BITMAPS
Pixmap	mazeBitmap;			/* the maze */
Pixmap	ratsBitmap;			/* the rats */
#else
XImage	*ratsImage;			/* the rats */
XImage	*mazeImage;			/* the maze */
#endif	USE_BITMAPS
Pixmap	iconMask;			/* mask for icon outline */
XFontStruct	*scoreFontInfo;
Boolean	mapped =	FALSE;		/* should really display? */
Boolean	flashIcon =	FALSE;		/* should icon be flashing? */
Boolean	iconInverted =	FALSE;		/* icon should be/is inverted */
int	displayFD;			/* fd of display for events */

Pixel		fg_pixel;		/* color of lines and such */
Pixel		bg_pixel;		/* color of background */
Pixel		mouseground;		/* mouse cursor color */
Font		scoreFont;		/* font for printing scores */
Pixel		borderColor;
Cardinal	borderWidth;

Display	*dpy;				/* display used */
int	screen;				/* screen on the display */
unsigned int	cur_width, cur_height;	/* current width, height of window */
GC	copyGC, xorGC;			/* graphics contexts for window */
Pixmap	icon_pixmap, iconmask_pixmap;	/* icon, mask bitmaps */
Pixmap	icon_reverse_pixmap;
int	icon_flash = 0;
XImage	*arrowImage;

#include "bitmaps/icon.ic"
#include "bitmaps/rat.cur"
#include "bitmaps/ratMask.cur"
#include "bitmaps/dRat.cur"
#include "bitmaps/dRatMask.cur"
#include "bitmaps/cup.cur"

static Cursor	ratCursor, deadRatCursor, hourGlassCursor;

extern char	*getenv();
extern int	errno;

static char	*progName;

Pixmap		xCreateBitmapFromBitCell();

static XrmOptionDescRec opTable[] = {
{"-mouse",	"*mouseColor",	XrmoptionSepArg,	(caddr_t) NULL},
};

#define	XtNmouse	"mouse"

int	zero = 0;
int	one = 1;

static XtResource resources[] = {
	{XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
	 (Cardinal)&fg_pixel, XtRString, "black"},
	{XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel),
	 (Cardinal)&bg_pixel, XtRString, "white"},
	{XtNmouse, XtCCursor, XtRPixel, sizeof(Pixel),
	 (Cardinal)&mouseground, XtRString, "black"},
	{XtNfont, XtCFont, XtRFont, sizeof(Font),
	 (Cardinal)&scoreFont, XtRString, DEFAULT_FONT}, 
	{XtNborderWidth, XtCBorderWidth, XtRInt, sizeof(int),
	 (Cardinal)&borderWidth, XtRInt, (caddr_t) &one},
	{XtNborder, XtCBorderColor, XtRPixel, sizeof(Pixel),
	 (Cardinal)&borderColor, XtRString, "black"},
};

/* 
 * ALL COORDINATES ASSUME ORIGIN (0,0) AT UPPER LEFT!
 */

/* 
 * Initialize the window. The overall window must be at least 608x by
 * 808y (the size of the Alto screen). There are three distinct
 * subregions:
 *	A 400x by 400y area for the view, beginning at (100,50)
 *	A 512x by 256y area for the maze, beginning at (48, 451)
 *	A 192x by 96y area for the score lines, at (208, 706)
 *
 *	Actually, the y dimension of both the whole window and the score
 *	subwindow must be enough to accommodate MAXRATS lines of the
 *	height of the scoreFont.
 */

/* parse arguments and set up window state */

InitWindow(argc, argv)
char	**argv;
{
	XGCValues gc_values;
	XWMHints wmhints;
	Widget	w;
	XClassHint	classHint;
	GC	iconGC;

	progName = rindex(argv[0], '/');
	if (progName)
		progName++;
	else
		progName = argv[0];

	/*
	 * We cheat here by using the Toolkit to do the initialization work.
	 * We just ignore the top-level widget that gets created.
	 */

	w = XtInitialize("mazewar", "mazewar", opTable, XtNumber(opTable), 
			&argc, argv);

	dpy = XtDisplay(w);
	screen = DefaultScreen(dpy);

	XtGetApplicationResources(w, (caddr_t) NULL, resources, 
				XtNumber(resources), NULL, (Cardinal) 0);
	
	if (!scoreFont)
		MWError("cannot open font");
	scoreFontInfo = XQueryFont(dpy, scoreFont);

        cur_width = MIN_X_DIM;
        cur_height = MIN_Y_DIM + (MAXRATS+1) *
	             (scoreFontInfo->max_bounds.ascent +
		      scoreFontInfo->max_bounds.descent);

	mwWindow = XCreateSimpleWindow(dpy, 
					RootWindow(dpy, screen),
					0, 0, 
					cur_width, cur_height,
					borderWidth, 0, bg_pixel);
	XStoreName(dpy, mwWindow, "MazeWar");
	XSetIconName(dpy, mwWindow, "MazeWar");
	classHint.res_name = "mazewar";
	classHint.res_class = "mazewar";
	XSetClassHint(dpy, mwWindow, &classHint);

	gc_values.function = GXcopy;
	gc_values.foreground = fg_pixel;
	gc_values.background = bg_pixel;
	gc_values.font = scoreFont;
	gc_values.line_width = 0;
	copyGC = XCreateGC(dpy, mwWindow,
		       GCFunction | GCForeground | GCBackground
		       | GCLineWidth | GCFont,
		       &gc_values);

	gc_values.function = GXxor;
	gc_values.plane_mask = AllPlanes;
	gc_values.foreground = fg_pixel ^ bg_pixel;
	gc_values.background = 0;
	xorGC = XCreateGC(dpy, mwWindow,
		       GCFunction | GCForeground | GCBackground | GCPlaneMask,
		       &gc_values);

	icon_pixmap = XCreatePixmapFromBitmapData(
			dpy, mwWindow, 
			icon_bits, 
			icon_width, icon_height, 
			fg_pixel, bg_pixel, 
			XDefaultDepth(dpy, screen));

	/* is this even used? */
	gc_values.function = GXclear;
	gc_values.plane_mask = AllPlanes;
	iconGC = XCreateGC(dpy, mwWindow, 
			GCFunction | GCPlaneMask, 
			&gc_values);
	iconmask_pixmap = XCreatePixmap(dpy, mwWindow,
					icon_width, icon_height, 
					XDefaultDepth(dpy, screen));
	XFillRectangle(dpy, iconmask_pixmap, iconGC, 0, 0,
			icon_width, icon_height);

	icon_reverse_pixmap = XCreatePixmapFromBitmapData(dpy, mwWindow,
						  icon_bits,
						  icon_width, icon_height,
						  bg_pixel, fg_pixel, 
						  XDefaultDepth(dpy, screen));

        wmhints.flags = IconPixmapHint | IconMaskHint;
        wmhints.icon_pixmap = icon_pixmap;
        wmhints.icon_mask = iconmask_pixmap;
        XSetWMHints(dpy, mwWindow, &wmhints);

	initCursors();
	arrowImage = XCreateImage(dpy, DefaultVisual(dpy, DefaultScreen(dpy)),
				1, XYBitmap, 0, NULL, 
				16, 16, 8, 2);
	arrowImage->byte_order = MSBFirst;
	arrowImage->bitmap_bit_order = MSBFirst;
}

/* 
 * actually start the display up, after all the user interaction has
 * been done.
 */

StartWindow(ratBits, ratWidth, ratHeight)
short	*ratBits;
{
	XSelectInput(dpy, mwWindow, KeyPressMask | ButtonPressMask |
		                    ButtonReleaseMask |  ExposureMask |
				    StructureNotifyMask | FocusChangeMask);
	HourGlassCursor();

	initMaze();
	SwapBitmaps();

	displayFD = XConnectionNumber(dpy);

	XMapWindow(dpy, mwWindow);		/* Map window to screen */
	initRats(ratBits, ratWidth, ratHeight);
}

/*
 * Shut down the window system, resetting any terminal modes or what
 * have you that may have been altered. No-op for X11.
 */

StopWindow()
{
	
}

/* 
 * Clear out the view subwindow. If possible, delay the visible effect
 * until all the lines have been drawn (since this is only called by
 * ShowView(), which is about to draw a bunch of lines).
 */

ClearView()
{
	XClearArea(dpy, mwWindow, VIEW_X_ORIGIN, VIEW_Y_ORIGIN,
		       VIEW_X_DIM, VIEW_Y_DIM, False);
}

/*
 * Flash the view window to indicate a hit on an opponent.
 */

FlashTop()
{
	XFillRectangle(dpy, mwWindow, xorGC, 
		VIEW_X_ORIGIN, VIEW_Y_ORIGIN, 
		VIEW_X_DIM, VIEW_Y_DIM);
	XFlush(dpy);

	/* might want some delay here */

	XFillRectangle(dpy, mwWindow, xorGC, 
		VIEW_X_ORIGIN, VIEW_Y_ORIGIN, 
		VIEW_X_DIM, VIEW_Y_DIM);
	XFlush(dpy);
}

/*
 * Flash the whole window to indicate we were hit.
 */

FlashScreen()
{
	XFillRectangle(dpy, mwWindow, xorGC, 0, 0, cur_width, cur_height);
	XFlush(dpy);

	/* might want some delay here */

	XFillRectangle(dpy, mwWindow, xorGC, 0, 0, cur_width, cur_height);
	XFlush(dpy);
}

/* 
 * Draw a line in the view subwindow. Don't show it right away if possible.
 */

DrawViewLine(x1, y1, x2, y2)
{
	XDrawLine(dpy, mwWindow, copyGC, x1+VIEW_X_ORIGIN, y1+VIEW_Y_ORIGIN,
		  x2+VIEW_X_ORIGIN, y2+VIEW_Y_ORIGIN);
}

/* 
 * Event handling. Presents a uniform event interface to the game, and
 * handles all other events in here. The game only sees the events
 * that affect it directly, as defined in mazewar.h. They are:
 *
 *	keypresses that affect the game
 *	mouse button clicks
 *	incoming network packets
 *	timeouts (there's a "heartbeat" to drive the shot clock)
 *
 * All other events should be swallowed by this routine. Also, since
 * keyboard focus is dependent on the mouse being in the game window,
 * try to let the user know that he's wandered outside the window
 * by inverting the window (should make it obvious) and is now vulnerable
 * and defenseless.
 */

static
repaintWindow();
static
repaintIcon();

void
NextEvent(event)
MWEvent	*event;
{
	int	fdmask;
	XEvent	xev;
	struct timeval timeout;
	int	ret;
	char	c;

    while (1) {
	icon_flash = (++icon_flash) % ICON_FLASH_PERIOD;

	if (!XPending(dpy))	/* this does an XFlush, too */
		if (flashIcon && !icon_flash && !mapped) {
			/* invert the icon  */
			iconInverted = !iconInverted;
			repaintIcon();
		}

	/*
	 * Look for events.  Try to arrange that X events have priority over
	 * network traffic.  See if there's an X event pending.  If so, check
	 * for a net event, too; if not, select on both the network and the X
	 * connection.  If that doesn't time out, but there's no X event
	 * pending, try again, just selecting on the X connection.  If that
	 * times out, let the network event get processed.
	 *
	 * Can't just select on the two fds, because there may be X events
	 * pending in the queue that have already been read.
	 *
	 * This may look baroque, but we've seen some instances where X server
	 * latency seems to let the network events take priority over sever
	 * events, leading to sluggish keyboard response and lots of local
	 * death.
	 *
	 * Also, can't just ignore network events -- if the server is slow and
	 * lots of X events get queued, someone else's ratDoctor will time you
	 * out if you never answer.
	 */

	if (!XPending(dpy)) {
		fdmask = (1 << displayFD) | (1 << M.theSocket);
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;
		while ((ret = select(32, &fdmask, NULL, NULL, &timeout)) == -1)
			if (errno != EINTR)
				MWError("select error on events");
		if (ret == 0) {
			event->eventType = EVENT_TIMEOUT;
			return;
		}
	} else {
		fdmask = 1 << M.theSocket;
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		while ((ret = select(32, &fdmask, NULL, NULL, &timeout)) == -1)
			if (errno != EINTR)
				MWError("select error on events");
	}
	if (XPending(dpy)) {
		XNextEvent(dpy, &xev);
		switch (xev.type) {
		case KeyPress:
			event->eventType = 0;
			XLookupString((XKeyEvent *) &xev, &c, 1, 
					NULL, NULL);
			
			switch(c) {
			case 'a':
			case '4':	/* keypad */
				event->eventType = EVENT_A;
				return;

			case 's':
			case '5':
				event->eventType = EVENT_S;
				return;

			case 'd':
			case '6':
				event->eventType = EVENT_D;
				return;

			case 'f':
			case ',':
				event->eventType = EVENT_F;
				return;

			case ' ':
			case '\033':	/* ESC lead in of arrow */
				event->eventType = EVENT_BAR;
				return;

			case 'i':
				event->eventType = EVENT_I;
				return;

			case 'k':
				event->eventType = EVENT_K;
				return;

			case 'o':
				event->eventType = EVENT_O;
				return;

			case 'l':
				event->eventType = EVENT_L;
				return;

			case 'q':
			case '\177':	/* DEL */
			case '\003':	/* ^C */
				event->eventType = EVENT_INT;
				return;
			}
			break;

#define	RightButton	Button3
#define	MiddleButton	Button2
#define	LeftButton	Button1
		case ButtonPress:
			event->eventType = 0;
			switch(((XButtonPressedEvent *) 
				&xev)->button & 0xff) {
			case RightButton:
				event->eventType = EVENT_RIGHT_D;
				return;

			case MiddleButton:
				event->eventType = EVENT_MIDDLE_D;
				return;

			case LeftButton:
				event->eventType = EVENT_LEFT_D;
				return;
			}
			break;

		case ButtonRelease:
			event->eventType = 0;
			switch(((XButtonReleasedEvent *) 
				&xev)->button&0xff) {
			case RightButton:
				event->eventType = EVENT_RIGHT_U;
				return;

			case LeftButton:
				event->eventType = EVENT_LEFT_U;
				return;
			}
			break;

		case Expose:
			repaintWindow();
			break;

		case FocusIn:
		case MapNotify:
			mapped = TRUE;
			iconInverted = FALSE;
			flashIcon = FALSE;
			repaintIcon();
			break;

		case FocusOut:
		case UnmapNotify:
			mapped = FALSE;
			break;
		}
	}

	if (fdmask & (1 << M.theSocket)) {
		int fromLen = sizeof(event->eventSource);
		int cc;

		event->eventType = EVENT_NETWORK;
		cc = recvfrom(M.theSocket, event->eventDetail,
			sizeof(RatPacket), 0, &event->eventSource, &fromLen);
		if (cc <= 0) {
			if (cc < 0 && errno != EINTR)
				perror("event recvfrom");
			continue;
		}
		if (fromLen != sizeof(struct sockaddr_in))
			continue;
		ConvertIncoming(event->eventDetail);
		return;
	}
    }
}


/* 
 * Peek to see if there's a keyboard event waiting, in case the
 * program wants to short-circuit some code. If your system won't let
 * you implement this feature easily, just always return FALSE.
 */

Boolean
KBEventPending()
{
        return (XPending(dpy) != 0);
}

/* Please stand by ... */

HourGlassCursor()
{
	XDefineCursor(dpy, mwWindow, hourGlassCursor);
	XFlush(dpy);
}

/* Let the games begin! */
RatCursor()
{
	XDefineCursor(dpy, mwWindow, ratCursor);
	XFlush(dpy);
}

/* He's dead, Jim ... or might be */
DeadRatCursor()
{
	XDefineCursor(dpy, mwWindow, deadRatCursor);
	XFlush(dpy);
}

/* 
 * Update the displayed bitmap. Would really like to store the arrow bitmaps
 * remotely, but the non-normal ones (especially otherArrows) may change. 
 */

HackMazeBitmap(x, y, newBits)
BitCell *newBits;
{
	arrowImage->data = (char *)newBits;
	XPutImage(dpy, mwWindow, copyGC, arrowImage, 0, 0, 
			  x*16 + MAZE_X_ORIGIN, y*16 + MAZE_Y_ORIGIN, 
			  16, 16);
}

/*
 * Display a rat.  Width, height, srcX and srcY describe the subbitmap in the
 * rats bitmap that is the desired view of the opponent.
 */

DisplayRatBitmap(screenX, screenY, width, height, srcX, srcY)
{
#ifdef	USE_BITMAPS
	XCopyPlane(dpy, ratsBitmap, mwWindow, xorGC, srcX, srcY,
		  width, height,
		  screenX+VIEW_X_ORIGIN, screenY+VIEW_Y_ORIGIN, 1);
#else
	XPutImage(dpy, mwWindow, xorGC, ratsImage, srcX, srcY, 
		  screenX+VIEW_X_ORIGIN, screenY+VIEW_Y_ORIGIN,
		  width, height);
#endif	USE_BITMAPS
}

/* 
 * Display the score line for the indicated player. Name is left
 * justified, score is right justified within the score window.
 */

WriteScoreString(rat)
RatId	rat;
{
	char	buf[64];
	int	leftEdge;

	sprintf(buf, "%d", (unsigned int) M.ratcb.rats[rat].score);

	XClearArea(dpy, mwWindow, SCORE_X_ORIGIN,
		   SCORE_Y_ORIGIN +
		   rat * (scoreFontInfo->max_bounds.ascent +
			  scoreFontInfo->max_bounds.descent),
		   SCORE_X_DIM,
		   (scoreFontInfo->max_bounds.ascent +
		    scoreFontInfo->max_bounds.descent),
		   FALSE);
	XDrawImageString(dpy, mwWindow, copyGC, SCORE_X_ORIGIN,
			 SCORE_Y_ORIGIN +
			 rat * (scoreFontInfo->max_bounds.ascent +
				scoreFontInfo->max_bounds.descent) +
			 scoreFontInfo->max_bounds.ascent,
			 M.ratcb.rats[rat].name, strlen(M.ratcb.rats[rat].name));
	leftEdge = SCORE_X_DIM - XTextWidth(scoreFontInfo, buf, strlen(buf));
	XDrawImageString(dpy, mwWindow, copyGC, leftEdge+SCORE_X_ORIGIN,
			 SCORE_Y_ORIGIN +
			 rat * (scoreFontInfo->max_bounds.ascent +
				scoreFontInfo->max_bounds.descent) +
			 scoreFontInfo->max_bounds.ascent,
			 buf, strlen(buf));
}

/* 
 * Clear out the score line for a player that's left the game.
 */

ClearScoreLine(rat)
RatId	rat;
{
	XClearArea(dpy, mwWindow, SCORE_X_ORIGIN,
		       SCORE_Y_ORIGIN +
		       rat * (scoreFontInfo->max_bounds.ascent +
			      scoreFontInfo->max_bounds.descent),
		       SCORE_X_DIM,
		       scoreFontInfo->max_bounds.ascent +
		         scoreFontInfo->max_bounds.descent, False);
}

/* 
 * Pretty obvious, eh? Means the guy's in sight.
 */

InvertScoreLine(rat)
RatId	rat;
{
	XFillRectangle(dpy, mwWindow, xorGC, 
		  SCORE_X_ORIGIN,
		  SCORE_Y_ORIGIN +
		  rat * (scoreFontInfo->max_bounds.ascent +
			 scoreFontInfo->max_bounds.descent),
		  SCORE_X_DIM,
		  scoreFontInfo->max_bounds.ascent +
		  scoreFontInfo->max_bounds.descent);
}

/* 
 * Let the user know that a new player joined. This really should only
 * take effect if the window is iconic, so you can keep a game around
 * all day, close it up when there's no activity, and know when
 * someone else is also out to kill some time.
 */

NotifyPlayer()
{
	flashIcon = TRUE;
}

/*
 * END PUBLIC ROUTINES
 */

/* set up needed bitmaps in the server */

initCursors()
{
        Pixmap	p, m;
	XColor	mc, bc;

	mc.pixel = mouseground;
	bc.pixel = bg_pixel;
	mc.flags = DoRed | DoGreen | DoBlue;
	bc.flags = DoRed | DoGreen | DoBlue;
	XQueryColor(dpy, DefaultColormap(dpy, screen), &mc);
	XQueryColor(dpy, DefaultColormap(dpy, screen), &bc);

	m = XCreateBitmapFromData(dpy, mwWindow, 
		(char *)ratMask_bits, 
		ratMask_width, ratMask_height);
	p = XCreateBitmapFromData(dpy, mwWindow, 
		(char *)rat_bits, 
		rat_width, rat_height);
	ratCursor = XCreatePixmapCursor(dpy, p, m, &mc, &bc, 
		rat_x_hot, rat_y_hot);

	m = XCreateBitmapFromData(dpy, mwWindow, 
		(char *)dRatMask_bits, 
		dRatMask_width, dRatMask_height);
	p = XCreateBitmapFromData(dpy, mwWindow, 
		(char *)dRat_bits, 
		dRat_width, dRat_height);
	deadRatCursor = XCreatePixmapCursor(dpy, p, m, &mc, &bc, 0, 0);

	p = XCreateBitmapFromData(dpy, mwWindow, 
		(char *)cup_bits,
		cup_width, cup_height);
	hourGlassCursor = XCreatePixmapCursor(dpy, p, p, &mc, &bc, 0, 0);
}

/*
 * construct an XImage of the maze.
 */

initMaze()
{
	register int	i, j, k, line, index;

	for (i = 0; i < MAZEYMAX; i++) {
		line = i * MAZEXMAX * MAZEYMAX;
		for (j = 0; j < MAZEXMAX; j++) {
			index = line + j;
			for (k = 0; k < 16; k++) {
				if (M.maze[j].y[i])
					mazeBits[index] = 0177777;
				else
					mazeBits[index] = 0;
				index += 32;
			}
					
		}
	}

#ifdef	USE_BITMAPS
	mazeBitmap = xCreateBitmapFromBitCell(dpy, mwWindow, (char *) mazeBits,
					MAZE_X_DIM, MAZE_Y_DIM);
	if (mazeBitmap == 0)
#else
	mazeImage = XCreateImage(dpy, DefaultVisual(dpy, DefaultScreen(dpy)),
				1, XYBitmap, 0, mazeBits, 
				MAZE_X_DIM, MAZE_Y_DIM, 8, MAZE_X_DIM>>3);
	mazeImage->byte_order = MSBFirst;
	mazeImage->bitmap_bit_order = MSBFirst;

	if (mazeImage == 0)
#endif	USE_BITMAPS
		MWError("Can't create maze Pixmap");
}

/*
 * actually put the maze Pixmap on the screen. 
 */

drawMaze()
{
#ifdef	USE_BITMAPS
	XCopyPlane(dpy, mazeBitmap, mwWindow, copyGC, 0, 0, 
			MAZE_X_DIM, MAZE_Y_DIM, 
			MAZE_X_ORIGIN, MAZE_Y_ORIGIN, 1);
#else
	XPutImage(dpy, mwWindow, copyGC, mazeImage, 0, 0, 
			MAZE_X_ORIGIN, MAZE_Y_ORIGIN,
			MAZE_X_DIM, MAZE_Y_DIM);
#endif	USE_BITMAPS
}

/*
 * Create the bitmap of the rats for later use.
 */

initRats(bits, width, height)
short *bits;
{
#ifdef	USE_BITMAPS
	ratsBitmap = xCreateBitmapFromBitCell(dpy, mwWindow, (char *) bits, 
					width, height);
	if (ratsBitmap == 0)
#else
	ratsImage = XCreateImage(dpy, DefaultVisual(dpy, DefaultScreen(dpy)),
				1, XYBitmap, 0, (char *) bits,
				width, height, 8, width>>3);
	ratsImage->byte_order = MSBFirst;
	ratsImage->bitmap_bit_order = MSBFirst;

	if (ratsImage == 0)
#endif	USE_BITMAPS
		MWError("Can't create rats");
}

/*
 * Repaint the window for exposure and resize events. All drawing is
 * done here, even though there are times during the initialization
 * code when it would seem obvious to put some of the display up; this
 * way portions of the display aren't shown twice then.
 */

static
repaintWindow()
{
	drawMaze();
	ShowPosition(M.xloc, M.yloc, M.invincible, M.dir);
	ShowView(M.xloc, M.yloc, M.dir);
	ShowAllPositions();

	NewScoreCard();
	XFlush(dpy);
}

/* 
 * Repaint the icon for exposure events, or while flashing to indicate
 * that there's a new player. 
 */

static
repaintIcon()
{
	XWMHints wmhints;

	if (!iconInverted)
		wmhints.icon_pixmap = icon_pixmap;
	else
		wmhints.icon_pixmap = icon_reverse_pixmap;
	wmhints.flags = IconPixmapHint | IconMaskHint;
	wmhints.icon_mask = iconmask_pixmap;
	XSetWMHints(dpy, mwWindow, &wmhints);
}

/* 
 * Invert the whole display. Used to let the user know the mouse has
 * strayed outside the window. We don't do this for X11.
 */

static
invertDisplay()
{

}

/*
 * A hack of XCreateBitmapFromData that assumes MSBFirst BitCells insterad of
 * LSBFirst bytes.
 */

Pixmap
xCreateBitmapFromBitCell(display, d, data, width, height)
Display *display;
Drawable d;
char *data;
unsigned int width, height;
{
	XImage ximage;
	GC gc;
	Pixmap pix;

	pix = XCreatePixmap(display, d, width, height, 1);
	if (!pix)
	  return(0);
	gc = XCreateGC(display, pix, (unsigned long)0, (XGCValues *)0);
	ximage.height = height;
	ximage.width = width;
	ximage.depth = 1;
	ximage.xoffset = 0;
	ximage.format = ZPixmap;
	ximage.data = data;
	ximage.byte_order = MSBFirst;
	ximage.bitmap_unit = 8;
	ximage.bitmap_bit_order = MSBFirst;
	ximage.bitmap_pad = 8;
	ximage.bytes_per_line = (width+7)/8;

	XPutImage(display, pix, gc, &ximage, 0, 0, 0, 0, width, height);
	XFreeGC(display, gc);
	return(pix);
}
