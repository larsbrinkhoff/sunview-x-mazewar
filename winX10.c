/* $Header: winX10.c,v 1.14 89/02/24 16:59:12 kent Exp $ */

/* 
 * window_X10.c - Window-system specific routines
 * 
 * Author:	Christopher A. Kent
 * 		Western Research Laboratory
 *	 	Digital Equipment Corporation
 * Date:	Thu Oct  2 1986
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
 * It is intended that this file presents a window system-independent
 * set of routines to MazeWar; that is, if you want to run MW under
 * another window system, this is the only file that needs to be
 * changed. It isn't yet clear how well I've succeeded, but it's a start.
 * 
 * Routines that are presented to the outside are capitalized; local
 * routines are not.
 *
 * This file is for X version 10.
 */

/*
 * $Log:	winX10.c,v $
 * Revision 1.14  89/02/24  16:59:12  kent
 * moved bitmaps into a subdirectory, because they're different than
 * the X11 ones now.
 * 
 * Revision 1.13  88/08/25  09:59:12  kent
 * Copyright and filename changes for system V compatability.
 * 
 * Revision 1.12  88/08/09  14:46:28  kent
 * Moved the bitmaps.
 * 
 * Revision 1.11  87/08/21  15:49:57  kent
 * Fixed things for monochrome displays. Since the rat images are a pixmap,
 * not a bitmap, XORing the foreground color does funny things. Now
 * a little more careful in how the pixmap is set up, and all is copacetic.
 * 
 * Revision 1.10  87/08/21  11:46:34  kent
 * Changed to use cursor masks and GXcopy; too many displays don't 
 * support XOR cursors, so the cursor disappears into the background at times.
 * 
 * Revision 1.9  87/03/31  14:42:38  kent
 * Portability considerations, especially byteswapping to/from the net.
 * 
 * Revision 1.8  86/12/04  17:46:58  kent
 * Make sure colors are always right when creating the iconic version.
 * 
 * Revision 1.7  86/12/04  14:36:56  kent
 * Changes to try to notify the user when a new player joins the game.
 * 
 * Revision 1.6  86/12/04  11:20:37  kent
 * Added some error return checks, so the program doesn't die with
 * a mysterious X runtime error when the server runs out of resources.
 * 
 * Revision 1.5  86/12/03  13:30:57  kent
 * 
 * 
 * Revision 1.4  86/12/03  10:00:58  kent
 * Changes to allow multiple players per machine.
 * 
 * Revision 1.3  86/12/01  23:45:19  kent
 * Housecleaning and documentation pass.
 * 
 * Revision 1.2  86/12/01  14:49:11  kent
 * Changes for a realistic implementation of shooting.
 * 
 * Revision 1.1  86/11/26  16:58:31  kent
 * Initial revision
 */

#ifndef	lint
static char rcs_ident[] = "$Header: winX10.c,v 1.14 89/02/24 16:59:12 kent Exp $";
#endif

#include <X/Xlib.h>
#include <X/Xkeyboard.h>

#include <sys/types.h>
#include <sys/time.h>

#include <netinet/in.h>

#include <errno.h>
#include <stdio.h>
#include <strings.h>

#include "mazewar.h"
#include "bitmaps/X10/icon.ic"
#include "bitmaps/X10/iconMask.ic"

#define	VIEW_X_DIM	400
#define	VIEW_Y_DIM	400
#define	VIEW_X_ORIGIN	100
#define	VIEW_Y_ORIGIN	50
#define	MAZE_X_DIM	(MAZEXMAX*16)
#define	MAZE_Y_DIM	(MAZEYMAX*16)
#define	MAZE_X_ORIGIN	48
#define	MAZE_Y_ORIGIN	451
#define	SCORE_X_DIM	192
#define	SCORE_Y_DIM	(scoreFontInfo.height * MAXRATS)
#define	SCORE_X_ORIGIN	208
#define	SCORE_Y_ORIGIN	708
#define	MIN_X_DIM	608		/* see comments for InitWindow() */
#define	MIN_Y_DIM	SCORE_Y_ORIGIN	/* see InitWindow() */

