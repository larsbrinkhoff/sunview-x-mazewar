/* $Header: mazewar.c,v 1.13 88/08/25 09:57:53 kent Exp $ */

/* 
 * mazewar.c - Rats in a maze
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
 * This is an adaptation of the Mazewar program built at Xerox PARC in
 * Mesa for Altos by Jim Guyton and others in the late 1970s and beyond.
 * Unlike other programs that have appeared from time to time, this
 * one attempts to remain faithful to the original spirit of the game,
 * with no fancy additions (like seeing your opponents or teleport
 * traps). I hope you enjoy it.
 */

/*
 * $Log:	mazewar.c,v $
 * Revision 1.13  88/08/25  09:57:53  kent
 * Copyright.
 * 
 * Revision 1.12  88/02/11  18:00:39  kent
 * Changes so the value of M.theSocket doesn't change (this makes the
 * window system code simpler).
 * 
 * Revision 1.11  88/02/11  17:52:41  kent
 * Move some code out of the play loop so it can be called by an "external" 
 * play loop (such as is needed for the X11 toolkit).
 * 
 * Revision 1.10  87/03/31  15:47:34  kent
 * Handle duplicated RAT_NEW packets while joining the game. If the 
 * guy is already in the game, just send him a status packet; previously,
 * he'd get added to the game again!
 * 
 * Revision 1.9  87/03/31  14:37:37  kent
 * Portability considerations, especially byteswapping to/from the net.
 * 
 * Revision 1.8  86/12/04  17:44:53  kent
 * Notify user also if shot; make sure non-dukes get notified on game joins.
 * 
 * Revision 1.7  86/12/03  18:13:10  kent
 * Cleaned up the shot handling code a bit. Was waiting two seconds
 * instead of one, and would only handle one shot off the queue
 * every time around the loop, instead of all applicable.
 * 
 * Also cleaned up a race in the port moving code for when the mover and
 * the quitter were on the same machine.
 * 
 * Revision 1.6  86/12/03  13:31:10  kent
 * 
 * 
 * Revision 1.5  86/12/03  10:15:03  kent
 * Only send location when moving, not every time screen needs updating.
 * 
 * Revision 1.4  86/12/03  10:00:19  kent
 * Changes to allow multiple players per machine.
 * 
 * Revision 1.3  86/12/01  23:44:42  kent
 * Housecleaning and documentation pass.
 * 
 * Revision 1.2  86/12/01  14:47:04  kent
 * Changes for a realistic implementation of shooting.
 * 
 * Revision 1.1  86/11/26  16:57:53  kent
 * Initial revision
 * 
 */

#ifndef	lint
static char rcs_ident[] = "$Header: mazewar.c,v 1.13 88/08/25 09:57:53 kent Exp $";
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <netinet/in.h>

#include <signal.h>
#include <stdio.h>
#include <strings.h>

#include "mazewar.h"

static int		i1 = 0;		/* random number hackery */
static int		i2 = 24;

static Boolean		updateView;	/* true if update needed */
static Boolean		sendLocation;	/* true if must send out location */

static MazeTypePtr	mp = M.maze;	/* easy access to Maze */

static RatHealth	ratHealth;	/* keep track of other players */

main(argc, argv)
char **argv;
{
	int	quit();

	signal(SIGHUP, quit);
	signal(SIGINT, quit);
	signal(SIGTERM, quit);
	MazeInit(argc, argv);
	play();
}

play()
{
	MWEvent		event;
	RatPacket	incoming;
	Boolean		KBEventPending();

	event.eventDetail = &incoming;

	while (1) {
		NextEvent(&event);
		if (!M.peeking)
			switch(event.eventType) {
			case EVENT_A:
				aboutFace();
				break;

			case EVENT_S:
				leftTurn();
				break;

			case EVENT_D:
				forward();
				break;

			case EVENT_F:
				rightTurn();
				break;

			case EVENT_BAR:
				backward();
				break;

			case EVENT_I:
				makeInvincible(TRUE);
				break;

			case EVENT_K:
				makeInvincible(FALSE);
				break;

			case EVENT_O:
				makeOmniscient(TRUE);
				break;

			case EVENT_L:
				makeOmniscient(FALSE);
				break;

			case EVENT_LEFT_D:
				peekLeft();
				break;

			case EVENT_MIDDLE_D:
				shoot();
				break;

			case EVENT_RIGHT_D:
				peekRight();
				break;

			case EVENT_NETWORK:
				readRats(&event);
				break;

			case EVENT_INT:
				quit();
				break;

			}
		else
			switch (event.eventType) {
			case EVENT_RIGHT_U:
			case EVENT_LEFT_U:
				peekStop();
				break;

			case EVENT_NETWORK:
				readRats(&event);
				break;
			}
			
		ratDoctor();		/* clean house */

		DoRatKillQ();
		
		DoViewUpdate();
		
		SendLocation();
	}
}

static	Direction	_aboutFace[NDIRECTION] ={SOUTH, NORTH, WEST, EAST};
static	Direction	_leftTurn[NDIRECTION] =	{WEST, EAST, NORTH, SOUTH};
static	Direction	_rightTurn[NDIRECTION] ={EAST, WEST, SOUTH, NORTH};

