/* $Id: XWindow.c,v 1.22 2001/02/13 09:00:13 reinelt Exp $
 *
 * X11 Driver for LCD4Linux 
 *
 * (c) 2000 Herbert Rosmanith <herp@widsau.idv.uni-linz.ac.at>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * $Log: XWindow.c,v $
 * Revision 1.22  2001/02/13 09:00:13  reinelt
 *
 * prepared framework for GPO's (general purpose outputs)
 *
 * Revision 1.21  2000/08/10 18:42:20  reinelt
 *
 * fixed some bugs with the new syslog code
 *
 * Revision 1.20  2000/08/10 09:44:09  reinelt
 *
 * new debugging scheme: error(), info(), debug()
 * uses syslog if in daemon mode
 *
 * Revision 1.19  2000/08/09 09:50:29  reinelt
 *
 * opened 0.98 development
 * removed driver-specific signal-handlers
 * added 'quit'-function to driver structure
 * added global signal-handler
 *
 * Revision 1.18  2000/05/02 23:07:48  herp
 * Crystalfontz initial coding
 *
 * Revision 1.17  2000/04/05 05:58:36  reinelt
 *
 * fixed bug in XWindow.c: union semun isn't defined with glibc-2.1
 *
 * Revision 1.16  2000/04/03 23:53:23  herp
 * fixed a bug that caused pixel-errors ("fliegendreck") under high load
 *
 * Revision 1.15  2000/04/03 04:01:31  reinelt
 *
 * if 'gap' is specified as -1, a gap of (pixelsize+pixelgap) is selected automatically
 *
 * Revision 1.14  2000/04/02 22:07:10  herp
 * fixded a bug that occasionally caused Xlib errors
 *
 * Revision 1.13  2000/04/01 22:40:42  herp
 * geometric correction (too many pixelgaps)
 * lcd4linux main should return int, not void
 *
 * Revision 1.12  2000/04/01 19:33:45  herp
 *
 * colors in format \#RRGGBB in config-file now understood
 *
 * Revision 1.11  2000/04/01 16:22:38  reinelt
 *
 * bug that caused a segfault in processor.c fixed (thanks to herp)
 *
 * Revision 1.10  2000/03/31 01:42:11  herp
 *
 * semaphore bug fixed
 *
 * Revision 1.9  2000/03/30 16:46:57  reinelt
 *
 * configure now handles '--with-x' and '--without-x' correct
 *
 * Revision 1.8  2000/03/28 08:48:33  reinelt
 *
 * README.X11 added
 *
 * Revision 1.7  2000/03/28 07:22:15  reinelt
 *
 * version 0.95 released
 * X11 driver up and running
 * minor bugs fixed
 *
 */

/* 
 *
 * exported fuctions:
 *
 * struct LCD XWindow[]
 *
 */


/*
 * Tue Apr  4 02:37:38 MET 2000 fixed a bug that caused pixelerrors under h/load
 * Sun Apr  2 22:07:10 MET 2000 fixed a bug that occasionally caused Xlib error
 * Sun Apr  2 01:32:48 MET 2000 geometric correction (too many pixelgaps)
 * Sat Apr  1 22:18:04 MET 2000 colors in format \#RRGGBB in config-file
 * Fri Mar 31 01:42:11 MET 2000 semaphore bug fixed
 * Sun Mar 26 15:28:23 MET 2000 various rewrites
 * Sat Mar 25 23:58:19 MET 2000 use generic pixmap driver
 * Thu Mar 23 01:05:07 MET 2000 multithreading, synchronization
 * Tue Mar 21 22:22:03 MET 2000 initial coding
 *
 */

#ifndef X_DISPLAY_MISSING

#include	<X11/Xlib.h>
#include	<X11/Xutil.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<errno.h>
#include	<sys/types.h>
#include	<sys/ipc.h>
#include	<sys/sem.h>
#include	<sys/shm.h>
#include	<unistd.h>
#include        <signal.h>

#include        "debug.h"
#include	"cfg.h"
#include	"display.h"
#include	"pixmap.h"

/* glibc 2.1 requires defining semun ourselves */
#ifdef _SEM_SEMUN_UNDEFINED
union semun {
  int val;
  struct semid_ds *buf;
  unsigned short int *array;
  struct seminfo *__buf;
};
#endif

#define BARS ( BAR_L | BAR_R | BAR_U | BAR_D | BAR_H2 | BAR_V2 )

