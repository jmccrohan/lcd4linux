/*
 * driver for display modules from Crystalfontz
 *
 * Copyright 2000 by Herbert Rosmanith (herp@wildsau.idv.uni-linz.ac.at)
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
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<errno.h>
#include	<termios.h>
#include	<unistd.h>
#include	<signal.h>
#include	<fcntl.h>

#include	"cfg.h"
#include	"lock.h"
#include	"display.h"
#include	"Crystalfontz.h"

#define XRES	6
#define YRES	8
#define BARS	( BAR_L | BAR_R )

static LCD Lcd;
static char *Port=NULL;
static speed_t Speed;
static int Device=-1;

static char *Txtbuf,*BackupTxtbuf;	/* text (+backup) buffer */
static char *Barbuf,*BackupBarbuf;	/* bar (+backup) buffer */
static char *CustCharMap;
static int tdim,bdim;			/* text/bar dimension */
static char isTxtDirty;
static char *isBarDirty;
static char isAnyBarDirty;

static void cryfonquit() {

	close(Device);
	unlock_port(Port);
	exit(0);
}

static int cryfonopen() {
int fd;
pid_t pid;
struct termios portset;

	if ((pid=lock_port(Port))!=0) {
		if (pid==-1) fprintf(stderr,"Crystalfontz: port %s could not be locked\n",Port);
		else fprintf(stderr,"Crystalfontz: port %s is locked by process %d\n",Port,pid);
		return -1;
	}
	fd=open(Port,O_RDWR|O_NOCTTY|O_NDELAY);
	if (fd==-1) {
		fprintf(stderr,"Crystalfontz: open(%s) failed: %s\n",
			Port,strerror(errno));
		return -1;
	}
	if (tcgetattr(fd,&portset)==-1) {
		fprintf(stderr,"Crystalfontz: tcgetattr(%s) failed: %s\n",
			Port,strerror(errno));
		return -1;
	}
	cfmakeraw(&portset);
	cfsetospeed(&portset,Speed);
	if (tcsetattr(fd, TCSANOW, &portset)==-1) {
		fprintf(stderr,"Crystalfontz: tcsetattr(%s) failed: %s\n",
			Port,strerror(errno));
		return -1;
	}
	return fd;
}

static int cryfoninit(LCD *Self) {
char *port;
char *speed;
char *backlight;
char *contrast;
char cmd_backlight[2]={ CRYFON_BACKLIGHT_CTRL, };
char cmd_contrast[2]={ CRYFON_CONTRAST_CTRL, };

	Lcd=*Self;

	if (Port) {
		free(Port);
		Port=NULL;
	}

	if ((port=cfg_get("Port"))==NULL || *port=='\0') {
		fprintf(stderr,"CrystalFontz: no 'Port' entry in %s\n",
			cfg_file());
		return -1;
	}
	if (port[0]=='/') Port=strdup(port);
	else {
		Port=(char *)malloc(5/*/dev/ */+strlen(port)+1);
		sprintf(Port,"/dev/%s",port);
	}

	speed=cfg_get("Speed")?:"9600";
	switch(atoi(speed)) {
	case 1200:
		Speed=B1200;
		break;
	case 2400:
		Speed=B2400;
		break;
	case 9600:
		Speed=B9600;
		break;
	case 19200:
		Speed=B19200;
		break;
	default:
		fprintf(stderr,"CrystalFontz: unsupported speed '%s' in '%s'\n",
			speed,cfg_file());
		return -1;
	}

	if ((Device=cryfonopen())==-1)
		return -1;

	tdim=Lcd.rows*Lcd.cols;
	bdim=Lcd.rows*Lcd.cols*2;
			
	Txtbuf=(char *)malloc(tdim);
	if (Txtbuf==NULL) {
		fprintf(stderr,"CrystalFontz: out of memory\n");
		return -1;
	}
	CustCharMap=(char *)malloc(tdim);
	if (CustCharMap==NULL) {
		fprintf(stderr,"CrystalFontz: out of memory\n");
		return -1;
	}
	BackupTxtbuf=(char *)malloc(tdim);
	if (BackupTxtbuf==NULL) {
		fprintf(stderr,"CrystalFontz: out of memory\n");
		return -1;
	}
	Barbuf=(char *)malloc(bdim);
	if (Barbuf==NULL) {
		fprintf(stderr,"CrystalFontz: out of memory\n");
		return -1;
	}
	BackupBarbuf=(char *)malloc(bdim);
	if (BackupBarbuf==NULL) {
		fprintf(stderr,"CrystalFontz: out of memory\n");
		return -1;
	}
	isBarDirty=(char *)malloc(Lcd.rows);
	if (isBarDirty==NULL) {
		fprintf(stderr,"CrystalFontz: out of memory\n");
		return -1;
	}
	memset(Txtbuf,' ',tdim);
	memset(CustCharMap,-1,tdim);
	memset(BackupTxtbuf,255,tdim);
	memset(Barbuf,0,bdim);
	memset(BackupBarbuf,0,bdim);
	memset(isBarDirty,0,Lcd.rows);
	isAnyBarDirty=0;
	isTxtDirty=0;

	signal(SIGINT,cryfonquit);
	signal(SIGQUIT,cryfonquit);
	signal(SIGTERM,cryfonquit);

	usleep(350000);
	write(Device, CRYFON_HIDE_CURSOR CRYFON_SCROLL_OFF CRYFON_WRAP_OFF,3);
	backlight=cfg_get("Backlight")?:NULL;
	if (backlight) {
		cmd_backlight[1]=atoi(backlight);
		write(Device,cmd_backlight,4);
	}
	
	contrast=cfg_get("Contrast")?:NULL;
	if (contrast) {
		cmd_contrast[1]=atoi(contrast);
		write(Device,cmd_contrast,2);
	}
	
	return 0;
}