aboutFace()
{
	M.dir = _aboutFace[M.dir];
	updateView = TRUE;
	sendLocation = TRUE;
}

leftTurn()
{
	M.dir = _leftTurn[M.dir];
	updateView = TRUE;
	sendLocation = TRUE;
}

rightTurn()
{
	M.dir = _rightTurn[M.dir];
	updateView = TRUE;
	sendLocation = TRUE;
}

/* remember ... "North" is to the right ... positive X motion */

forward()
{
	register int	tx = M.xloc;
	register int	ty = M.yloc;

	switch(M.dir) {
	case NORTH:	if (!mp[tx+1].y[ty])	tx++; break;
	case SOUTH:	if (!mp[tx-1].y[ty])	tx--; break;
	case EAST:	if (!mp[tx].y[ty+1])	ty++; break;
	case WEST:	if (!mp[tx].y[ty-1])	ty--; break;
	default:
		MWError("bad direction in Forward");
	}
	if ((M.xloc != tx) || (M.yloc != ty)) {
		M.xloc = tx; M.yloc = ty;
		updateView = TRUE;
		sendLocation = TRUE;
	}
}

backward()
{
	register int	tx = M.xloc;
	register int	ty = M.yloc;

	switch(M.dir) {
	case NORTH:	if (!mp[tx-1].y[ty])	tx--; break;
	case SOUTH:	if (!mp[tx+1].y[ty])	tx++; break;
	case EAST:	if (!mp[tx].y[ty-1])	ty--; break;
	case WEST:	if (!mp[tx].y[ty+1])	ty++; break;
	default:
		MWError("bad direction in Backward");
	}
	if ((M.xloc != tx) || (M.yloc != ty)) {
		M.xloc = tx; M.yloc = ty;
		updateView = TRUE;
		sendLocation = TRUE;
	}
}

makeInvincible(neverDie)
Boolean neverDie;
{
	M.invincible = neverDie;
	ShowPosition(M.xloc, M.yloc, M.invincible, M.dir);
}

makeOmniscient(allSeeing)
Boolean allSeeing;
{
	M.omniscient = allSeeing;
	ShowAllPositions();
}

peekLeft()
{
	M.xPeek = M.xloc;
	M.yPeek = M.yloc;
	M.dirPeek = M.dir;

	switch(M.dir) {
	case NORTH:	if (!mp[M.xloc+1].y[M.yloc]) {
				M.xPeek = M.xloc + 1;
				M.dirPeek = WEST;
			}
			break;

	case SOUTH:	if (!mp[M.xloc-1].y[M.yloc]) {
				M.xPeek = M.xloc - 1;
				M.dirPeek = EAST;
			}
			break;

	case EAST:	if (!mp[M.xloc].y[M.yloc+1]) {
				M.yPeek = M.yloc + 1;
				M.dirPeek = NORTH;
			}
			break;

	case WEST:	if (!mp[M.xloc].y[M.yloc-1]) {
				M.yPeek = M.yloc - 1;
				M.dirPeek = SOUTH;
			}
			break;

	default:
			MWError("bad direction in PeekLeft");
	}

	/* if any change, display the new view without moving! */

	if ((M.xPeek != M.xloc) || (M.yPeek != M.yloc)) {
		M.peeking = TRUE;
		updateView = TRUE;
	}
}

peekRight()
{
	M.xPeek = M.xloc;
	M.yPeek = M.yloc;
	M.dirPeek = M.dir;

	switch(M.dir) {
	case NORTH:	if (!mp[M.xloc+1].y[M.yloc]) {
				M.xPeek = M.xloc + 1;
				M.dirPeek = EAST;
			}
			break;

	case SOUTH:	if (!mp[M.xloc-1].y[M.yloc]) {
				M.xPeek = M.xloc - 1;
				M.dirPeek = WEST;
			}
			break;

	case EAST:	if (!mp[M.xloc].y[M.yloc+1]) {
				M.yPeek = M.yloc + 1;
				M.dirPeek = SOUTH;
			}
			break;

	case WEST:	if (!mp[M.xloc].y[M.yloc-1]) {
				M.yPeek = M.yloc - 1;
				M.dirPeek = NORTH;
			}
			break;

	default:
			MWError("bad direction in PeekRight");
	}

	/* if any change, display the new view without moving! */

	if ((M.xPeek != M.xloc) || (M.yPeek != M.yloc)) {
		M.peeking = TRUE;
		updateView = TRUE;
	}
}

peekStop()
{
	M.peeking = FALSE;
	updateView = TRUE;
}

DoViewUpdate()
{
	if (updateView) {	/* paint the screen */
		ShowPosition(M.xloc, M.yloc, M.invincible, M.dir);
		if (M.peeking)
			ShowView(M.xPeek, M.yPeek, M.dirPeek);
		else
			ShowView(M.xloc, M.yloc, M.dir);
		updateView = FALSE;
	}
}

shoot()
{
	M.score--;
	M.ratcb.rats[M.myRatId].score--;
	UpdateScoreCard(M.myRatId);
	sendKill();
}

