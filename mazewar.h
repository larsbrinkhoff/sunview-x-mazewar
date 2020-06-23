/* $Header: mazewar.h,v 1.7 88/08/25 09:59:51 kent Exp $ */

/* 
 * mazewar.h - Definitions for MazeWar
 * 
 * Author:	Christopher A. Kent
 * 		Western Research Laboratory
 * 		Digital Equipment Corporation
 * Date:	Wed Sep 24 1986
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
 * $Log:	mazewar.h,v $
 * Revision 1.7  88/08/25  09:59:51  kent
 * Copyright.
 * 
 * Revision 1.6  88/08/11  11:37:59  kent
 * Fix a type clash with X11 include files. We need a Boolean to be a short
 * for alignment purposes, but X11 typedefs it to be a char. 
 * 
 * Revision 1.5  87/03/31  14:42:32  kent
 * Portability considerations, especially byteswapping to/from the net.
 * 
 * Revision 1.4  86/12/03  10:00:54  kent
 * Changes to allow multiple players per machine.
 * 
 * Revision 1.3  86/12/01  23:45:53  kent
 * Housecleaning and documentation pass.
 * 
 * Revision 1.2  86/12/01  14:47:52  kent
 * Changes for a realistic implementation of shooting.
 * 
 * Revision 1.1  86/11/26  16:58:19  kent
 * Initial revision
 * 
 * Revision 1.1  86/11/26  16:56:33  kent
 * Initial revision
 * 
 */

/* fundamental constants */

#ifndef	TRUE
#define	TRUE		1
#define	FALSE		0
#endif	TRUE

#define	MAZEXMAX	32
#define	MAZEYMAX	16
#define	MAXRATS		8
#define	VECTORSIZE	55
#define	NAMESIZE	20
#define	NDIRECTION	4
#define	NORTH		0
#define	SOUTH		1
#define	EAST		2
#define	WEST		3
#define	NVIEW		4
#define	LEFT		0
#define	RIGHT		1
#define	REAR		2
#define	FRONT		3

/* types */

typedef	struct sockaddr_in		Sockaddr;
#define	Boolean	short		/* don't clash with X11 toolkit */
typedef struct {Boolean	y[MAZEYMAX];}	MazeRow;
typedef	MazeRow				MazeType
	[MAZEXMAX];
typedef	MazeRow				*MazeTypePtr;
typedef	short				Direction;
typedef	struct {short	x, y; }		XYpoint;
typedef	struct {XYpoint	p1, p2;}	XYpair;
typedef	struct {short	xcor, ycor;}	XY;

typedef	struct {short	bits[16];}	BitCell;

typedef	short				Loc;
typedef	unsigned long			Score;
typedef	char				RatName
	[NAMESIZE];
typedef	long				RatId;

typedef	struct {
	RatId	ratId;
	Score	score;
	Loc	xLoc, yLoc;
	Direction	dir;
}					AqRatLocation;
typedef	AqRatLocation *			RatLocation;

typedef struct {
	RatId	ratId;
	Loc	xLoc, yLoc;
	Direction	dir;
}					AqRatKill;
typedef	AqRatKill *			RatKill;

struct _AqRatKillQ {
	AqRatKill	thisOne;
	struct _AqRatKillQ	*nextOne;
	struct timeval	shotHits;
};
typedef struct _AqRatKillQ		AqRatKillQ;
typedef	AqRatKillQ *			RatKillQ_t;

typedef	struct {
	RatId	ratId;
	RatId	killedBy;
}					AqRatDead;
typedef	AqRatDead *			RatDead;

typedef	struct {
	Sockaddr	addr;
	short		pass;
	Loc		xLoc, yLoc;
	Direction	dir;
	RatName		name;
}					AqRatNew;
typedef	AqRatNew *			RatNew;

typedef	struct {
	RatId	ratId;
}					AqRatGone;
typedef	AqRatGone *			RatGone;

typedef	struct {
	RatId	ratId;
}					AqRatQuery;
typedef	AqRatQuery *			RatQuery;

typedef struct {
	RatId	ratId;
}					AqRatAlive;
typedef	AqRatAlive *			RatAlive;

