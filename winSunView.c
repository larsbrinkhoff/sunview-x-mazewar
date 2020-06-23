/* $Header$ */

/* 
 * window_sunview.c - Window-system specific routines
 * 
 * Author:	Bob Brown
 * 		RIACS
 *	 	NASA Ames Research Center
 * Date:	Thu Dec 5 1986
 * Copyright (c) 1986 Robert L. Brown
 *
 * Revised by:	Malcolm Slaney (@ Schlumberger Palo Alto Research)
 *		August 1987 and January 1988
 *
 */

/* 
 * It is intended that this file presents a window system-independent
 * set of routines to MazeWar; that is, if you want to run MW under
 * another window system, this is the only file that needs to be
 * changed. It isn't yet clear how well I've succeeded, but it's a start.
 * 
 * Routines that are presented to the outside are capitalized; local
 * routines are not.
 *
 * This file is for Sun 3.1 Sunview.  It has also been tested under Sun 3.4.
 */

/*
 * $Log$
 */

#ifndef	lint
static char rcs_ident[] = "$Header$";
#endif

#include <sys/types.h>			/* I hate programs that do this... */
#include <sys/socket.h>			/* ... but sometimes
you just gotta */
#include <netinet/in.h>
#include <suntool/sunview.h>
#include <suntool/canvas.h>
#include <stdio.h>
#include "mazewar.h"

#include <errno.h>

/*
** These constants define where in the main pixwin the subregions lie
*/ 
#define	VIEW_X_DIM	400
#define	VIEW_Y_DIM	400
#define	VIEW_X_ORIGIN	100
#define	VIEW_Y_ORIGIN	50
#define	MAZE_X_DIM	(MAZEXMAX*16)
#define	MAZE_Y_DIM	(MAZEYMAX*16)
#define	MAZE_X_ORIGIN	48
#define	MAZE_Y_ORIGIN	451
#define	SCORE_X_DIM	192
#define	SCORE_Y_DIM	(mwFontHeight * MAXRATS)
#define	SCORE_X_ORIGIN	208
#define	SCORE_Y_ORIGIN	708
#define	MIN_X_DIM	608		/* see comments for InitWindow() */
#define	MIN_Y_DIM	SCORE_Y_ORIGIN	/* see InitWindow() */
#define MAX_X_DIM	MIN_X_DIM
#define	MAX_Y_DIM	(MIN_Y_DIM + SCORE_Y_DIM)

short	mazeBits[MAZEXMAX*MAZEYMAX*16]; /* shorts better be 16 bits! */
Frame	BaseFrame;			/* parent frame */
Canvas	BaseCanvas;			/* Base canvas */
Pixwin	*mwPixwin;			/* parent pixel window */
Pixwin	*viewPixwin;			/* view subwindow */
Pixwin	*mazePixwin;			/* maze subwindow */
Pixwin	*scorePixwin;			/* score subwindow */
struct pixrect *mazePixrect;		/* Pixrect that has the maze */
struct pixrect *ratsPixrect;		/* Pixrect that has the rats */
struct pixfont *mwFont;			/* Font being used */
int	mwFontHeight;			/* Height of the font */
Boolean	inverted = FALSE;		/* user warned of window exit */
int	displayFD;			/* fd of display for events */
int	frameFD;			/* for handling frame events */
int	RastOp = PIX_SRC;
int	MaxOffset;			/* max vertical font offset from 
					 * baseline */
int	flashIcon = FALSE;		/* Should icon be flashing ? */

static short grayBits[16] = {
    0xaaaa, 0x5555, 0xaaaa, 0x5555,
    0xaaaa, 0x5555, 0xaaaa, 0x5555,
    0xaaaa, 0x5555, 0xaaaa, 0x5555,
    0xaaaa, 0x5555, 0xaaaa, 0x5555
};

static short ratBits[] = {
	0x0000,0x0000,0x07C0,0x0FE8,0x0FFC,0x7FFA,0x8FFF,0x8842,
	0x4000,0x3F00,0x0080,0x0100,0x0200,0x0100,0x0000,0x0000
};
mpr_static(ratPixrect, 16, 16, 1, ratBits);

static short deadRatBits[] = {
    0x0000, 0x2088, 0x1045, 0x0082,
    0x0000, 0x1000, 0x2000, 0x4100,
    0x8220, 0x87ff, 0x8ffa, 0x7ffc,
    0x0fe8, 0x07c0, 0x0000, 0x0000
};
mpr_static(deadRatPixrect, 16, 16, 1, deadRatBits);