static int cryfonclear() {
	memset(Txtbuf,' ',tdim);
	memset(Barbuf,0,bdim);
	return 0;
}

static int cryfonput(int row,int col,char *text) {
int pos;

	pos=row*Lcd.cols+col;
	memcpy(Txtbuf+pos,text,strlen(text));
	isTxtDirty|=memcmp(Txtbuf+pos,BackupTxtbuf+pos,strlen(text));
	return 0;
}

static unsigned char p1[] = { 0x3f,0x1f,0x0f,0x07,0x03,0x01,0x00 };
static unsigned char p2[] = { 0x00,0x20,0x30,0x38,0x3c,0x3e,0x3f };

static void blacken(int bitfrom,int len,int pos,int startbyte,int endbyte) {

	if (startbyte==endbyte)
		Barbuf[pos] |=
			(p1[bitfrom%XRES] & p2[1+((bitfrom+len-1)%XRES)]);
	else {
	int n;
		Barbuf[pos] |= p1[bitfrom%XRES];
		n=endbyte-startbyte-1;
		if (n>0) memset(Barbuf+pos+1,0x3f,n);
		Barbuf[pos+n+1] |= p2[1+((bitfrom+len-1)%XRES)];

	}
}

static void whiten(int bitfrom,int len,int pos,int startbyte,int endbyte) {

	if (startbyte==endbyte)
		Barbuf[pos] &=
			(p2[bitfrom%XRES] | p1[1+((bitfrom+len-1)%XRES)]);
	else {
	int n;
		Barbuf[pos] &= p2[bitfrom%XRES];
		n=endbyte-startbyte-1;
		if (n>0) memset(Barbuf+pos+1,0x00,n);
		Barbuf[pos+n+1] &= p1[1+((bitfrom+len-1)%XRES)];
	}
}