/* 
 * add the shot to the shot queue. It'll be processed later.
 */

holdBreath(ratKill)
RatKill	ratKill;
{
	RatKillQ_t	rkp;
	struct timeval	now;
	
	DeadRatCursor();
	gettimeofday(&now, NULL);

	if (RatKillQ == NULL) {
		RatKillQ = (RatKillQ_t) malloc(sizeof(AqRatKillQ));
		rkp = RatKillQ;
	} else {
		for (rkp = RatKillQ; rkp->nextOne != NULL; rkp = rkp->nextOne)
			;
		rkp->nextOne = (RatKillQ_t) malloc(sizeof(AqRatKillQ));
		rkp = rkp->nextOne;
	}

	bcopy((char *)ratKill, (char *)&rkp->thisOne, sizeof(AqRatKill));
	rkp->nextOne = NULL;
	rkp->shotHits = now;
	rkp->shotHits.tv_sec++;
}

/* 
 * finally see if the shot hit home.
 */

handleKill(tx, ty, td, ratId)
Loc	tx, ty;
Direction td;
RatId	ratId;
{
	while (!M.maze[tx].y[ty]) {
		switch (td) {
		case NORTH:	tx++; break;
		case SOUTH:	tx--; break;
		case EAST:	ty++; break;
		case WEST:	ty--; break;
		}
		if ((M.xloc == tx) && (M.yloc == ty)) {	/* Oh oh... */
			sendDead(ratId);
			NewPosition();	/* avoid multiple hits */
			FlashScreen();
			M.score -= 5;	/* minus 5 points for getting killed */
			M.ratcb.rats[M.myRatId].score = M.score;
			UpdateScoreCard(M.myRatId);
			NotifyPlayer();
			updateView = TRUE;
			sendLocation = TRUE;
		}
	}
	RatCursor();
}

/*
 * Process the pending shots, if any.
 */

DoRatKillQ()
{
	struct timeval	now;
	RatKillQ_t	rkp = RatKillQ;
	RatKill		ratKill;

	if (RatKillQ != NULL) {
		gettimeofday(&now, NULL);
		while (rkp != NULL) {
			if (now.tv_sec >= rkp->shotHits.tv_sec) {
				if (now.tv_usec >=
				    rkp->shotHits.tv_usec) {
					ratKill = &RatKillQ->thisOne;
					handleKill(ratKill->xLoc,
						   ratKill->yLoc,
						   ratKill->dir,
						   ratKill->ratId);
					RatKillQ = RatKillQ->nextOne;
					free((char *) rkp);
					rkp = RatKillQ;
				} else
					break;
			} else
				break;
		}
		if (RatKillQ == NULL)
			RatCursor();
	}
}
		
/* 
 * Convert the contents of a packet to network order before sending.
 */

ConvertOutgoing(p)
RatPacket *p;
{
	char		buf[64];
	RatId		ratId;
	RatLocation	ratLoc;
	RatKill		ratKill;
	RatDead		ratDead;
	RatStatus	ratStatus;
	RatNew		ratNew;
	RatGone		ratGone;
	RatQuery	ratQuery;
	RatAlive	ratAlive;
	RatMove		ratMove;

	switch(p->type) {
	case RAT_LOCATION:
		ratLoc = (RatLocation) &p->body;
		ratLoc->ratId = htonl(ratLoc->ratId);
		ratLoc->xLoc =  htons(ratLoc->xLoc);
		ratLoc->yLoc =  htons(ratLoc->yLoc);
		ratLoc->dir =   htons(ratLoc->dir);
		ratLoc->score = htonl(ratLoc->score);
		break;

	case RAT_KILL:
		ratKill = (RatKill) &p->body;
		ratKill->ratId = htonl(ratKill->ratId);
		ratKill->xLoc  = htons(ratKill->xLoc);
		ratKill->yLoc  = htons(ratKill->yLoc);
		ratKill->dir   = htons(ratKill->dir);
		break;

	case RAT_DEAD:
		ratDead = (RatDead) &p->body;
		ratDead->ratId = htonl(ratDead->ratId);
		ratDead->killedBy = htonl(ratDead->killedBy);
		break;

	case RAT_STATUS:
		ratStatus = (RatStatus) &p->body;
		ratStatus->dukeRat = htonl(ratStatus->dukeRat);
		for (ratId = 0; ratId < MAXRATS; ratId++) {
			RatInfo	ratInfo;

			ratInfo = &ratStatus->rats[ratId];
			ratInfo->playing = htons(ratInfo->playing);
			ratInfo->xLoc = htons(ratInfo->xLoc);
			ratInfo->yLoc = htons(ratInfo->yLoc);
			ratInfo->dir = htons(ratInfo->dir);
			ratInfo->score = htonl(ratInfo->score);
			ratInfo->addr.sin_family =
				ntohs(ratInfo->addr.sin_family);
			/* don't touch address or name */
		}
		break;

	case RAT_NEW:
		ratNew = (RatNew) &p->body;
		ratNew->pass = htons(ratNew->pass);
		ratNew->xLoc = htons(ratNew->xLoc);
		ratNew->yLoc = htons(ratNew->yLoc);
		ratNew->dir  = htons(ratNew->dir);
		ratNew->addr.sin_family =
			htons(ratNew->addr.sin_family);
		/* don't touch address or name */
		break;

	case RAT_GOING:
		ratGone = (RatGone) &p->body;
		ratGone->ratId = htonl(ratGone->ratId);
		break;

	case RAT_QUERY:
		ratQuery = (RatQuery) &p->body;
		ratQuery->ratId = htonl(ratQuery->ratId);
		break;

	case RAT_ALIVE:
		ratAlive = (RatAlive) &p->body;
		ratAlive->ratId = htonl(ratAlive->ratId);
		break;

	case RAT_SURVEY:
		ratNew = (RatNew) &p->body;
		ratNew->pass = htons(ratNew->pass);
		ratNew->xLoc = htons(ratNew->xLoc);
		ratNew->yLoc = htons(ratNew->yLoc);
		ratNew->dir  = htons(ratNew->dir);
		/* don't touch address or name */
		break;

	case RAT_MOVE:
		ratMove = (RatMove) &p->body;
		ratMove->ratId = htonl(ratMove->ratId);
		break;

	default:
		sprintf(buf, "ConvertOutgoing bad type %d (%d)",
			p->type, htons(p->type));
		MWError(buf);
	}
	p->type = htonl(p->type);
}