#define DEFAULT_BORDER_WIDTH 3
#define DEFAULT_FONT "helv10b"
#define DEFAULT_POSITION "=%dx%d+0+0"	/* upper left hand corner */

#define	KC_A	0302			/* interesting letter keycodes */
#define	KC_D	0315
#define	KC_F	0322
#define	KC_I	0346
#define	KC_K	0347
#define	KC_S	0307
#define	KC_Q	0301
#define	KC_O	0353
#define	KC_L	0354
#define	KC_DEL	0274
#define	KC_BAR	0324

short	mazeBits[MAZEXMAX*MAZEYMAX*16]; /* shorts better be 16 bits! */
Window	mwWindow;			/* parent window */
OpaqueFrame	mwWin;			/* frame for the window */
Window	viewWindow;			/* view subwindow */
Window	mazeWindow;			/* maze subwindow */
Window	scoreWindow;			/* score subwindow */
Window	iconWindow;			/* icon window */
Pixmap	mazePixmap;			/* Pixmap that has the maze */
Pixmap	ratsPixmap;			/* Pixmap that has the rats */
Bitmap	iconMask;			/* mask for icon outline */
int	background;			/* color of background */
int	foreground;			/* color of lines and such */
int	iconFore;			/* icon foreground */
int	iconBack;			/* icon background */
int	mouseground;			/* mouse cursor color */
int	highlight;			/* flash colors */
Font	scoreFont;			/* font for printing scores */
char	*fontName =	DEFAULT_FONT;	/* font for scores */
FontInfo	scoreFontInfo;
Boolean	mapped =	FALSE;		/* should really display? */
Boolean	inverted =	FALSE;		/* user warned of window exit */
Boolean	flashIcon =	FALSE;		/* should icon be flashing? */
Boolean	iconMapped =	FALSE;		/* is icon on screen? */
int	displayFD;			/* fd of display for events */

static short grayBits[16] = {
    0xaaaa, 0x5555, 0xaaaa, 0x5555,
    0xaaaa, 0x5555, 0xaaaa, 0x5555,
    0xaaaa, 0x5555, 0xaaaa, 0x5555,
    0xaaaa, 0x5555, 0xaaaa, 0x5555
};

#include "bitmaps/X10/rat.cur"
#include "bitmaps/X10/ratMask.cur"
#include "bitmaps/X10/dRat.cur"
#include "bitmaps/X10/dRatMask.cur"
#include "bitmaps/X10/cup.cur"

static Cursor	ratCursor, deadRatCursor, hourGlassCursor;
static int	planeMask, planePixels;

extern char	*getenv();
extern int	errno;

