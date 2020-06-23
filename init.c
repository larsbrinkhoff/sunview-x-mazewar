/* $Header: init.c,v 1.13 88/09/07 14:37:21 kent Exp $ */

/* 
 * init.c - Initialization code for mazewar
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
 * $Log:	init.c,v $
 * Revision 1.13  88/09/07  14:37:21  kent
 * Portability changes for broadcasts.
 * 
 * Revision 1.12  88/08/25  09:57:53  kent
 * Copyright and changed to allow the broadcast address to be set in Makefile.
 * 
 * Revision 1.11  88/06/15  16:36:31  kent
 * Updated to use proper broadcast address.
 * 
 * Revision 1.10  87/07/28  13:51:16  kent
 * Fixed a problem in netInit(); didn't handle the case when gethostname()
 * returned something that couldn't be found in the host table.
 * 
 * Revision 1.9  87/03/31  14:42:14  kent
 * Portability considerations, especially byteswapping to/from the net.
 * 
 * Revision 1.8  86/12/04  17:48:05  kent
 * findDuke's return value pointed to something on the stack!
 * 
 * Revision 1.7  86/12/04  11:16:54  kent
 * Handle name case when there's no comma in the gecos field.
 * 
 * Revision 1.6  86/12/04  11:14:23  kent
 * Clean up join, so you can specify any host in a game, not just the duke.
 * 
 * Revision 1.5  86/12/03  13:31:25  kent
 * 
 * 
 * Revision 1.4  86/12/03  10:00:45  kent
 * Changes to allow multiple players per machine.
 * 
 * Revision 1.3  86/12/01  23:44:25  kent
 * Housecleaning and documentation pass.
 * 
 * Revision 1.2  86/12/01  14:48:17  kent
 * Changes for a realistic implementation of shooting.
 * 
 * Revision 1.1  86/11/26  16:57:42  kent
 * Initial revision
 * 
 */

#ifndef	lint
static char rcs_ident[] = "$Header: init.c,v 1.13 88/09/07 14:37:21 kent Exp $";
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <netinet/in.h>

#include <errno.h>
#include <netdb.h>
#include <pwd.h>
#include <stdio.h>
#include <strings.h>

#include "mazewar.h"

int	RV[VECTORSIZE] = {
	0031575, 0055455, 0147160, 0176745, 0173126, 0117426, 0033612, 0136020,
	0054013, 0167672, 0070252, 0033100, 0015700, 0113523, 0170465, 0024344,
	0175535, 0137325, 0126211, 0010207, 0173547, 0016071, 0056622, 0014433,
	0113225, 0047553, 0103024, 0110174, 0000124, 0173304, 0076700, 0104042,
	0135030, 0126234, 0175154, 0140123, 0167542, 0000405, 0035464, 0166537,
	0050260, 0167655, 0123715, 0176164, 0172206, 0140365, 0074606, 0075656,
	0176163, 0030017, 0022102, 0040051, 0154620, 0017144, 0073372
};