/* 
 * Convert the contents of a packet to host order after ConvertIncoming.
 */

ConvertIncoming(p)
RatPacket *p;
{
	char		buf[64];
	RatId		ratId;
	RatLocation	ratLoc;
	RatKill		ratKill;
	RatDead		ratDead;
	RatStatus	ratStatus;
	RatNew		ratNew;
	RatGone		ratGone;
	RatQuery	ratQuery;
	RatAlive	ratAlive;
	RatMove		ratMove;

	p->type = ntohl(p->type);
	switch(p->type) {
	case RAT_LOCATION:
		ratLoc = (RatLocation) &p->body;
		ratLoc->ratId = ntohl(ratLoc->ratId);
		ratLoc->xLoc =  ntohs(ratLoc->xLoc);
		ratLoc->yLoc =  ntohs(ratLoc->yLoc);
		ratLoc->dir =   ntohs(ratLoc->dir);
		ratLoc->score = ntohl(ratLoc->score);
		break;

	case RAT_KILL:
		ratKill = (RatKill) &p->body;
		ratKill->ratId = ntohl(ratKill->ratId);
		ratKill->xLoc  = ntohs(ratKill->xLoc);
		ratKill->yLoc  = ntohs(ratKill->yLoc);
		ratKill->dir   = ntohs(ratKill->dir);
		break;

	case RAT_DEAD:
		ratDead = (RatDead) &p->body;
		ratDead->ratId = ntohl(ratDead->ratId);
		ratDead->killedBy = ntohl(ratDead->killedBy);
		break;

	case RAT_STATUS:
		ratStatus = (RatStatus) &p->body;
		ratStatus->dukeRat = ntohl(ratStatus->dukeRat);
		for (ratId = 0; ratId < MAXRATS; ratId++) {
			RatInfo	ratInfo;

			ratInfo = &ratStatus->rats[ratId];
			ratInfo->playing = ntohs(ratInfo->playing);
			ratInfo->xLoc = ntohs(ratInfo->xLoc);
			ratInfo->yLoc = ntohs(ratInfo->yLoc);
			ratInfo->dir = ntohs(ratInfo->dir);
			ratInfo->score = ntohl(ratInfo->score);
			ratInfo->addr.sin_family =
				ntohs(ratInfo->addr.sin_family);
			/* don't touch address or name */
		}
		break;

	case RAT_NEW:
		ratNew = (RatNew) &p->body;
		ratNew->pass = ntohs(ratNew->pass);
		ratNew->xLoc = ntohs(ratNew->xLoc);
		ratNew->yLoc = ntohs(ratNew->yLoc);
		ratNew->dir  = ntohs(ratNew->dir);
		ratNew->addr.sin_family =
			ntohs(ratNew->addr.sin_family);
		/* don't touch address or name */
		break;

	case RAT_GOING:
		ratGone = (RatGone) &p->body;
		ratGone->ratId = ntohl(ratGone->ratId);
		break;

	case RAT_QUERY:
		ratQuery = (RatQuery) &p->body;
		ratQuery->ratId = ntohl(ratQuery->ratId);
		break;

	case RAT_ALIVE:
		ratAlive = (RatAlive) &p->body;
		ratAlive->ratId = ntohl(ratAlive->ratId);
		break;

	case RAT_SURVEY:
		ratNew = (RatNew) &p->body;
		ratNew->pass = ntohs(ratNew->pass);
		ratNew->xLoc = ntohs(ratNew->xLoc);
		ratNew->yLoc = ntohs(ratNew->yLoc);
		ratNew->dir  = ntohs(ratNew->dir);
		/* don't touch address or name */
		break;

	case RAT_MOVE:
		ratMove = (RatMove) &p->body;
		ratMove->ratId = ntohl(ratMove->ratId);
		break;

	default:
		sprintf(buf, "ConvertIncoming bad type %d (%d)",
			p->type, ntohs(p->type));
		MWError(buf);
	}
}