static int cryfonbar(int type,int row,int col,int max,int len1,int len2) {
int endb,maxb;
int bitfrom;
int pos;

	if (len1<1) len1=1;
	else if (len1>max) len1=max;

	if (len2<1) len2=1;
	else if (len2>max) len2=max;

	bitfrom=col*XRES;
	endb=(bitfrom+len1-1)/XRES;
	pos=row*Lcd.cols*2;

	switch(type) {
	case BAR_L:
		blacken(bitfrom,len1,pos+col,col,endb);
		endb=(bitfrom+len1)/XRES;
		maxb=(bitfrom+max-1)/XRES;
		whiten(bitfrom+len1,max-len1,pos+endb,endb,maxb);
		if (len1==len2)
			memcpy(Barbuf+pos+col+Lcd.cols,
				Barbuf+pos+col,
				maxb-col+1);
		else {
			pos+=Lcd.cols;
			endb=(bitfrom+len2-1)/XRES;
			blacken(bitfrom,len2,pos+col,col,endb);
			endb=(bitfrom+len2)/XRES;
			whiten(bitfrom+len2,max-len2,pos+endb,endb,maxb);
		}
		break;
	case BAR_R:
		blacken(bitfrom,len1,pos+col,col,endb);
		endb=(bitfrom+len1)/XRES;
		maxb=(bitfrom+max-1)/XRES;
		whiten(bitfrom+len1,max-len1,pos+endb,endb,maxb);
		if (len1==len2)
			memcpy(Barbuf+pos+col+Lcd.cols,
				Barbuf+pos+col,
				maxb-col+1);
		else {
			pos+=Lcd.cols;
			endb=(bitfrom+len2-1)/XRES;
			blacken(bitfrom,len2,pos+col,col,endb);
			endb=(bitfrom+len2)/XRES;
			whiten(bitfrom+len2,max-len2,pos+endb,endb,maxb);
		}
		break;
	}
	isBarDirty[row]=1;
	isAnyBarDirty=1;	/* dont know exactly, check anyway */
	return 0;
}

static int txt_lc=-1,txt_lr=-1;

static void writeTxt(char r,char c,int itxt,int len) {
static char cmd_goto[3]=CRYFON_GOTO;

	if (txt_lr!=r || txt_lc!=c) {
		if (r==0 && c==0) write(Device,CRYFON_HOME,1);
		else {
			cmd_goto[1]=(unsigned char)c;
			cmd_goto[2]=(unsigned char)r;
			write(Device,(char *)&cmd_goto,3);
		}
	}
	txt_lr=r;
	txt_lc=c+len;
	write(Device,Txtbuf+itxt,len);
}

static void writeTxtDiff() {
int spos,scol;
int i,j,k;

	k=0;
	txt_lr=txt_lc=-1;
	for (i=0;i<Lcd.rows;i++) {
		spos=-1;
		scol=0;	/* make gcc happy */
		for (j=0;j<Lcd.cols;j++) {
			if (Txtbuf[k]^BackupTxtbuf[k]) {
				if (spos==-1) {
					spos=k;
					scol=j;
				}
			} else if (spos>-1) {
				writeTxt((char)i,(char)scol,spos,k-spos);
				memcpy(BackupTxtbuf+spos,Txtbuf+spos,k-spos);
				spos=-1;
			}
			k++;
		}
		if (spos>-1) {
			writeTxt((char)i,(char)scol,spos,k-spos);
			memcpy(BackupTxtbuf+spos,Txtbuf+spos,k-spos);
		}
	}
}

/* private bar flushing routines */

static char BarCharBuf[256];
static int bi=0;

static void flushBarCharBuf() {
	if (bi) {
		write(Device,BarCharBuf,bi);	/* flush buffer */
		tcdrain(Device);
		bi=0;
	}
}

static void writeBarCharBuf(char *s,int len) {
	if (bi+len>=sizeof(BarCharBuf)) {
		flushBarCharBuf();
	}
	memcpy(BarCharBuf+bi,s,len);
	bi+=len;
}

static struct {
	char use_count;	/* 0 - unused */
	char data[8];
} cust_chars[8] = {
 { 0, }, { 0, }, { 0, }, { 0, }, { 0, }, { 0, }, { 0, }, { 0, }
};

static int search_cust_char(char c1,char c2) {
int i;
	for (i=0;i<8;i++) {
		if (cust_chars[i].data[0]==c1
			&& cust_chars[i].data[4]==c2) {
			return i;
		}
	}
	return -1;	/* not found */
}

static void set_cust_char(int ci,char c1,char c2) {
static char cmd_cust[10] = CRYFON_SET_CUSTOM_CHAR_BITMAP;

	memset(cust_chars[ci].data,c1,4);
	memset(cust_chars[ci].data+4,c2,4);
	cmd_cust[1]=ci;
	memset(cmd_cust+2,c1,4);
	memset(cmd_cust+6,c2,4);
	writeBarCharBuf(cmd_cust,10);
}