/* state between InitWindow() and StartWindow() */
static char	*geometry = NULL;
static int	borderPixmap;
static int	borderWidth = DEFAULT_BORDER_WIDTH;
static char	def[128];
static char	*progName;

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
	int	i;
	char	display[256];		/* will contain vs host */
	int	reverse = 0;
	char	*borderColor;
	char	*foreColor;
	char	*backColor;
	char	*highColor;
	char	*mouseColor;
	Color	cdef;
	char	*option;
	
	display[0] = '\0';
	progName = argv[0];

	if ((option = XGetDefault(argv[0],"ReverseVideo")) != NULL )
		if (strcmp (option, "on") == 0)
			reverse = 1;
	if ((option = XGetDefault(argv[0],"BorderWidth")) != NULL)
		borderWidth = atoi(option);
	if ((option = XGetDefault(argv[0],"BodyFont")) != NULL)
		fontName = option;
	if ((borderColor = XGetDefault(argv[0],"Border")) == NULL)
		borderColor = XGetDefault(argv[0],"BorderColor");
	backColor = XGetDefault(argv[0],"Background");
	foreColor = XGetDefault(argv[0],"Foreground");
	highColor = XGetDefault(argv[0],"Highlight");
	mouseColor = XGetDefault(argv[0],"Mouse");

	for (i = 1; i < argc; i++) {	/* Parse line */
		if (argv[i][0] == '=') {
			geometry = argv[i];
			continue;
		}
		if (index(argv[i], ':') != NULL) {	/* host:display */
			strncpy(display, argv[i], sizeof(display));
			continue;
		}
		if (strcmp(argv[i], "-rv") == 0 ||
		    strcmp(argv[i], "-reverse") == 0) {	/* white on black */
			reverse = 1;
			continue;
		}
		if (strcmp(argv[i], "-fw") == 0 ||
		    strcmp(argv[i], "-forward") == 0) {	/* black on white */
			reverse = 0;
			continue;
		}
		if (strcmp(argv[i], "-bw") == 0 ||
		    strcmp(argv[i], "-border") == 0) {	/* border width */
			if (++i >= argc) usage();
			borderWidth = atoi(argv[i]);
			continue;
		}
		if (strcmp(argv[i], "-fn") == 0 ||
		    strcmp(argv[i], "-font") == 0) {	/* score font */
			if (++i >= argc) usage();
			fontName = argv[i];
			continue;
		}
		if (strcmp(argv[i], "-bd") == 0 ||
		    strcmp(argv[i], "-color") == 0) {	/* border color */
			if (++i >= argc) usage();
			borderColor = argv[i];
			continue;
		}
		if (strcmp(argv[i], "-fg") == 0 ||
		    strcmp(argv[i], "-foreground") == 0) { /* foreground color */
			if (++i >= argc) usage();
			foreColor = argv[i];
			continue;
		}
		if (strcmp(argv[i], "-bg") == 0 ||
		    strcmp(argv[i], "-background") == 0) { /* background color */
			if (++i >= argc) usage();
			backColor = argv[i];
			continue;
		}
		if (strcmp(argv[i], "-ms") == 0 ||
		    strcmp(argv[i], "-mouse") == 0) { /* mouse color */
			if (++i >= argc) usage();
			mouseColor = argv[i];
			continue;
		}
		if (strcmp(argv[i], "-hl") == 0 ||
		    strcmp(argv[i], "-highlight") == 0) { /* highlight color */
			if (++i >= argc) usage();
			highColor = argv[i];
			continue;
		}
		usage();
	}

	if (borderWidth < 0) borderWidth = DEFAULT_BORDER_WIDTH;

	/* Open display  */
	if (!XOpenDisplay(display))
		MWError("cannot open display");

	/* Need a font to print scores */
	scoreFont = XGetFont(fontName);
	if (!scoreFont)
		MWError("cannot open font");
	XQueryFont(scoreFont, &scoreFontInfo);	/* or XOpenFont */

	if (DisplayCells() > 2)		/* get some color map to play with */
		XGetColorCells(0, 1, 1, &planeMask, &planePixels);
	else
		planeMask = AllPlanes;

	if (borderColor && DisplayCells() > 2 &&
	    XParseColor(borderColor, &cdef) && XGetHardwareColor(&cdef))
		borderPixmap = XMakeTile(cdef.pixel);
	else if (borderColor && strcmp(borderColor, "black") == 0)
		borderPixmap = BlackPixmap;
	else if (borderColor && strcmp(borderColor, "white") == 0)
		borderPixmap = WhitePixmap;
	else
		borderPixmap = XMakePixmap (
				(Bitmap) XStoreBitmap (16, 16, grayBits),
				BlackPixel, WhitePixel);

	if (backColor && strcmp(backColor, "white") == 0) {
		background = WhitePixel;
		reverse = 0;
	} else if (backColor && strcmp(backColor, "black") == 0) {
		background = BlackPixel;
		reverse = 0;
	} else
		background = WhitePixel;
	if (DisplayCells() > 2) {
		if (backColor == NULL)
			backColor = "white";
		if (XParseColor(backColor, &cdef)) {
			if (reverse)
				cdef.pixel = planePixels | planeMask;
			else
				cdef.pixel = planePixels;
			XStoreColor(&cdef);
			background = cdef.pixel;
		}
	}

	if (foreColor && strcmp(foreColor, "black") == 0) {
		foreground = BlackPixel;
		reverse = 0;
	} else if (foreColor && strcmp(foreColor, "white") == 0) {
		foreground = WhitePixel;
		reverse = 0;
	} else
		foreground = BlackPixel;
	if (DisplayCells() > 2) {
		if (foreColor == NULL)
			foreColor = "black";
		if (XParseColor(foreColor, &cdef)) {
			if (reverse)
				cdef.pixel = planePixels;
			else
				cdef.pixel = planePixels | planeMask;
			XStoreColor(&cdef);
			foreground = cdef.pixel;
		}
	}

	if (mouseColor && DisplayCells() > 2 &&
	    XParseColor(mouseColor, &cdef) && XGetHardwareColor(&cdef)) {
		mouseground = cdef.pixel;
	} else if (mouseColor && strcmp(mouseColor, "black") == 0) {
		mouseground = BlackPixel;
		reverse = 0;
	} else if (mouseColor && strcmp(mouseColor, "white") == 0) {
		mouseground = WhitePixel;
		reverse = 0;
	} else
		mouseground = BlackPixel;

	if (highColor && DisplayCells() > 2 &&
	    XParseColor(highColor, &cdef) && XGetHardwareColor(&cdef))
		highlight = cdef.pixel;
	else if (reverse)
		highlight = background;
	else
		highlight = foreground;

	if (reverse) {
		background = foreground;
		foreground = highlight;
	}
}