#ifdef	PACKET_TRACE
static char	*packTypes[] = {
	"RAT_LOCATION",
	"RAT_KILL",
	"RAT_DEAD",
	"RAT_STATUS",
	"RAT_NEW",
	"RAT_GOING",
	"RAT_QUERY",
	"RAT_ALIVE",
	"RAT_SURVEY",
	"RAT_MOVE",
	0
};
#endif	PACKET_TRACE

readRats(evp)
MWEvent *evp;
{
	register RatLocation	ratLoc;
	register RatLook	ratLook;
	register RatAlive	ratAlive;
	RatPacket	*pack = evp->eventDetail;
	RatInfo		ratInfo;
	Boolean		oldVisible;
	RatId		ratId;
	RatStatus	status;
	RatNew		ratNew;
	RatGone		ratGone;
	RatKill		ratKill;
	RatDead		ratDead;
	RatQuery	ratQuery;
	RatMove		ratMove;
	Sockaddr	nullAddr;
	char		buf[32];
	int		newSocket;

#ifdef	PACKET_TRACE
	printf("received %s (%d)\n",
		packTypes[pack->type - 1], pack->type);
#endif	PACKET_TRACE

	switch(pack->type) {
	case RAT_LOCATION:		/* someone moved */
		ratLoc = (RatLocation) &pack->body;
		ratLook = &R2d2[ratLoc->ratId];
		if ((oldVisible = ratLook->visible) == TRUE)
			XORToken(ratLoc->ratId);
		ratInfo = &M.ratcb.rats[ratLoc->ratId];
		ratInfo->xLoc = ratLoc->xLoc;
		ratInfo->yLoc = ratLoc->yLoc;
		ratInfo->dir  = ratLoc->dir;
		DisplayOthersPosition(ratLoc->ratId, ratLoc->xLoc,
				      ratLoc->yLoc, ratLoc->dir);
		TokenVisible(ratLoc->ratId);
		if (ratLook->visible == TRUE)
			XORToken(ratLoc->ratId);
		if ((oldVisible != ratLook->visible) ||
		    (ratInfo->score != ratLoc->score)) {
			ratInfo->score = ratLoc->score;
			UpdateScoreCard(ratLoc->ratId);
		}
		ratHealth[ratLoc->ratId].rcvd = TRUE;
		break;

	case RAT_KILL:			/* someone shot at me */
		if (!M.invincible) {
			ratKill = (RatKill) &pack->body;
			holdBreath(ratKill);
		}
		break;

	case RAT_DEAD:			/* I hit someone */
		ratDead = (RatDead) &pack->body;
		if (ratDead->killedBy == M.myRatId) {
			FlashTop();	/* got him! */
			M.score += 10;	/* 10 points for a kill */
			M.score += 1;	/* make up for shot cost */
			M.ratcb.rats[M.myRatId].score = M.score;
			UpdateScoreCard(M.myRatId);
			sendLocToAll();
		}
		break;

	case RAT_STATUS:		/* new info about everyone */
		status = (RatStatus) &pack->body;
		if (bcmp(&status->rats[M.myRatId].addr, &M.myAddr,
			  sizeof(M.myAddr)) != 0)
			break;		/* not for me, from another game */
					/* perhaps left over from findDuke() */

		/* Have a new table, turn off any visible opponents */

		for (ratId = 0; ratId < MAXRATS; ratId++)
			if (R2d2[ratId].visible == TRUE)
				XORToken(ratId);
		bcopy((char *)status, (char *)&M.ratcb, sizeof(RatCb));
		if (M.ratcb.dukeRat == M.myRatId)
			M.duke = TRUE;
		for (ratId = 0; ratId < MAXRATS; ratId++) {
			TokenVisible(ratId);
			if (R2d2[ratId].visible == TRUE)
				XORToken(ratId);
		}
		NewScoreCard();
		ratInfo = &M.ratcb.rats[M.myRatId];
		if ((ratInfo->xLoc != M.xloc) ||
		    (ratInfo->yLoc != M.yloc) ||
		    (ratInfo->dir  != M.dir))
			sendLocToAll();
		
		break;

	case RAT_NEW:			/* new player */
		ratNew = (RatNew) &pack->body;
		if (ratNew->pass == RAT_PASSWORD) {
			if (M.duke) {
				/* 
				 * need to check for duplicates here;
				 * a previous reply might have been
				 * lost. Can't let the same guy in the
				 * game twice.
				 */

				register RatId	id;

				for (id = 0; id < MAXRATS; id++)
					if (M.ratcb.rats[id].playing &&
					    !bcmp(&M.ratcb.rats[id].addr,
						  &evp->eventSource,
						  sizeof (Sockaddr))) {

						/* already there */
						sendStatus(evp->eventSource);
						break;
					}
				if (id >= MAXRATS) {	/* fell through */
					allocateNewRat(ratNew);
					sendAllStatus();
				}
			} else
				sendStatus(evp->eventSource);
		}
		break;

	case RAT_GOING:		       /* player leaving, only rcvd if duke */
		ratGone = (RatGone) &pack->body;
		ratLeft(ratGone->ratId);
		break;

	case RAT_QUERY:			/* are you alive? */
		/*
		 * register their net address, in case they got a
		 * RAT_MOVE, moved, and the change got lost somewhere.
		 */
		ratQuery = (RatQuery) &pack->body;
		M.ratcb.rats[ratQuery->ratId].addr = evp->eventSource;

		sendAlive();
		break;

	case RAT_ALIVE:			/* I am alive */
		ratAlive = (RatAlive) &pack->body;
		ratHealth[ratAlive->ratId].rcvd = TRUE;
		break;

	case RAT_SURVEY:		/* who's out there? */
		ratNew = (RatNew) &pack->body;
		if (ratNew->pass == RAT_PASSWORD)
			sendStatus(evp->eventSource);
		break;

	case RAT_MOVE:			/* move to M.mazePort */
		ratMove = (RatMove) &pack->body;
		if (ratMove->ratId != M.ratcb.dukeRat)
			MWError("bogus RAT_MOVE");
		close(M.theSocket);

		/* 
		 * If the socket being closed is on this machine,
		 * leave some time for the socket to close down before
		 * I try to grab it.
		 */

		if (bcmp((char *) &evp->eventSource.sin_addr,
			  (char *) &M.myAddr.sin_addr,
			  sizeof(M.myAddr.sin_addr)) == 0)
			sleep(1);

		/* grab the socket */

		newSocket = socket(AF_INET, SOCK_DGRAM, 0);
		if (newSocket < 0)
			MWError("RAT_MOVE lost socket");
		if (dup2(newSocket, M.theSocket) < 0)
			MWError("RAT_MOVE dup2 failed");
		if (setsockopt(M.theSocket, SOL_SOCKET, SO_REUSEADDR,
			       NULL, 0) < 0)
			MWError("RAT_MOVE can't reuse addresses");
		M.myAddr.sin_port = M.mazePort;
		nullAddr = M.myAddr;	/* see netInit() */
		bzero((char *) &nullAddr.sin_addr, sizeof(nullAddr.sin_addr));
		if (bind(M.theSocket, &nullAddr, sizeof(nullAddr)) < 0)
			MWError("RAT_MOVE can't bind");
		M.ratcb.rats[M.myRatId].addr = M.myAddr;
		sendAllStatus();
		break;

	default:
		sprintf(buf, "readRats bad packet type 0x%x", pack->type);
		MWError(buf);
	}
}