static short coffeeCupBits[] = {
    0x0080, 0x0700, 0x0800, 0x07c0,
    0x0020, 0x07f0, 0x3c48, 0x47f8,
    0x5808, 0x4008, 0x3c08, 0x0818,
    0x37f6, 0x2002, 0x17fc, 0x0000
};
mpr_static(coffeeCupPixrect, 16, 16, 1, coffeeCupBits);

static short ToolIconBits[] = {
/* Format_version=1, Width=64, Height=64, Depth=1, Valid_bits_per_item=16
 */
	0x8888,0x8888,0x8888,0x8888,0x8800,0x0000,0x0000,0x0008,
	0x2200,0x0000,0x0000,0x0022,0x2284,0x0000,0x0820,0x0022,
	0x8884,0x0000,0x0820,0x0008,0x88CC,0x71F1,0xC827,0x1608,
	0x22CC,0x8812,0x2928,0x9922,0x22B4,0x0822,0x2540,0x9022,
	0x88B4,0x7843,0xE547,0x9008,0x8884,0x8882,0x0548,0x9008,
	0x2284,0x8902,0x2288,0x9022,0x2284,0x79F1,0xC287,0x9022,
	0x8800,0x0000,0x0000,0x0008,0x8800,0x0000,0x0000,0x0008,
	0x2200,0x0000,0x0000,0x0022,0x2222,0x2222,0x2222,0x2222,
	0x8888,0x8888,0x8888,0x8888,0x9FFF,0xFFFF,0xFFFF,0xFFFC,
	0x3800,0x0000,0x0000,0x0006,0x3400,0x0000,0x0000,0x0006,
	0x9200,0x0000,0x0000,0x0004,0x9100,0x0000,0x0000,0x0004,
	0x3080,0x0000,0x0000,0x0006,0x3040,0x0000,0x0000,0x01FE,
	0x9020,0x0000,0x0000,0x0304,0x9010,0x0000,0x0000,0x0504,
	0x3008,0x0000,0x0000,0x0906,0x3004,0x0000,0x0000,0x1106,
	0x9002,0x0000,0x0000,0x2104,0x9001,0x0000,0x0000,0x4104,
	0x3000,0x8000,0x0000,0x8106,0x3000,0x4000,0x0001,0x0106,
	0x9000,0x3FFF,0xFFFE,0x0104,0x9000,0x2000,0x0002,0x0104,
	0x3000,0x2000,0x0002,0x0106,0x3000,0x2000,0x0002,0x0106,
	0x9000,0x2000,0x0002,0x0104,0x9000,0x2000,0x0002,0x0104,
	0x3000,0x2000,0x0002,0x0106,0x3000,0x2000,0x0002,0x0106,
	0x9000,0x2000,0x0002,0x0104,0x9000,0x2000,0x0002,0x0104,
	0x3000,0x2000,0x0002,0x0106,0x3000,0x3FFF,0xFFFE,0x0106,
	0x9000,0x4000,0x0001,0x0104,0x9000,0x8000,0x0000,0x8104,
	0x3001,0x0000,0x0000,0x4106,0x3002,0x0000,0x0000,0x2106,
	0x9004,0x0000,0x0000,0x1104,0x9008,0x0001,0xF000,0x0904,
	0x3010,0x000B,0xF800,0x0506,0x3020,0x001F,0xF800,0x0306,
	0x9040,0x002F,0xFF00,0x01FC,0x9080,0x007F,0xF880,0x0004,
	0x3100,0x0021,0x0880,0x0006,0x3200,0x0000,0x0100,0x0006,
	0x9400,0x0000,0x7E00,0x0004,0x9800,0x0000,0x8000,0x0004,
	0x3000,0x0000,0x4000,0x0006,0x3000,0x0000,0x2000,0x0006,
	0x9FFF,0xFFFF,0xFFFF,0xFFFC,0x8888,0x8888,0x8888,0x8888,
	0x2222,0x2222,0x2222,0x2222,0x2222,0x2222,0x2222,0x2222
};
DEFINE_ICON_FROM_IMAGE(ToolIcon, ToolIconBits);

static Cursor	ratCursor, deadRatCursor, hourGlassCursor;

extern char	*getenv();
extern int	errno;

/* state between InitWindow() and StartWindow() */
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
	int i;

	progName = argv[0];

	BaseFrame = window_create(NULL, FRAME, 
		FRAME_ARGC_PTR_ARGV, &argc, argv,
		FRAME_LABEL, "Maze War",
		FRAME_ICON, &ToolIcon,
		0);
	mwFont = (struct pixfont *)window_get(BaseFrame, WIN_FONT);
	mwFontHeight = mwFont->pf_defaultsize.y;
	MaxOffset = 999;
	for(i=0;i<256;i++) {
		if(MaxOffset > mwFont->pf_char[i].pc_home.y)
			MaxOffset = mwFont->pf_char[i].pc_home.y;
	}