static LCD Lcd;
static Display *dp;
static int sc;
static Window w,rw;
static Visual *vi;
static int dd;
static Colormap cm;
static GC gc,gcb,gch;
static XColor co[3];
static Pixmap pmback;

static unsigned char *BackupLCDpixmap;
static char *rgbfg,*rgbbg,*rgbhg;
static int pixel=-1;			/*pointsize in pixel*/
static int pgap=0;			/*gap between points */
static int rgap=0;			/*row gap between lines*/
static int cgap=0;			/*column gap between characters*/
static int border=0;			/*window border*/
static int rows=-1,cols=-1;		/*rows+cols without background*/
static int xres=-1,yres=-1;		/*xres+yres (same as self->...)*/
static int dimx,dimy;			/*total window dimension in pixel*/
static int boxw,boxh;			/*box width, box height*/
static int async_update();		/*PROTO*/
static pid_t async_updater_pid=1;
static int semid=-1;
static int shmid=-1;
static int ppid;			/*parent pid*/

static void acquire_lock() {
struct sembuf sembuf;
	sembuf.sem_num=0;
	sembuf.sem_op=-1;
	sembuf.sem_flg=0;
	semop(semid,&sembuf,1);		/* get mutex */
}

static void release_lock() {
struct sembuf sembuf;
	sembuf.sem_num=0;
	sembuf.sem_op=1;
	sembuf.sem_flg=0;
	semop(semid,&sembuf,1);		/* free mutex */
}

static void semcleanup() {
union semun arg;
	if (semid>-1) semctl(semid,0,IPC_RMID,arg);
}

static void shmcleanup() {
	if (shmid>-1) shmctl(shmid,IPC_RMID,NULL);
}

static void quit_updater() {
	if (async_updater_pid>1)
		kill(async_updater_pid,15);
}

static int init_shm(int nbytes,unsigned char **buf) {

	shmid=shmget(IPC_PRIVATE,nbytes,SHM_R|SHM_W);
	if (shmid==-1) {
		error ("X11: shmget() failed: %s", strerror(errno));
		return -1;
	}
	*buf=shmat(shmid,NULL,0);
	if (*buf==NULL) {
		error ("X11: shmat() failed: %s", strerror(errno));
		return -1;
	}
	return 0;
}

static int init_thread(int bufsiz) {
union semun semun;

/* acording to SUN-Solaris man-pages: */

#define SEM_ALTER       0200

	semid=semget(IPC_PRIVATE,1,SEM_ALTER);
	if (semid==-1) {
		error ("X11: semget() failed: %s", strerror(errno));
		return -1;
	}
	semun.val=1;
	semctl(semid,0,SETVAL,semun);

	ppid=getpid();
	switch(async_updater_pid=fork()) {
	case -1:
		error ("X11: fork() failed: %s", strerror(errno));
		return -1;
	case 0:
		async_update();
		error ("X11: async_update failed");
		kill(ppid,SIGTERM);
		exit(-1);
	default:
		break;
	}
	atexit(quit_updater);
	return 0;
}