/* 
 * In order to reduce the network traffic, only send out the location change if
 * there's no keyboard input pending.
 */

SendLocation()
{
	if (!KBEventPending())
		if (sendLocation) {
			sendLocToAll();
			sendLocation = FALSE;
		}
}

/* 
 * Let everyone know I've moved.
 */

sendLocToAll()
{
	RatPacket	pack;
	RatLocation	ratloc;
	RatId		i;
	RatInfo		ratInfo = &M.ratcb.rats[M.myRatId];

	ratInfo->xLoc = M.xloc;		/* update my table, too */
	ratInfo->yLoc = M.yloc;
	ratInfo->dir  = M.dir;
	ratInfo->score = M.score;

	pack.type = RAT_LOCATION;
	ratloc = (RatLocation) &pack.body;
	ratloc->ratId = M.myRatId;
	ratloc->xLoc = ratInfo->xLoc;
	ratloc->yLoc = ratInfo->yLoc;
	ratloc->dir = ratInfo->dir;
	ratloc->score = ratInfo->score;
	ConvertOutgoing(&pack);
	
	/* 
	 * Would really like this to be asynchronous, so play could
	 * continue while the packets are being sent. Of course, then,
	 * the data might change in the midst of all this...
	 */

	for (i = 0; i < MAXRATS; i++)
		if ((i != M.myRatId) && (M.ratcb.rats[i].playing))
			if (sendto(M.theSocket, &pack, sizeof(pack), 0,
				  &M.ratcb.rats[i].addr, sizeof(Sockaddr)) < 0)
				MWError("sendLocToAll");
}

sendAllStatus()
{
	RatId	i;

	for (i = 0; i < MAXRATS; i++)
		if ((i != M.myRatId) && (M.ratcb.rats[i].playing))
			sendStatus(M.ratcb.rats[i].addr);
}

/* 
 * Send the entire status data to a rat.
 */

sendStatus(to)
Sockaddr to;
{
	RatPacket	pack;

	pack.type = RAT_STATUS;
	pack.body = M.ratcb;
	ConvertOutgoing(&pack);
	if (sendto(M.theSocket, &pack, sizeof(pack), 0, &to, sizeof(to)) < 0)
		MWError("sendStatus");
}

/* 
 * Tell a player he's being shot at.
 */