/*	printf("maxoffset = %d\n", MaxOffset);				*/
	frameFD = (int)window_get(BaseFrame,  WIN_FD);
}

/* 
 * actually start the display up, after all the user interaction has
 * been done.
 */

StartWindow(ratBits, ratWidth, ratHeight)
short	*ratBits;
{
	int repaintWindow();

	initCursors();

	HourGlassCursor();

	initMaze();
/*	FlipBitmaps();			/* bit-reverse all the bitmaps */
	initRats(ratBits, ratWidth, ratHeight);

	BaseCanvas = window_create(BaseFrame, CANVAS,
		WIN_X, 0,
		WIN_Y, 0,
		WIN_WIDTH, MAX_X_DIM,
		WIN_HEIGHT, MAX_Y_DIM,
		0);
	window_fit(BaseFrame);
	window_set(BaseFrame, WIN_SHOW, TRUE, 0);
	window_set(BaseCanvas,
		   WIN_CONSUME_KBD_EVENTS, WIN_NO_EVENTS, 
		    WIN_ASCII_EVENTS, KBD_USE, KBD_DONE, 0, 
		   WIN_CONSUME_PICK_EVENTS, WIN_NO_EVENTS, WIN_MOUSE_BUTTONS,
		     LOC_WINENTER, LOC_WINEXIT, 0, 
		0);
	mwPixwin = (Pixwin *)window_get(BaseCanvas, CANVAS_PIXWIN);
	notify_dispatch();
	displayFD = (int)window_get(BaseCanvas, WIN_FD);
	viewPixwin = (Pixwin *)pw_region(mwPixwin,
		VIEW_X_ORIGIN, VIEW_Y_ORIGIN, VIEW_X_DIM, VIEW_Y_DIM);
	mazePixwin = (Pixwin *)pw_region(mwPixwin,
		MAZE_X_ORIGIN, MAZE_Y_ORIGIN, MAZE_X_DIM, MAZE_Y_DIM);
	scorePixwin = (Pixwin *)pw_region(mwPixwin,
		SCORE_X_ORIGIN, SCORE_Y_ORIGIN, SCORE_X_DIM, SCORE_Y_DIM);
	window_set(BaseCanvas, 
		CANVAS_REPAINT_PROC, repaintWindow, 
		CANVAS_RESIZE_PROC, repaintWindow,
		CANVAS_RETAINED, TRUE,
		CANVAS_AUTO_CLEAR, TRUE,
		0);
	repaintWindow();
}

/*
 * Shut down the window system, resetting any terminal modes or what
 * have you that may have been altered. No-op for Sun View
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
	pw_writebackground(viewPixwin, 0, 0, VIEW_X_DIM, VIEW_Y_DIM,
	    RastOp);
}

/*
 * Flash the view window to indicate a hit on an opponent.
 */

FlashTop()
{
	pw_rop(viewPixwin, 0, 0, VIEW_X_DIM, VIEW_Y_DIM,
	    PIX_NOT(PIX_SRC), viewPixwin, 0, 0);

	/* might want some delay here */

	pw_rop(viewPixwin, 0, 0, VIEW_X_DIM, VIEW_Y_DIM,
	    PIX_NOT(PIX_SRC), viewPixwin, 0, 0);
}

/*
 * Flash the whole window to indicate we were hit.
 */

FlashScreen()
{
	pw_rop(mwPixwin, 0, 0, window_get(BaseCanvas, WIN_WIDTH),
	    window_get(BaseCanvas, WIN_HEIGHT), PIX_NOT(PIX_SRC),
	    mwPixwin, 0, 0);

	/* might want some delay here */

	pw_rop(mwPixwin, 0, 0, window_get(BaseCanvas, WIN_WIDTH),
	    window_get(BaseCanvas, WIN_HEIGHT), PIX_NOT(PIX_SRC),
	    mwPixwin, 0, 0);
}

/* 
 * Draw a line in the view subwindow. Don't show it right away if possible.
 */