/* 
 * actually start the display up, after all the user interaction has
 * been done.
 */

StartWindow(ratBits, ratWidth, ratHeight)
short	*ratBits;
{
	WindowInfo mwInfo;

	mwWin.bdrwidth = borderWidth;
	mwWin.border = borderPixmap;
	mwWin.background = XMakeTile(background);
	sprintf(def, DEFAULT_POSITION, MIN_X_DIM,
		MIN_Y_DIM + (MAXRATS+1) * scoreFontInfo.height);
	mwWindow = XCreate ("Maze War", progName, geometry, def, &mwWin,
		   MIN_X_DIM, MIN_Y_DIM + (MAXRATS+1) * scoreFontInfo.height);

	/* set up the subwindows */
	/* see above comment for magic numbers */
	viewWindow = XCreateTransparency(mwWindow, VIEW_X_ORIGIN,
			VIEW_Y_ORIGIN, VIEW_X_DIM, VIEW_Y_DIM);
	mazeWindow = XCreateTransparency(mwWindow, MAZE_X_ORIGIN,
			MAZE_Y_ORIGIN, MAZE_X_DIM, MAZE_Y_DIM);
	scoreWindow = XCreateTransparency(mwWindow, SCORE_X_ORIGIN,
			SCORE_Y_ORIGIN, SCORE_X_DIM, SCORE_Y_DIM);
	
	/* set up the icon */
	XQueryWindow(mwWindow, &mwInfo);
	iconWindow = XCreateWindow(RootWindow,
				mwInfo.x + (mwInfo.width - icon_width) / 2,
				mwInfo.y + (mwInfo.height - icon_height) / 2,
				icon_width, icon_height, 0, 0, 0);
	XTileRelative(iconWindow);
	XSetIconWindow(mwWindow, iconWindow);
	iconMask = XStoreBitmap(icon_mask_width, icon_mask_height,
				icon_mask_bits);
	XSelectInput(iconWindow, ExposeWindow|UnmapWindow);
	iconFore = foreground;
	iconBack = background;
				
	initCursors();

	XSelectInput(mwWindow,
		KeyPressed|ButtonPressed|ButtonReleased|
		EnterWindow|LeaveWindow|ExposeWindow|UnmapWindow);
	XMapWindow(viewWindow);
	XMapWindow(mazeWindow);
	XMapWindow(scoreWindow);
	HourGlassCursor();
	XMapWindow(mwWindow);		/* Map window to screen */

	initMaze();
	FlipBitmaps();			/* bit-reverse all the bitmaps */
	initRats(ratBits, ratWidth, ratHeight);

	displayFD = dpyno();
}