static MazeType mazeBits = {
	{ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},	/* 0 */
	{ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{ 1,0,1,1,1,1,1,0,1,1,1,1,0,1,1,1},
	{ 1,0,0,0,0,0,1,0,1,0,0,0,0,0,0,1},

	{ 1,0,1,0,1,0,0,0,1,0,1,1,1,1,0,1},	/* 1 */
	{ 1,0,1,1,1,0,1,0,1,0,1,0,0,0,0,1},
	{ 1,0,0,0,0,0,1,0,0,0,1,0,1,1,0,1},
	{ 1,1,1,1,1,0,1,1,1,0,0,0,0,0,0,1},

	{ 1,0,0,0,0,0,0,0,1,0,1,1,1,1,0,1},	/* 2 */
	{ 1,0,1,1,1,1,1,0,1,0,1,0,0,0,0,1},
	{ 1,0,0,0,0,0,1,0,0,0,1,0,1,1,1,1},
	{ 1,0,1,1,1,0,1,0,1,0,1,0,0,0,0,1},

	{ 1,0,0,0,0,0,1,0,1,0,1,1,1,1,0,1},	/* 3 */
	{ 1,0,1,1,1,1,1,0,1,0,1,1,0,1,0,1},
	{ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{ 1,0,1,1,1,1,1,1,1,0,1,0,1,1,1,1},

	{ 1,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1},	/* 4 */
	{ 1,1,1,0,1,0,1,0,1,0,1,0,1,1,0,1},
	{ 1,0,0,0,1,0,1,0,1,0,1,0,0,1,0,1},
	{ 1,0,1,1,1,0,1,0,1,0,1,1,0,0,0,1},

	{ 1,0,0,0,0,0,0,0,1,0,1,0,0,1,0,1},	/* 5 */
	{ 1,0,1,1,1,1,1,1,1,0,1,0,1,1,0,1},
	{ 1,0,0,0,1,0,0,0,1,0,1,0,0,1,0,1},
	{ 1,0,1,0,1,0,1,0,1,0,1,1,0,0,0,1},

	{ 1,0,1,0,1,0,1,0,1,0,0,0,0,1,0,1},	/* 6 */
	{ 1,0,1,0,1,0,1,0,1,0,1,0,1,1,0,1},
	{ 1,0,1,0,1,0,1,0,1,0,1,0,0,1,0,1},
	{ 1,0,1,0,0,0,1,0,1,0,1,1,0,1,0,1},

	{ 1,0,1,0,1,0,1,0,0,0,1,0,0,0,0,1},	/* 7 */
	{ 1,0,1,0,1,0,1,0,1,0,1,1,1,1,0,1},
	{ 1,0,0,0,1,0,0,0,1,0,0,0,0,1,0,1},
	{ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

static char	*ratName = NULL;
static char	*dukeName = NULL;
static Sockaddr	dukeAddr;

Sockaddr	*resolveHost();

extern int	errno;

MazeInit(argc, argv)
char	**argv;
{
	register RatId	ratId;

	for (ratId = 0; ratId < MAXRATS; ratId++) {
		bzero((char *) M.ratcb.rats[ratId].name, NAMESIZE);
		M.ratcb.rats[ratId].playing = FALSE;
		R2d2[ratId].visible = FALSE;
	}
	M.invincible = FALSE;
	M.omniscient = FALSE;
	RatKillQ = (RatKillQ_t) NULL;

	getMaze();

	setRandom();
	InitRandom();

	InitDisplay(argc, argv);
	getNames();
	NewPosition();

	/*
	 * We don't do ShowPosition() or ShowView() here, but let the update
	 * routine in the window handler do it for the first time when
	 * the window is exposed.
	 */

	netInit();
	StartDisplay();
	RatCursor();
}

/* get the maze into memory */
getMaze()
{
	int i, j;

	for (i = 0; i < MAZEXMAX; i++)
		for (j = 0; j < MAZEYMAX; j++)
			if (mazeBits[i].y[MAZEYMAX-1 - j] == 1)
				M.maze[i].y[j] = TRUE;
			else
				M.maze[i].y[j] = FALSE;
}

/* initialize the random vector */
setRandom()
{
	register	i;

	for (i = 0; i < VECTORSIZE; i++)
		M.randomVector[i] = RV[i];
}

/* get player and duke host names */
getNames()
{
	char		buf[128], *cp;
	char		*comma;
	Sockaddr	*duke;
	char		*getenv();
	struct passwd	*getpwuid();

	buf[0] = '\0';
	printf("Welcome to Alto MazeWar!\n\n");
	printf("Your Name: ");
	gets(buf);
	if (buf[0] == '\0') {
		if ((cp = getenv("USERNAME")) != NULL)
			strcpy(buf, cp);
		else {
			strcpy(buf, (getpwuid(getuid()))->pw_gecos);
			comma = index(buf, ',');
			if (comma != NULL)
				*comma = '\0';
		}
	}
	ratName = malloc((unsigned) (strlen(buf) + 1));
	if (ratName == NULL)
		MWError("no mem for ratName");
	strcpy(ratName, buf);
	for (duke = NULL; duke == NULL; ) {
#ifndef	NO_BROADCAST
		printf("Duke Host (CR for any game): ");
#else
		printf("Duke Host: ");
#endif	NO_BROADCAST
		gets(buf);
		if (strlen(buf) == 0)
			break;
		dukeName = malloc((unsigned) (strlen(buf) + 1));
		if (dukeName == NULL)
			MWError("no mem for dukeName");
		strcpy(dukeName, buf);

		/* check for valid duke name */
		duke = resolveHost(dukeName);
		if (duke == (Sockaddr *) NULL) {
			printf("Don't know host %s\n", dukeName);
			free(dukeName);
			dukeName = NULL;
		}
	}
	if ((dukeName != NULL) &&
	    (strlen(dukeName) != 0))
		bcopy((char *) duke, (char *) &dukeAddr, sizeof(Sockaddr));
}

netInit()
{
	register RatId	ratId;
	Boolean		joinGame();
	struct servent	*mazeService;
	u_short		port;
	Sockaddr	nullAddr;
	Sockaddr	*thisHost;
	char		buf[128];

	mazeService = getservbyname(MAZESERVICE, "udp");
	if (mazeService != (struct servent *) NULL)
		M.mazePort = mazeService->s_port;
	else
		M.mazePort = htons(MAZEPORT);

	gethostname(buf, sizeof(buf));
	if ((thisHost = resolveHost(buf)) == (Sockaddr *) NULL)
		MWError("who am I?");
	bcopy((caddr_t) thisHost, (caddr_t) &M.myAddr, sizeof(Sockaddr));
	nullAddr = M.myAddr;
	bzero((char *)&nullAddr.sin_addr, sizeof(nullAddr.sin_addr));

	/* 
	 * Figure out which port I can use. Start at MAZEPORT and go
	 * up till there's a free one. We use nullAddr so that we
	 * can receive any incoming packet, in case there's some
	 * multi-homed nonsense. However, not binding may cause troubles
	 * if a specified duke is on a net that causes the source of the
	 * packets to be something other than what M.myAddr is...
	 * There also seems to be a problem with broadcasts not being
	 * accepted on sockets that have an address bound to them.
	 */

	M.theSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (M.theSocket < 0)
		MWError("can't get socket");

	for (port = ntohs(M.mazePort); ; port++) {
		nullAddr.sin_port = htons(port);
		if (bind(M.theSocket, &nullAddr, sizeof(nullAddr)) < 0)
			if (errno != EADDRINUSE)
				MWError("netInit binding");
			else
				continue;
		else
			break;
	}
	M.myAddr.sin_port = nullAddr.sin_port;

	/*
	 * Now we can try to find a game to join; if none, start one.
	 */

	if (!joinGame())
		startGame();
	printf("\n");

	for (ratId = 0; ratId < MAXRATS; ratId++)
		TokenVisible(ratId);	/* oh oh say can we see? */

}

/* 
 * Find a game. If the user specified a "duke" host, contact that
 * duke. Otherwise broadcast to find a duke. Finally join that game.
 */

Boolean
joinGame()
{
	Sockaddr	*duke, *findDuke();
	Boolean		join();

	if ((dukeName == NULL) ||
	    (strlen(dukeName) == 0)) {
		duke = findDuke();
		if (duke != NULL)
			bcopy((char *) duke, (char *) &dukeAddr,
				sizeof(Sockaddr));
		else
			return FALSE;
	} else
		dukeAddr.sin_port = M.mazePort;
	return join(dukeAddr);
}

/* 
 * No game to join; set ourselves up as duke and wait for others to join.
 */

startGame()
{
	RatId	i;
	RatInfo		rsp;

	printf("starting a new game");
	M.myRatId = 0;
	M.score = 0;
	SetMyRatId(0);
	M.ratcb.dukeRat = M.myRatId;
	M.ratcb.rats[M.myRatId].addr = M.myAddr;
	rsp = &M.ratcb.rats[M.myRatId];
	rsp->playing = TRUE;
	rsp->xLoc = M.xloc;
	rsp->yLoc = M.yloc;
	rsp->dir = M.dir;
	rsp->score = M.score;
	rsp->addr = M.myAddr;
	strncpy(rsp->name, ratName, NAMESIZE);
	for (i = 1; i < MAXRATS; i++)
		M.ratcb.rats[i].playing = FALSE;
	M.duke = TRUE;
}

/* 
 * Join an existing game. Send a RAT_NEW packet and wait for a
 * RAT_STATUS packet to come back with the other players. 
 */

Boolean
join(addr)
Sockaddr addr;
{
	RatPacket	b, r;
	RatNew		ratNew;
	RatId		id;
	RatStatus	status;
	int		i, ret;
	int		fds;
	struct timeval	timeout;
	Boolean		retVal = FALSE;
	
	b.type = RAT_NEW;
	ratNew = (RatNew) &b.body;
	ratNew->pass = RAT_PASSWORD;
	ratNew->xLoc = M.xloc;
	ratNew->yLoc = M.yloc;
	ratNew->dir = M.dir;
	ratNew->addr = M.myAddr;
	strncpy(ratNew->name, ratName, NAMESIZE);
	free(ratName);
	ConvertOutgoing(&b);

	for (i = 0; i < 5; i++) {
		int fromlen;
		struct sockaddr from;

		if (sendto(M.theSocket, (char *) &b, sizeof(b), 0, &addr,
			   sizeof(addr)) < 0)
			MWError("join sendto");
		fds = (1<<M.theSocket);
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		ret = select(32, &fds, NULL, NULL, &timeout);
		if (ret < 0)
			MWError("join select");
		if (ret == 0)
			continue;
		if (recvfrom(M.theSocket, (char *) &r, sizeof(r),
				0, &from, &fromlen) < 0)
			MWError("join status receive");
		ConvertIncoming(&r);
		if (r.type != RAT_STATUS)
			continue;

		/* see if I got into the game */
		status = &r.body;
		for (id = 0; id < MAXRATS; id++)
			if (status->rats[id].playing &&
			    !bcmp(&status->rats[id].addr, &M.myAddr,
				  sizeof(M.myAddr))) {
				M.myRatId = id;
				break;
			}

		/* 
		 * Didn't find me; this guy must not really be the
		 * duke. Contact the duke of the game and get added.
		 */

		if (id == MAXRATS) {
			addr = status->rats[status->dukeRat].addr;
			continue;
		}
		
		bcopy((char *) status, (char *) &M.ratcb, sizeof(RatCb));
		retVal = TRUE;
		break;
	}
	return retVal;
}

/* 
 * Find a "duke" host. Broadcast a RAT_SURVEY packet to MAZEPORT.
 *
 * The broadcast is done just on the network that is associated
 * with the name returned by gethostname(); for most workstations
 * this is accurate, but multi-homed hosts may miss some games. Oh
 * well.
 *
 * After the broadcast, collect responses. This goes on until no packets
 * have been received for 5 seconds or the max number have been seen
 *
 * All players are supposed to respond. If a packet comes from a non-duke,
 * the information is inspected to send a RAT_SURVEY directly to him.
 * Otherwise, the answer is saved. When all the answers are in, the
 * first game with an empty slot is chosen.
 *
 * If all players were listening on MAZEPORT, we could probably do
 * without the resending nonsense. Since the duke might not be on
 * MAZEPORT, this is necessary. It also adds a certain robustness, in
 * case the duke drops the packet or somesuch. Care is taken elsewhere
 * (ratLeft() and ratDoctor())to make sure there are always players
 * listening on MAZEPORT.
 */

struct answer {
	Sockaddr	source;
	int		srclen;
	RatPacket	packet;
};

Sockaddr *
findDuke()
{
#ifndef	NO_BROADCAST
	register int	i;
	int		cnt = 0;
	int		maxAnswers = 10;
	static Sockaddr	rmtAddr;
	struct	timeval	timeout;
	RatPacket	*b = (RatPacket *) malloc(sizeof(RatPacket));
	struct answer	*answers;
	int		fds, fd = M.theSocket;
	int		ret, cc;
	Boolean		duke(), freeSlot();
	struct in_addr	inet_makeaddr();

	printf("Checking out available games..."); fflush(stdout);

#ifdef	SO_BROADCAST
#ifdef	BSD_43
	{
	int	one = 1;

	if (setsockopt(M.theSocket, SOL_SOCKET, SO_BROADCAST, 
				&one, sizeof one) < 0)
		MWError("can't get broadcast permission");
	}
#else	BSD_43
	if (setsockopt(M.theSocket, SOL_SOCKET, SO_BROADCAST, NULL, 0) < 0)
		MWError("can't get broadcast permission");
#endif	BSD_43
#endif	SO_BROADCAST

	rmtAddr = M.myAddr;
	rmtAddr.sin_addr = inet_makeaddr(inet_netof(M.myAddr.sin_addr),
					BROAD_ADDR);	/* from Makefile */
	rmtAddr.sin_port = M.mazePort;

	answers = (struct answer *)
		malloc((unsigned) (maxAnswers * sizeof(*answers)));

	setupSurvey(b);
	ConvertOutgoing(b);
	if (sendto(fd, (char *) b, sizeof(*b), 0, &rmtAddr, sizeof(rmtAddr)) < 0)
		fprintf(stderr, "findDuke survey can't broadcast\n");

	while (1) {
		fds = (1<<fd);
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		ret = select(32, &fds, NULL, NULL, &timeout);
		if (ret < 0)
			MWError("findDuke select");
		if (ret == 0)
			break;
		answers[cnt].srclen = sizeof(Sockaddr);
		if (recvfrom(fd, &answers[cnt].packet, sizeof(RatPacket), 0,
			     &answers[cnt].source, &answers[cnt].srclen) < 0)
			MWError("findDuke recv");
		ConvertIncoming(&answers[cnt].packet);
		if (answers[cnt].packet.type != RAT_STATUS)
			continue;
		if (!duke(&answers[cnt])) {
			RatStatus	rs;

			rs = (RatStatus) &answers[cnt].packet.body;
			setupSurvey(b);
			ConvertOutgoing(b);
			if (sendto(fd, b, sizeof(*b), 0,
				   &rs->rats[rs->dukeRat].addr,
				   sizeof(rs->rats[rs->dukeRat].addr)) < 0)
				fprintf(stderr, 
					"findDuke resend can't broadcast");
			continue;
		}
		if (freeSlot(&answers[cnt]))
			if (++cnt == maxAnswers)
				break;
	}

	for (i = 0; i < cnt; i++)
		if (duke(&answers[i])) {
			rmtAddr = answers[i].source;
			break;
		}
	free((char *) answers);
	free((char *) b);
	if (i == cnt)
		return (Sockaddr *) NULL;
	else
		return &rmtAddr;
#else	NO_BROADCAST
	return (Sockaddr *) NULL;
#endif	NO_BROADCAST
}

/* 
 * See if the game contains any empty slots for players.
 */

Boolean
freeSlot(a)
struct answer	*a;
{
	RatStatus	test;
	RatId		id;

	test = (RatStatus) &a->packet.body;
	for (id = 0; id < MAXRATS; id++)
		if (!test->rats[id].playing)
			return TRUE;
	return FALSE;
}

/* 
 * Fill in the RAT_SURVEY packet.
 */

setupSurvey(b)
RatPacket *b;
{
	RatNew	ratSurvey;

	b->type = RAT_SURVEY;
	ratSurvey = (RatNew) &b->body;
	ratSurvey->pass = RAT_PASSWORD;
}

/* 
 * Check if this guy's the duke of his game.
 */

Boolean
duke(a)
struct answer	*a;
{
	RatStatus	test;

	test = (RatStatus) &a->packet.body;
	return !bcmp((char *) &test->rats[test->dukeRat].addr.sin_addr,
			(char *) &a->source.sin_addr, sizeof(struct in_addr));
}

/*
 * Resolve the specified host name into an internet address.  The "name" may
 * be either a character string name, or an address in the form a.b.c.d where
 * the pieces are octal, decimal, or hex numbers.  Returns a pointer to a
 * sockaddr_in struct (note this structure is statically allocated and must
 * be copied), or NULL if the name is unknown.
 */

Sockaddr *
resolveHost(name)
register char *name;
{
	register struct hostent *fhost;
	struct in_addr fadd;
	static Sockaddr sa;

	if ((fhost = gethostbyname(name)) != NULL) {
		sa.sin_family = fhost->h_addrtype;
		sa.sin_port = 0;
		bcopy(fhost->h_addr, &sa.sin_addr, fhost->h_length);
	} else {
		fadd.s_addr = inet_addr(name);
		if (fadd.s_addr != -1) {
			sa.sin_family = AF_INET;	/* grot */
			sa.sin_port = 0;
			sa.sin_addr.s_addr = fadd.s_addr;
		} else
			return(NULL);
	}
	return(&sa);
}