DrawViewLine(x1, y1, x2, y2)
{
	pw_vector(viewPixwin, x1, y1, x2, y2, RastOp, 1);
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
	Event	sunEvent;
	struct timeval timeout;
	int	ret;
	static	int	LastWinEnterExit = 0;

    while (1) {
	if (flashIcon == TRUE){
		if (window_get(BaseFrame, FRAME_CLOSED)){
			InvertIcon();
		} else {
			flashIcon = FALSE;
			window_set(BaseFrame, FRAME_ICON, &ToolIcon, 0);
		}
	}

	/* must do this every time, since M.theSocket may change! */
	fdmask = (1 << displayFD) | (1 << frameFD) | (1 << M.theSocket);
	timeout.tv_sec = 0;
	timeout.tv_usec = 250000;
	while ((ret = select(32, &fdmask, NULL, NULL, &timeout)) == -1){
						/* EINTR implies interrupts
						 * which we don't care about.
						 * EBADF implies that things
						 * are probably being shut down
						 * so we should exit.
						 */
		if (errno == EBADF){
			event->eventType = EVENT_INT;
			return;
		}
		if (errno != EINTR)
			MWError("select error on events");
	}
	if (ret == 0) {
		event->eventType = EVENT_TIMEOUT;
		(void) notify_dispatch();
		return;
	}
	if (fdmask & (1 <<displayFD)) {
		window_read_event(BaseCanvas, &sunEvent);
		event->eventType = 0;
		switch (sunEvent.ie_code) {
		case 'a':
			event->eventType = EVENT_A;
			return;

		case 's':
			event->eventType = EVENT_S;
			return;

		case 'd':
			event->eventType = EVENT_D;
			return;

		case 'f':
			event->eventType = EVENT_F;
			return;

		case ' ':
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
		case 077:
			/* should also deal with Interrupt char */
			event->eventType = EVENT_INT;
			return;

		case MS_RIGHT:
			event->eventType = event_is_down(&sunEvent) ?
				EVENT_RIGHT_D : EVENT_RIGHT_U;
			return;

		case MS_MIDDLE:
			event->eventType = event_is_down(&sunEvent) ?
				EVENT_MIDDLE_D : 0;
			return;

		case MS_LEFT:
			event->eventType = event_is_down(&sunEvent) ?
				EVENT_LEFT_D : EVENT_LEFT_U;
			return;

		case KBD_USE:
#ifdef	DEBUG			
			printf("Have keyboard focus again.....\n");
#endif	DEBUG			
			if (RastOp != PIX_SRC) {
				invertDisplay();
			}
			RastOp = PIX_SRC;
			break;

		case KBD_DONE:
#ifdef	DEBUG			
			printf("Lost keyboard focus.....\n");
#endif	DEBUG			
			if (RastOp != PIX_NOT(PIX_SRC)) {
				invertDisplay();
			}
			RastOp = PIX_NOT(PIX_SRC);
			break;
		case WIN_REPAINT:
			repaintWindow();
			break;
		default:
/*			printf("Window event %d\n", sunEvent.ie_code);	*/
			break;
		}
		continue;
	}
	if (fdmask & (1 << M.theSocket)) {
		int fromLen = sizeof(event->eventSource);
		int cc;

		event->eventType = EVENT_NETWORK;
		cc = recvfrom(M.theSocket, event->eventDetail,
			sizeof(RatPacket), 0, &event->eventSource, &fromLen);
		ConvertIncoming(event->eventDetail);
		if (cc <= 0) {
			if (cc < 0 && errno != EINTR)
				perror("event recvfrom");
			continue;
		}
		if (fromLen != sizeof(struct sockaddr_in))
			continue;
		return;
	}
	if ( fdmask & (1 << frameFD)) {
		notify_dispatch();
	}
    }
}

/* Please stand by ... */

HourGlassCursor()
{
	window_set(BaseCanvas, WIN_CURSOR, hourGlassCursor, 0);
}

/* Let the games begin! */
RatCursor()
{
	window_set(BaseCanvas, WIN_CURSOR, ratCursor, 0);
}

/* He's dead, Jim ... or might be */
DeadRatCursor()
{
	window_set(BaseCanvas, WIN_CURSOR, deadRatCursor, 0);
}

/* update the displayed bitmap */

mpr_static(Hack_pr, 16, 16, 1, NULL);

HackMazeBitmap(x, y, newBits)
BitCell *newBits;
{
	Hack_pr_data.md_image = newBits->bits;
	pw_rop(mazePixwin, x*16, y*16, 16, 16, RastOp, &Hack_pr, 0, 0);
}

/*
 * Display a rat. Width, height, srcX and srcY describe the subbitmap
 * in the rats bitmap that is the desired view of the opponent.
 */

