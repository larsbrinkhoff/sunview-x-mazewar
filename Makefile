CFLAGS = -O -DBROAD_ADDR=INADDR_BROADCAST -DBSD_43
OBJS = mazewar.o display.o init.o
SRCS = mazewar.c display.c init.c

#all: 		sun_mazewar X10_mazewar X11_mazewar mazefind 
all: 		X11_mazewar mazefind 

sun_mazewar: 	${OBJS} winSunView.o
		${CC} ${CFLAGS} ${OBJS} winSunView.o -o sun_mazewar \
			-lsuntool -lsunwindow -lpixrect

X10_mazewar:	${OBJS} winX10.o
		${CC} ${CFLAGS} ${OBJS} winX10.o -o X10_mazewar -lX
	
X11_mazewar:	${OBJS} winX11.o
		${CC} ${CFLAGS} ${OBJS} winX11.o -o X11_mazewar -lXt -lX11

saber_o:
		#load ${CFLAGS} ${OBJS} winX11.o -lXt -lX11

saber:		
		#load ${CFLAGS} mazewar.c display.c init.c winX11.c -lXt -lX11
	
mazefind: 	mazefind.o
		${CC} ${CFLAGS} mazefind.o -o mazefind

lint:
		lint -hcx $(SRCS) winX11.c

clean:
		rm -f *.o sun_mazewar X10_mazewar X11_mazewar mazefind

mazewar.o: mazewar.h
display.o: mazewar.h
init.o: mazewar.h
winX10.o: mazewar.h
winX11.o: mazewar.h
winSunView.o: mazewar.h
mazefind.o: mazewar.h