static int alloc_cust_char(char c1,char c2) {
static char allzero[8]={0, };
int i;

	/* first, try to allocate a never used entry */

	for(i=0;i<8;i++)
		if (memcmp(cust_chars[i].data,allzero,8)==0) {
			set_cust_char(i,c1,c2);
			return i;
		}

	/* if that fails, pick an entry with is not yet in use */

	for(i=0;i<8;i++)
		if (cust_chars[i].use_count==0) {
			set_cust_char(i,c1,c2);
			return i;
		}
	return -1;	/* no free char */
}

static void use_cust_char(int i,int j,int ci) {
int x;
	x=i*Lcd.cols+j;
	if (CustCharMap[x]==-1) {
		cust_chars[ci].use_count++;
		CustCharMap[x]=ci;
	}
	/* else: internal consistency failure */
	else {
		printf("Crystalfontz: internal consistency failure 1\n");
		exit(0);
	}
}

static void unuse_cust_char(int i,int j) {
int ci,x;
	x=i*Lcd.cols+j;
	ci=CustCharMap[x];
	if (ci>-1) {
		CustCharMap[x]=-1;
		cust_chars[ci].use_count--;
		if (cust_chars[i].use_count==-1) {
			printf("Crystalfontz: internal consistency failure 2\n");
			exit(0);
		}
	}
}

static int bar_lc=-1,bar_lr=-1;


static void writeChar(unsigned char r,unsigned char c,unsigned char ch) {
static char cmd_goto[3]=CRYFON_GOTO;

	if (bar_lr!=r || bar_lc!=c) {
		if (r==0 && c==0) writeBarCharBuf(CRYFON_HOME,1);
		else {
			cmd_goto[1]=(unsigned char)c;
			cmd_goto[2]=(unsigned char)r;
			writeBarCharBuf((char *)&cmd_goto,3);
		}
	}
	bar_lr=r;
	bar_lc=c++;
	writeBarCharBuf(&ch,1);
}

static void writeBarDiff() {
char c1,c2;
int i,j,k1,k2,ci;

	for (i=0;i<Lcd.rows;i++) {
		if (isBarDirty[i]) {
			k1=i*Lcd.cols*2;
			k2=k1+Lcd.cols;
			bar_lr=bar_lc=-1;
			for (j=0;j<Lcd.cols;j++) {
				c1=Barbuf[k1];
				c2=Barbuf[k2];
				if (c1^BackupBarbuf[k1] ||
				    c2^BackupBarbuf[k2]) {

					if (c1==0 && c2==0) { /* blank */
						unuse_cust_char(i,j);
						writeChar(i,j,' ');
					}
					else if (c1==0x1f && c2==0x1f) { /*boxlike*/
						unuse_cust_char(i,j);
						writeChar(i,j,0xff);
					}
					else {
						/* search cust char */
						ci=search_cust_char(c1,c2);
						if (ci>-1) { /* found: reuse that char */
							unuse_cust_char(i,j);
							writeChar(i,j,128+ci);
							use_cust_char(i,j,ci);
						}
						else {	/* not found: get a new one */
							ci=alloc_cust_char(c1,c2);
							if (ci>-1) {
								unuse_cust_char(i,j);
								writeChar(i,j,128+ci);
								use_cust_char(i,j,ci);
							}
							else printf("failed to alloc a custom char\n");
						}
					}
					BackupBarbuf[k1]=c1;
					BackupBarbuf[k2]=c2;
				}
				k1++;
				k2++;
			}
		}
		isBarDirty[i]=0;
	}
	flushBarCharBuf();
}

static int cryfonflush() {

	if (isTxtDirty) {
		writeTxtDiff();
		isTxtDirty=0;
	}
	if (isAnyBarDirty) {
		writeBarDiff();
		isAnyBarDirty=0;
	}

	return 0;
}

LCD Crystalfontz[] = {
	{ "626", 2, 16, XRES, YRES, BARS, cryfoninit, cryfonclear, cryfonput, cryfonbar, cryfonflush },
	{ "636", 2, 16, XRES, YRES, BARS, cryfoninit, cryfonclear, cryfonput, cryfonbar, cryfonflush },
	{ "632", 2, 16, XRES, YRES, BARS, cryfoninit, cryfonclear, cryfonput, cryfonbar, cryfonflush },
	{ "634", 4, 20, XRES, YRES, BARS, cryfoninit, cryfonclear, cryfonput, cryfonbar, cryfonflush },
	{ NULL }
};