DisplayRatBitmap(screenX, screenY, width, height, srcX, srcY)
{
	pw_rop(viewPixwin, screenX, screenY, width, height, PIX_SRC^PIX_DST, 
		ratsPixrect, srcX, srcY);
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
	struct pr_size size;

	pw_text(scorePixwin, 0, (rat+1)*mwFontHeight, RastOp, mwFont, 
		M.ratcb.rats[rat].name);
	sprintf(buf, "%d", (unsigned int)M.ratcb.rats[rat].score);
	size = pf_textwidth(strlen(buf), mwFont, buf);
	leftEdge = SCORE_X_DIM - size.x;
	pw_text(scorePixwin, leftEdge, (rat+1)*mwFontHeight, RastOp, mwFont,
		buf);
}

/* 
 * Clear out the score line for a player that's left the game.
 */

ClearScoreLine(rat)
RatId	rat;
{
	pw_writebackground(scorePixwin, 0, (rat+1)*mwFontHeight+MaxOffset, 
		SCORE_X_DIM, mwFontHeight, RastOp);
}

/* 
 * Pretty obvious, eh? Means the guy's in sight.
 */

InvertScoreLine(rat)
RatId	rat;
{
	pw_writebackground(scorePixwin, 0, (rat+1)*mwFontHeight+MaxOffset, 
		SCORE_X_DIM, mwFontHeight, PIX_NOT(PIX_DST));
}

/*
 * END PUBLIC ROUTINES
 */

/* set up needed bitmaps in the server */

initCursors()
{
	ratCursor = cursor_create(CURSOR_IMAGE, &ratPixrect, 
		CURSOR_OP, PIX_SRC^PIX_DST, 
		0);
	deadRatCursor = cursor_create(CURSOR_IMAGE, &deadRatPixrect, 
		CURSOR_OP, PIX_SRC^PIX_DST, 
		0);
	hourGlassCursor = cursor_create(CURSOR_IMAGE, &coffeeCupPixrect, 
		CURSOR_OP, PIX_SRC^PIX_DST, 
		0);
}

/*
 * construct a Pixmap of the maze, and store it in the server.
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
	mazePixrect = mem_point(MAZE_X_DIM, MAZE_Y_DIM, 1, mazeBits);
}

/*
 * actually put the maze Pixmap on the screen
 */

drawMaze()
{
	pw_rop(mazePixwin, 0, 0, MAZE_X_DIM, MAZE_Y_DIM, RastOp, 
		mazePixrect, 0, 0);
}

/* 
 * Create a memory pixrect of the rats so we can do arbitrary
 * bitblts on them later.
 */

initRats(bits, width, height)
short *bits;
{
	ratsPixrect = mem_point(width, height, 1, bits);
}

usage()
{
	fprintf(stderr,"usage: mazewar\n");
	exit(1);
	
}

repaintWindow()
{
#ifdef	DEBUG
	printf("repaint window called\n");
	if (RastOp == PIX_SRC)
		printf("repaintWindow called with RastOp = PIX_SRC.\n");
	else
		printf(
		     "repaintWindow called with RastOp = PIX_NOT(PIX_SRC).\n");
#endif	DEBUG	
	if (RastOp != PIX_SRC)
		pw_rop(mwPixwin, 0, 0, (int)window_get(BaseCanvas, WIN_WIDTH), 
		       (int)window_get(BaseCanvas, WIN_HEIGHT),
		       PIX_NOT(PIX_CLR), 
		       mwPixwin, 0, 0);
	drawMaze();
	ShowPosition(M.xloc, M.yloc, M.invincible, M.dir);
	ShowView(M.xloc, M.yloc, M.dir);
	ShowAllPositions();

	NewScoreCard();
}
/* 
 * Invert the whole display. Used to let the user know the mouse has
 * strayed outside the window. 
 */

invertDisplay()
{
	pw_rop(mwPixwin, 0, 0, (int)window_get(BaseCanvas, WIN_WIDTH), 
		(int)window_get(BaseCanvas, WIN_HEIGHT), PIX_NOT(PIX_SRC), 
		mwPixwin, 0, 0);
}

/*
* Peek to see if there's a keyboard event waiting, in case the
* program wants to short-circuit some code. If your system won't let
* you implement this feature easily, just always return FALSE.
*/

Boolean
KBEventPending()
{
	    return (FALSE);
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


InvertIcon(){
	Icon	icon;
	Pixrect	*pr;

	icon = (Icon) window_get(BaseFrame, FRAME_ICON);
	pr = (Pixrect *) icon_get(icon, ICON_IMAGE);

	pr_rop(pr, 0, 0, 64, 64, PIX_NOT(PIX_DST), pr, 0, 0);

	icon_set(icon, ICON_IMAGE, pr, 0);
	window_set(BaseFrame, FRAME_ICON, icon, 0);
}