sendKill()
{
	RatPacket	pack;
	RatKill		ratKill;
	RatId		ixRatId;

	for (ixRatId = 0; ixRatId < MAXRATS; ixRatId++) {
		if (ixRatId == M.myRatId)
			continue;
		if (R2d2[ixRatId].visible) {
			pack.type = RAT_KILL;
			ratKill = (RatKill) &pack.body;
			ratKill->ratId = M.myRatId;
			ratKill->xLoc = M.xloc;
			ratKill->yLoc = M.yloc;
			ratKill->dir  = M.dir;
			ConvertOutgoing(&pack);
			if (sendto(M.theSocket, &pack, sizeof(pack), 0,
				   &M.ratcb.rats[ixRatId].addr,
				   sizeof(M.ratcb.rats[ixRatId].addr)) < 0)
				MWError("sendKill");
		}
	}
}

/* 
 * Tell a shooter he hit me.
 */

sendDead(killerRatId)
RatId	killerRatId;
{
	RatPacket	pack;
	RatDead		ratDead;

	pack.type = RAT_DEAD;
	ratDead = (RatDead) &pack.body;
	ratDead->ratId = M.myRatId;
	ratDead->killedBy = killerRatId;
	ConvertOutgoing(&pack);
	if (sendto(M.theSocket, &pack, sizeof(pack), 0,
		  &M.ratcb.rats[killerRatId].addr, sizeof(Sockaddr)) < 0)
		MWError("sendDead");
}

/* 
 * Tell the duke I'm leaving.
 */

sendGoing()
{
	RatPacket	pack;
	RatGone		ratGone;

	pack.type = RAT_GOING;
	ratGone = (RatGone) &pack.body;
	ratGone->ratId = M.myRatId;
	ConvertOutgoing(&pack);
	if (sendto(M.theSocket, &pack, sizeof(pack), 0,
		  &M.ratcb.rats[M.ratcb.dukeRat].addr, sizeof(Sockaddr)) < 0)
		MWError("sendGoing");
}

/* 
 * Ask the silent types if they're alive.
 */

sendQuery()
{
	RatPacket	pack;
	RatId		ratId;
	RatQuery	ratQuery;

	for (ratId = 0; ratId < MAXRATS; ratId++)
		if (ratHealth[ratId].send) {
			pack.type = RAT_QUERY;
			ratQuery = (RatQuery) &pack.body;
			ratQuery->ratId = M.myRatId;
			ratHealth[ratId].send = FALSE;
			ConvertOutgoing(&pack);
			if (sendto(M.theSocket, &pack, sizeof(pack), 0,
				  &M.ratcb.rats[ratId].addr,
				  sizeof(Sockaddr)) < 0)
				MWError("sendQuery");
		}
}

/* 
 * Register someone as alive, and let them know we are, too. 
 */

sendAlive()
{
	RatPacket	pack;
	RatId		ratId;
	RatAlive	ratAlive;

	for (ratId = 0; ratId < MAXRATS; ratId++) {
		if ((ratId == M.myRatId) ||
		    !M.ratcb.rats[ratId].playing)
			continue;
		pack.type = RAT_ALIVE;
		ratAlive = (RatAlive) &pack.body;
		ratAlive->ratId = M.myRatId;
		ConvertOutgoing(&pack);
		if (sendto(M.theSocket, &pack, sizeof(pack), 0,
		      &M.ratcb.rats[ratId].addr, sizeof(Sockaddr)) < 0)
			MWError("sendAlive");
	}
}

/* 
 * Let a new player in the game.
 */

allocateNewRat(ratNew)
RatNew	ratNew;
{
	RatId	ratId;
	RatInfo	ratInfo;

	for (ratId = 0; ratId < MAXRATS; ratId++) {
		ratInfo = &M.ratcb.rats[ratId];
		if (!ratInfo->playing) {
			ratInfo->playing = TRUE;
			ratInfo->xLoc = ratNew->xLoc;
			ratInfo->yLoc = ratNew->yLoc;
			ratInfo->dir  = ratNew->dir;
			ratInfo->score = 0;
			ratInfo->addr = ratNew->addr;
			strncpy(ratInfo->name, ratNew->name, NAMESIZE);
			TokenVisible(ratId);
			UpdateScoreCard(ratId);
			if (R2d2[ratId].visible == TRUE)
				XORToken(ratId);
			AddNewPlayer(ratId, ratNew->xLoc, ratNew->yLoc,
					ratNew->dir);
			return;
		}
	}
}

/* 
 * I wanna go home!
 */

quit()
{
	RatId	ratId;

	if (!M.duke)
		sendGoing();
	else {				/* oh oh, I'm the duke rat */
		M.ratcb.rats[M.myRatId].playing = FALSE;
		for (ratId = 0; ratId < MAXRATS; ratId++)
			if (M.ratcb.rats[ratId].playing) {
				M.ratcb.dukeRat = ratId;
				sendAllStatus();
				break;
			}
		moveSomeone(M.myRatId);
	}
	StopWindow();
	exit(0);
}

/* 
 * Clean up after someone who has left. Let everyone else know, too.
 */

ratLeft(ratId)
RatId	ratId;
{
	if (R2d2[ratId].visible == TRUE)
		XORToken(ratId);
	R2d2[ratId].visible = FALSE;
	M.ratcb.rats[ratId].playing = FALSE;
	ExitPlayer(ratId);
	UpdateScoreCard(ratId);
	sendAllStatus();
	moveSomeone(ratId);
}