/*
 * Shut down the window system, resetting any terminal modes or what
 * have you that may have been altered. No-op for X V10.
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
	XClear(viewWindow);
}

/*
 * Flash the view window to indicate a hit on an opponent.
 */

FlashTop()
{
	XCopyArea(viewWindow, 0, 0, 0, 0, VIEW_X_DIM, VIEW_Y_DIM,
		GXinvert, planeMask);
	XFlush();

	/* might want some delay here */

	XCopyArea(viewWindow, 0, 0, 0, 0, VIEW_X_DIM, VIEW_Y_DIM,
		GXinvert, planeMask);
	XFlush();
}

/*
 * Flash the whole window to indicate we were hit.
 */

FlashScreen()
{
	XCopyArea(mwWindow, 0, 0, 0, 0, mwWin.width, mwWin.height,
		GXinvert, planeMask);
	XFlush();

	/* might want some delay here */

	XCopyArea(mwWindow, 0, 0, 0, 0, mwWin.width, mwWin.height,
		GXinvert, planeMask);
	XFlush();
}

/* 
 * Draw a line in the view subwindow. Don't show it right away if possible.
 */

DrawViewLine(x1, y1, x2, y2)
{
	XLine(viewWindow, x1, y1, x2, y2, 1, 1,
		foreground, GXcopy, planeMask);
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

void
NextEvent(event)
MWEvent	*event;
{
	int	fdmask;
	XEvent	xEvent;
	struct timeval timeout;
	int	ret;

    while (1) {
	if (flashIcon) {		/* invert the icon colors */
		int	temp;

		temp = iconFore;
		iconFore = iconBack;
		iconBack = temp;
		repaintIcon();
	}

	/* must do this every time, since M.theSocket may change! */
	fdmask = (1 << displayFD) | (1 << M.theSocket);
	if (!XPending()) {
		timeout.tv_sec = 0;
		timeout.tv_usec = 250000;
		while ((ret = select(32, &fdmask, NULL, NULL, &timeout)) == -1)
			if (errno != EINTR)
				MWError("select error on events");
		if (ret == 0) {
			event->eventType = EVENT_TIMEOUT;
			return;
		}
	} else
		fdmask = 0;
	if (XPending()) {
		XNextEvent(&xEvent);
		switch (xEvent.type) {
		case KeyPressed:
			event->eventType = 0;
			switch(((XKeyEvent *) &xEvent)->detail & 0xff) {
			case KC_A:
			case KC_KEYPAD_4:
				event->eventType = EVENT_A;
				return;

			case KC_S:
			case KC_KEYPAD_5:
				event->eventType = EVENT_S;
				return;

			case KC_D:
			case KC_KEYPAD_6:
				event->eventType = EVENT_D;
				return;

			case KC_F:
			case KC_KEYPAD_COMMA:
				event->eventType = EVENT_F;
				return;

			case KC_BAR:
			case KC_CURSOR_RIGHT:
				event->eventType = EVENT_BAR;
				return;

			case KC_I:
				event->eventType = EVENT_I;
				return;

			case KC_K:
				event->eventType = EVENT_K;
				return;

			case KC_O:
				event->eventType = EVENT_O;
				return;

			case KC_L:
				event->eventType = EVENT_L;
				return;

			case KC_Q:
			case KC_DEL:
				/* should also deal with Interrupt char */
				event->eventType = EVENT_INT;
				return;
			}
			break;

#define	RightButton	0
#define	MiddleButton	1
#define	LeftButton	2
		case ButtonPressed:
			event->eventType = 0;
			switch(((XButtonPressedEvent *) &xEvent)->detail&0xff) {
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

		case ButtonReleased:
			event->eventType = 0;
			switch(((XButtonReleasedEvent *) &xEvent)->detail&0xff) {
			case RightButton:
				event->eventType = EVENT_RIGHT_U;
				return;

			case LeftButton:
				event->eventType = EVENT_LEFT_U;
				return;
			}
			break;

		case EnterWindow:
			if (xEvent.subwindow != 0)
				break;
			if ((((XEnterWindowEvent *)&xEvent)->detail&0xff) == 1)
				break;
			if (inverted == TRUE) {
				inverted = FALSE;
				invertDisplay();
			}
			break;

		case LeaveWindow:
			if (xEvent.subwindow != 0)
				break;
			if ((((XLeaveWindowEvent *)&xEvent)->detail&0xff) == 1)
				break;
			if (inverted == FALSE) {
				inverted = TRUE;
				invertDisplay();
			}
			break;

		case ExposeWindow:
			if (((XExposeWindowEvent *)&xEvent)->window
			    == iconWindow) {
				if (iconMapped == FALSE) {
					flashIcon = FALSE;
					iconMapped = TRUE;
					iconFore = foreground;
					iconBack = background;
				}
				repaintIcon();
				break;
			}
			mapped = TRUE;
			if (inverted == TRUE) {
				inverted = FALSE;
				invertDisplay();
			}
			repaintWindow();
			break;

		case UnmapWindow:
			if (((XUnmapEvent *)&xEvent)->window
			    == iconWindow) {
				flashIcon = FALSE;
				iconMapped = FALSE;
				break;
			}
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
	return (QLength() != 0);
}

/* Please stand by ... */

HourGlassCursor()
{
	XDefineCursor(mwWindow, hourGlassCursor);
	XFlush();
}

/* Let the games begin! */
RatCursor()
{
	XDefineCursor(mwWindow, ratCursor);
	XFlush();
}

/* He's dead, Jim ... or might be */
DeadRatCursor()
{
	XDefineCursor(mwWindow, deadRatCursor);
	XFlush();
}

/* update the displayed bitmap */

HackMazeBitmap(x, y, newBits)
BitCell *newBits;
{
	XBitmapBitsPut(mazeWindow, x*16, y*16, 16, 16, newBits,
			foreground, background, 0, GXcopy, planeMask);
	XFlush();
}

/*
 * Display a rat. X is clever enough that the server does all the work for
 * you; provides a generalized bitblt on Pixmaps. Hopefully other window
 * systems do as well. Width, height, srcX and srcY describe the subbitmap
 * in the rats bitmap that is the desired view of the opponent.
 */

DisplayRatBitmap(screenX, screenY, width, height, srcX, srcY)
{
	XPixmapPut(viewWindow, srcX, srcY, screenX, screenY,
		width, height, ratsPixmap, GXxor, planeMask);
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

	XText(scoreWindow, 0, rat * scoreFontInfo.height,
		M.ratcb.rats[rat].name, strlen(M.ratcb.rats[rat].name),
		scoreFont, foreground, background);
	/* or XQueryWidth for speed, must XOpenFont above */
	leftEdge = SCORE_X_DIM - XQueryWidth(buf, scoreFont, 0, 0);
	XText(scoreWindow, leftEdge, rat * scoreFontInfo.height,
		buf, strlen(buf),
		scoreFont, foreground, background);
}

/* 
 * Clear out the score line for a player that's left the game.
 */

ClearScoreLine(rat)
RatId	rat;
{
	XPixSet(scoreWindow,
		0, rat * scoreFontInfo.height,
		SCORE_X_DIM, scoreFontInfo.height,
		background);
}

/* 
 * Pretty obvious, eh? Means the guy's in sight.
 */

InvertScoreLine(rat)
RatId	rat;
{
	XCopyArea(scoreWindow,
		0, rat * scoreFontInfo.height,
		0, rat * scoreFontInfo.height,
		SCORE_X_DIM, scoreFontInfo.height,
		GXinvert, planeMask);
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
	ratCursor = XCreateCursor(16, 16, rat_bits, ratMask_bits,
				0, 0, mouseground, background, GXcopy);
	deadRatCursor = XCreateCursor(16, 16, deadRat_bits, deadRatMask_bits,
				0, 0, mouseground, background, GXcopy);
	hourGlassCursor = XCreateCursor(16, 16, coffeeCup_bits, coffeeCup_bits,
				0, 0, mouseground, background, GXcopy);
}

/*
 * construct a Pixmap of the maze, and store it in the server.
 */

initMaze()
{
	register int	i, j, k, line, index;
	Bitmap		b;

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

	b = XStoreBitmap(MAZE_X_DIM, MAZE_Y_DIM, mazeBits);
	mazePixmap = XMakePixmap(b, foreground, background);
	if (mazePixmap == 0)
		MWError("Can't create maze Pixmap");
	XFreeBitmap(b);
}

/*
 * actually put the maze Pixmap on the screen
 */

drawMaze()
{
	XPixmapPut(mazeWindow, 0, 0, 0, 0, MAZE_X_DIM, MAZE_Y_DIM,
		mazePixmap, GXcopy, planeMask);
}

/* 
 * send the rat image bitmap to the server so we can do arbitrary
 * bitblts on them later. X10 doesn't allow you to do this on bitmaps,
 * just pixmaps. Ga.
 */

initRats(bits, width, height)
short *bits;
{
	Bitmap b;

	b = XStoreBitmap(width, height, bits);
	if (DisplayCells() > 2)
		ratsPixmap = XMakePixmap(b, planeMask, 0);
	else
		ratsPixmap = XMakePixmap(b, 1, 0);
	if (ratsPixmap == 0)
		MWError("Can't create rats Pixmap");
	XFreeBitmap(b);	
}

usage()
{
	fprintf(stderr,
"usage: mazewar [-fn {font}] [-update {seconds}] [-scale {integer}] [-rv]\n"
);
	fprintf(stderr,
"             [=[{width}][x{height}][{+-}{xoff}[{+-}{yoff}]]] [[{host}]:[{vs}]]\n"
);
	fprintf(stderr,
"             [-fg {color}] [-bg {color}] [-hl {color}] [-bd {color}]
[-bw {pixels}]\n");
    exit(1);
	
}

/*
 * Repaint the window for exposure and resize events. All drawing is
 * done here, even though there are times during the initialization
 * code when it would seem obvious to put some of the display up; this
 * way portions of the display aren't shown twice then.
 */

repaintWindow()
{
	drawMaze();
	ShowPosition(M.xloc, M.yloc, M.invincible, M.dir);
	ShowView(M.xloc, M.yloc, M.dir);
	ShowAllPositions();

	NewScoreCard();
	XFlush();
}

/* 
 * Repaint the icon for exposure events, or while flashing to indicate
 * that there's a new player. 
 */

repaintIcon()
{
	int	temp;
	
	XBitmapBitsPut(iconWindow, 0, 0, icon_width, icon_height, icon_bits,
		iconFore, iconBack, iconMask, GXcopy, AllPlanes);
}

/* 
 * Invert the whole display. Used to let the user know the mouse has
 * strayed outside the window. Doesn't currently work right.
 */

invertDisplay()
{
#ifdef	notdef
	XCopyArea(mwWindow, 0, 0, 0, 0, mwWin.width, mwWin.height,
		GXinvert, planeMask);
	XFlush();
#endif
}