typedef struct {
	RatId	ratId;
}					AqRatMove;
typedef	AqRatMove *			RatMove;

typedef	struct {
	Sockaddr	addr;
	Score		score;
	Boolean		playing;
	Loc		xLoc, yLoc;
	Direction	dir;
	RatName		name;
}					RatObject;
typedef	RatObject *			RatInfo;

typedef	struct {
	RatId		dukeRat;
	RatObject	rats[MAXRATS];
}					RatCb;
typedef	RatCb *				RatStatus;

typedef	long				TokenId;

typedef	struct {
	Boolean	visible;
	Loc	x, y;
	short	distance;
	TokenId	tokenId;
}					RatAppearance;
typedef	RatAppearance			R2d2_t
	[MAXRATS];
typedef	RatAppearance *			RatLook;

typedef	struct {TokenId	t[NDIRECTION];}	RelativeTokens
	[NDIRECTION];

typedef	struct {
	Boolean	send;
	Boolean	rcvd;
	short	count;
}					RatNetHealth;
typedef	RatNetHealth			RatHealth
	[MAXRATS];

/* network stuff */

#define	MAZEPORT	1111
#define	MAZESERVICE	"mazewar"

#define	RAT_LOCATION	1
#define	RAT_KILL	2
#define	RAT_DEAD	3
#define	RAT_STATUS	4
#define	RAT_NEW		5
#define	RAT_GOING	6
#define	RAT_QUERY	7
#define	RAT_ALIVE	8
#define	RAT_SURVEY	9
#define	RAT_MOVE	10
#define	RAT_PASSWORD	032610

typedef	struct {
	long	type;
	RatCb	body;			/* largest type above */
}					RatPacket;

/* variables "exported" by the mazewar "module" */

struct _mazewar {
	Direction	dir;		/* which way am I looking? */
	Direction	dirPeek;	/* direction for peek display */
	Boolean	duke;			/* I am the duke */
	Sockaddr	dukeAddr;	/* where is the duke? */
	Boolean	invincible;		/* can I be killed? */
	MazeType	maze;		/* the maze in bits */
	long		mazePort;	/* the port to keep busy */
	Sockaddr	myAddr;		/* my net address */
	RatId	myRatId;		/* my index into the ratcb */
	Boolean	omniscient;		/* can I see everyone? */
	Boolean	peeking;		/* peeking in progress */
	int	theSocket;		/* the main socket */
	short	randomVector[VECTORSIZE];
	RatCb	ratcb;			/* my copy of the world */
	Score	score;			/* my total so far */
	Loc	xloc;			/* current x location */
	Loc	yloc;			/* current y location */
	Loc	xPeek;			/* x for peek display */
	Loc	yPeek;			/* y for peek display */
}M;					/* expediency wins out */

/* events */

#define	EVENT_A		1		/* user pressed "A" */
#define	EVENT_S		2		/* user pressed "S" */
#define	EVENT_D		3		/* user pressed "D" */
#define	EVENT_F		4		/* user pressed "F" */
#define	EVENT_BAR	5		/* user pressed space bar */
#define	EVENT_I		6		/* user pressed "I" */
#define	EVENT_K		7		/* user pressed "K" */
#define	EVENT_O		8		/* user pressed "O" */
#define	EVENT_L		9		/* user pressed "L" */
#define	EVENT_LEFT_D	10		/* user pressed left mouse button */
#define	EVENT_LEFT_U	11		/* user released l.m.b */
#define	EVENT_MIDDLE_D	12		/* user pressed middle button */
#define	EVENT_RIGHT_D	13		/* user pressed right button */
#define	EVENT_RIGHT_U	14		/* user released r.m.b */
#define	EVENT_NETWORK	15		/* incoming network packet */
#define	EVENT_INT	16		/* user pressed interrupt key */
#define	EVENT_TIMEOUT	17		/* nothing happened! */

typedef	struct {
	short		eventType;
	RatPacket	*eventDetail;	/* for incoming data */
	Sockaddr	eventSource;
}					MWEvent;

R2d2_t		R2d2;
RatKillQ_t	RatKillQ;

void	NextEvent();
	
char	*malloc();