static int init_x(int rows,int cols,int xres,int yres) {
XSetWindowAttributes wa;
XSizeHints size_hints;
XColor co_dummy;
XEvent ev;

	if ((dp=XOpenDisplay(NULL))==NULL) {
		error ("X11: can't open display");
		return -1;
	}
	sc=DefaultScreen(dp);
	gc=DefaultGC(dp,sc);
	vi=DefaultVisual(dp,sc);
	dd=DefaultDepth(dp,sc);
	rw=DefaultRootWindow(dp);
	cm=DefaultColormap(dp,sc);

	if (XAllocNamedColor(dp,cm,rgbfg,&co[0],&co_dummy)==False) {
		error ("X11: can't alloc foreground color '%s'",
			rgbfg);
		return -1;
	}
	if (XAllocNamedColor(dp,cm,rgbbg,&co[1],&co_dummy)==False) {
		error ("X11: can't alloc background color '%s'",
			rgbbg);
		return -1;
	}
	if (XAllocNamedColor(dp,cm,rgbhg,&co[2],&co_dummy)==False) {
		error ("X11: can't alloc halfground color '%s'",
			rgbhg);
		return -1;
	}
	boxw=xres*pixel+(xres-1)*pgap+cgap;
	boxh=yres*pixel+(yres-1)*pgap+rgap;
	dimx=cols*xres*pixel+cols*(xres-1)*pgap+(cols-1)*cgap;
	dimy=rows*yres*pixel+rows*(yres-1)*pgap+(rows-1)*rgap;
	wa.event_mask=ExposureMask;
	w=XCreateWindow(dp,rw,0,0,dimx+2*border,dimy+2*border,0,0,
		InputOutput,vi,CWEventMask,&wa);
	pmback=XCreatePixmap(dp,w,dimx,dimy,dd);
	size_hints.min_width=size_hints.max_width=dimx+2*border;
	size_hints.min_height=size_hints.max_height=dimy+2*border;
	size_hints.flags=PPosition|PSize|PMinSize|PMaxSize;
	XSetWMProperties(dp,w,NULL,NULL,NULL,0,&size_hints,NULL,NULL);
	XSetForeground(dp,gc,co[0].pixel);
	XSetBackground(dp,gc,co[1].pixel);
	gcb=XCreateGC(dp,w,0,NULL);
	XSetForeground(dp,gcb,co[1].pixel);
	XSetBackground(dp,gcb,co[0].pixel);
	gch=XCreateGC(dp,w,0,NULL);
	XSetForeground(dp,gch,co[2].pixel);
	XSetBackground(dp,gch,co[0].pixel);
	XFillRectangle(dp,pmback,gcb,0,0,dimx,dimy);
	XSetWindowBackground(dp,w,co[1].pixel);
	XClearWindow(dp,w);
	XStoreName(dp,w,"XLCD4Linux");
	XMapWindow(dp,w);
	XFlush(dp);
	for(;;) {
		XNextEvent(dp,&ev);
		if (ev.type==Expose && ev.xexpose.count==0)
			break;
	}
	return 0;
}

int xlcdinit(LCD *Self) {
char *s;

	if (sscanf(s=cfg_get("size")?:"20x4","%dx%d",&cols,&rows)!=2
		|| rows<1 || cols<1) {
		error ("X11: bad size '%s'",s);
		return -1;
	}
	if (sscanf(s=cfg_get("font")?:"5x8","%dx%d",&xres,&yres)!=2
		|| xres<5 || yres>10) {
    		error ("X11: bad font '%s'",s);
    		return -1;
	}
	if (sscanf(s=cfg_get("pixel")?:"4+1","%d+%d",&pixel,&pgap)!=2
		|| pixel<1 || pgap<0) {
		error ("X11: bad pixel '%s'",s);
		return -1;
	}
	if (sscanf(s=cfg_get("gap")?:"3x3","%dx%d",&cgap,&rgap)!=2
		|| cgap<-1 || rgap<-1) {
		error ("X11: bad gap '%s'",s);
		return -1;
	}
	if (rgap<0) rgap=pixel+pgap;
	if (cgap<0) cgap=pixel+pgap;
	border=atoi(cfg_get("border")?:"0");
	rgbfg=cfg_get("foreground")?:"#000000";
        rgbbg=cfg_get("background")?:"#80d000";
        rgbhg=cfg_get("halfground")?:"#70c000";
	if (*rgbfg=='\\') rgbfg++;
	if (*rgbbg=='\\') rgbbg++;
	if (*rgbhg=='\\') rgbhg++;

	if (pix_init(rows,cols,xres,yres)==-1) return -1;
	if (init_x(rows,cols,xres,yres)==-1) return -1;
	if (init_shm(rows*cols*xres*yres,&BackupLCDpixmap)==-1) return -1;
	memset(BackupLCDpixmap,0xff,rows*yres*cols*xres);
	if (init_thread(rows*cols*xres*yres)==-1) return -1;
	Self->rows=rows;
	Self->cols=cols;
	Self->xres=xres;
	Self->yres=yres;
	Lcd=*Self;

	pix_clear();
	return 0;
}

int xlcdclear() {
	return pix_clear();
}

int xlcdput(int row,int col,char *text) {
	return pix_put(row,col,text);
}

int xlcdbar(int type, int row, int col, int max, int len1, int len2) {
	return pix_bar(type,row,col,max,len1,len2);
}

int xlcdflush() {
int dirty;
int i,j,igap,jgap,pos;
int x,y;

	acquire_lock();
	dirty=pos=igap=jgap=0;
	y=border;
	for(i=0;i<rows*yres;i++) {
		x=border;
		for(j=0;j<cols*xres;j++) {
			if (LCDpixmap[pos]^BackupLCDpixmap[pos]) {
				XFillRectangle(dp,w,
					LCDpixmap[pos]?gc:gch,
					x,y,
					pixel,pixel);
				BackupLCDpixmap[pos]=LCDpixmap[pos];
				dirty=1;
			}
			x+=pixel+pgap;
			if (++jgap==xres) { x+=cgap-pgap; jgap=0; }
			pos++;
		}
		y+=pixel+pgap;
		if (++igap==yres) { y+=rgap-pgap; igap=0; }
	}
	if (dirty) XSync(dp,False);
	release_lock();
	return 0;
}