/* 
 * See if the guy leaving has vacated the reserved port. If so, try to
 * find someone else on that machine and tell him to move there.
 */

moveSomeone(ratId)
RatId	ratId;
{
	Sockaddr	hisAddr;
	RatId		newRat;
	RatPacket	pack;
	RatMove		ratMove;

	hisAddr = M.ratcb.rats[ratId].addr;
	if (hisAddr.sin_port != M.mazePort)
		return;

	for (newRat = 0; newRat < MAXRATS; newRat++) {
		if (newRat == ratId)
			continue;
		if (M.ratcb.rats[newRat].playing == FALSE)
			continue;
		if (!bcmp(&M.ratcb.rats[newRat].addr.sin_addr,
			  &hisAddr.sin_addr, sizeof(hisAddr.sin_addr))) {
			pack.type = RAT_MOVE;
			ratMove = (RatMove) &pack.body;
			ratMove->ratId = M.ratcb.dukeRat;
			ConvertOutgoing(&pack);
			if (sendto(M.theSocket, &pack, sizeof(pack), 0,
				  &M.ratcb.rats[newRat].addr,
				  sizeof(Sockaddr)) < 0)
				MWError("moveSomeone");

			/* 
			 * If I'm the one leaving, must free up my port.
			 */

			if (ratId == M.myRatId)
				close(M.theSocket);
			break;
		}
	}
		
}

/* 
 * Make sure nobody has died unnoticed.
 */

static Boolean		started = FALSE;
struct timeval	waitStart;
static Boolean		runDoctor = TRUE;

static
ratDoctor()
{
	RatId	ratId, nextRatId();
	struct timeval	now;

	if (!runDoctor)
		return;

	if (started == FALSE) {
		gettimeofday(&waitStart, NULL);
		for (ratId = 0; ratId < MAXRATS; ratId++) {
			ratHealth[ratId].count = 0;
			ratHealth[ratId].send = FALSE;
			ratHealth[ratId].rcvd = FALSE;
		}
		started = TRUE;
		return;
	} else {
		gettimeofday(&now, NULL);
		if ((now.tv_sec - waitStart.tv_sec) < 5)
			return;
		for (ratId = 0; ratId < MAXRATS; ratId++) {
			if ((!M.ratcb.rats[ratId].playing) ||
			    (ratId == M.myRatId))
				continue;
			if (ratHealth[ratId].rcvd) {
				ratHealth[ratId].count = 0;
				ratHealth[ratId].rcvd = FALSE;
				continue;
			}
			if (++ratHealth[ratId].count < 5) {
				ratHealth[ratId].send = TRUE;
				continue;
			}
			if (M.duke ||
			    ((M.ratcb.dukeRat == ratId) &&
			     (nextRatId(ratId) == M.myRatId))) {
				M.duke = TRUE;
				M.ratcb.dukeRat = M.myRatId;
				ratLeft(ratId);
			}
		}
		sendQuery();
		gettimeofday(&waitStart, NULL);
	}
	
}

RatId
nextRatId(ratId)
RatId	ratId;
{
	RatId	ixRatId;

	for (ixRatId = 0; ixRatId < MAXRATS; ixRatId++)
		if (M.ratcb.rats[ixRatId].playing &&
		    (ixRatId != ratId))
			return ixRatId;
	return ixRatId;
}

NewPosition()
{
	register	rndCnt = 0;

	M.xloc = M.yloc = 0;		/* start on occupied square */
	while (mp[M.xloc].y[M.yloc]) {
		M.xloc = random(MAZEXMAX);
		M.yloc = random(MAZEYMAX);
		if (++rndCnt == 100) {
			rndCnt = 0;
			InitRandom();
		}
	}

	/* prevent a blank wall at first glimpse */

	if (!M.maze[M.xloc+1].y[M.yloc]) M.dir = NORTH;
	if (!M.maze[M.xloc-1].y[M.yloc]) M.dir = SOUTH;
	if (!M.maze[M.xloc].y[M.yloc+1]) M.dir = EAST;
	if (!M.maze[M.xloc].y[M.yloc-1]) M.dir = WEST;

	return;
}

/* re-initialize the maze randomization vector */
InitRandom()
{
	struct timeval	t;
	struct timezone	tz;
	register int	i;

	gettimeofday(&t, &tz);
	for (i = 0; i < VECTORSIZE; i++)
		M.randomVector[i] = M.randomVector[i] + t.tv_sec & 0xffff;
}

random(limit)
register int	limit;
{
	register unsigned int	ret;

	ret = M.randomVector[i1] = M.randomVector[i1] + M.randomVector[i2];
	if (++i1 >= VECTORSIZE)
		i1 = 0;
	if (++i2 >= VECTORSIZE)
		i2 = 0;
	return ret%limit;
}

MWError(s)
char *s;
{
	StopWindow();
	fprintf(stderr, "MazeWar: %s\n", s);
	perror("MazeWar");
	exit(-1);
}