int xlcdquit(void) {
	semcleanup();
	shmcleanup();
	if (ppid!=getpid())
	  // FIXME: kill(ppid,nsig);
	  kill(ppid,SIGTERM);
	return 0;
}

/*
 * this one should only be called from the updater-thread
 * no user serviceable parts inside
 */

static void update(int x,int y,int width,int height) {
int i,j,igap,jgap,wjgap,pos,wpos,xpix;
int xfrom,yfrom;
int xto,yto;
int dx,wx,wy;

	/*
	 * theory of operation:
	 * f:bitpos->pxnr*(pixel+pgap)+(pxnr/xres)*cgap;
	 * f^-1: pxnr -> f^-1(bitpos) = see paper
	 */
	x-=border;
	y-=border;
	if (x>=dimx || y>=dimy || x+width<0 || y+height<0)
		return;	/*border doesnt need update*/
	if (x<0) x=0;
	if (y<0) y=0;
	if (x+width>dimx) width=dimx-x;
	if (y+height>dimy) height=dimy-y;
	/*
	 * now we have to find out where the box starts
	 * with respects to the LCDpixmap coordinates
	 */
	/* upper left corner */
	xfrom=xres*(x/boxw);			/*start at col.no*/
	i=(x%boxw);				/*pixelpos rel. char*/
	if (i>xres*pixel+(xres-1)*pgap)		/*in cgap zone*/
		xfrom+=xres;
	else {
		xfrom+=i/(pixel+pgap);		/*character element*/
		if (i%(pixel+pgap)>=pixel)	/*in pgap zone*/
			xfrom++;
	}
	yfrom=yres*(y/boxh);			/*start at row.no*/
	i=(y%boxh);				/*pixelpos rel. char*/
	if (i>yres*pixel+(yres-1)*pgap)		/*in rgap zone*/
		yfrom+=yres;
	else {
		yfrom+=i/(pixel+pgap);		/*character element*/
		if (i%(pixel+pgap)>=pixel)	/*in pgag zone*/
			yfrom++;
	}
	/*lower right corner*/
	x+=width-1;
	y+=height-1;
	xto=xres*(x/boxw)+(x%boxw)/(pixel+pgap);
	yto=yres*(y/boxh)+(y%boxh)/(pixel+pgap);

	pos=yfrom*xres*cols+xfrom;
	wy=border+yfrom*(pixel+pgap)+(yfrom/yres)*(rgap-pgap);
	wx=border+xfrom*(pixel+pgap)+(xfrom/xres)*(cgap-pgap);
	wpos=pos; xpix=xres*cols;
	igap=yfrom%yres; wjgap=xfrom%xres;
	for(i=yfrom;i<=yto;i++) {
		dx=wx;
		jgap=wjgap;
		for(j=xfrom;j<=xto;j++) {
			XFillRectangle(dp,w,
				BackupLCDpixmap[wpos++]?gc:gch,
				dx,wy,
				pixel,pixel);
			dx+=pixel+pgap;
			if (++jgap==xres) { dx+=cgap-pgap; jgap=0; }
		}
		wy+=pixel+pgap;
		if (++igap==yres) { wy+=rgap-pgap; igap=0; }
		pos+=xpix;
		wpos=pos;
	}
	XSync(dp,False);
}

static int async_update() {
XSetWindowAttributes wa;
XEvent ev;

	if ((dp=XOpenDisplay(NULL))==NULL)
		return -1;
	wa.event_mask=ExposureMask;
	XChangeWindowAttributes(dp,w,CWEventMask,&wa);
	for(;;) {
		XWindowEvent(dp,w,ExposureMask,&ev);
		if (ev.type==Expose) {
			acquire_lock();
			update(ev.xexpose.x,ev.xexpose.y,
				ev.xexpose.width,ev.xexpose.height);
			release_lock();
		}
	}
}

LCD XWindow[] = {
	{ "X11",0,0,0,0,BARS,0,xlcdinit,xlcdclear,xlcdput,xlcdbar,NULL,xlcdflush,xlcdquit },
	{ NULL }
};

#endif /* X_DISPLAY_MISSING */
