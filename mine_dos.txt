/*
 * mine.c (c) 1994 by Graham Wheeler, All Rights Reserved
 *
 * This is a simple implementation of the MS-Windows game
 * WinMine for DOS. It requires a Microsoft-compatible
 * mouse. You need Borland C to compile it.
 *
 * I have seen a few other programs to implement this game,
 * and they have been really ugly. This implementation is
 * considerably more elegant and a lot shorter, mostly due
 * to exploiting the recursive nature of the game.
 *
 * This program is really meant as an example for students
 * of C programming. As such, it may be freely distributed
 * and used, provided this message is preserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <time.h>
#include <dos.h>

/* Characters and attributes for display */

#define MINECHAR	'*'
#define FLAGCHAR	0xE7	/* non-ASCII PC flag character */

#define FLAGATTR	0x74	/* red on white */
#define BOARDATTR	0x71	/* blue on white */
#define MINEATTR	0x47	/* white on red	*/

/* Uncomment the next line to remove delays */

/*#define delay(d)*/

/* Registers needed for DOS calls */

union REGS inregs, outregs;

/* Board size and number of mines */

#define HEIGHT	10
#define WIDTH		20
#define MINES		20

/* Board square flags. */

#define EXPOSED	0x10
#define FLAG		0x20
#define MINE		0x40

/*
 * The board. The low nybble holds the number of neighbouring
 * 	mines, while the high nybbles hold the flags
 */

unsigned char	minefield[HEIGHT][WIDTH];

/* Count of how many exposed squares there are */

int 	exposed=0;

/* Useful macro for common type of loop */

#define BOARDLOOP	for (r=0;r<HEIGHT;r++) \
							for (c=0;c<WIDTH;c++)

/* Useful type for passing functions as parameters */

typedef void (*trav_fn)(int,int,int,int);

/*********************/
/* MS-Mouse routines */
/*********************/

void mouse_status(int *mstat, int *nbuttons)
{
  inregs.x.ax = 0;
  int86(51, &inregs, &outregs); /* Reset mouse */
  if (mstat)    *mstat    = outregs.x.ax; /* Mouse present? */
  if (nbuttons) *nbuttons = outregs.x.bx; /* Number of buttons */
}

/* Set mouse cursor shape */

void mouse_shape(int scn, int cur)
{
  inregs.x.ax = 10;
  inregs.x.bx = 0;
  inregs.x.cx = scn;
  inregs.x.dx = cur;
  int86(51, &inregs, &outregs);
}

/* show mouse cursor */

void mouse_show(void)
{
  inregs.x.ax = 1;
  int86(51, &inregs, &outregs);
}

/* hide mouse cursor */

void mouse_hide(void)
{
  inregs.x.ax = 2;
  int86(51, &inregs, &outregs);
}

/* return position and buttons */

void mouse_read(int *mbt, int *mx, int *my)
{
  inregs.x.ax = 3;
  int86(51, &inregs, &outregs);
  if (mbt) *mbt = outregs.x.bx;
  if (mx)  *mx  = outregs.x.cx;
  if (my)  *my  = outregs.x.dx;
}

/****************/
/* Basic output */
/****************/

/* make some noise! */

void beep(int freq, int duration)
{
	long count = 1193280l/(long)freq;
	outportb(67,182);
	outportb(66,(char)(count%256));
	outportb(66,(char)(count/256));
	outportb(97,inportb(97)|3);
	delay(duration);
	outportb(97,inportb(97)&0xFC);
}

/* Put character and attribute at screen cursor position */

void put_screen_ch(int c, int a)
{
	inregs.h.al = (char)c;
	inregs.h.bl = (char)a;
	inregs.x.cx = 1;
	inregs.h.ah=9;
	int86(16, &inregs, &outregs);
}

/*******************/
/* Board traversal */
/*******************/

/* Traverse all non-exposed neighbours of a square applying a function */

void traverse(int r, int c, trav_fn f)
{
	int rd, cd;
	for (rd=-1;rd<2;rd++)
		for (cd = -1; cd<2; cd++) {
			int nr = r+rd, nc = c+cd;
			/* If this is the center square or we are off the edge of the
				board or we are exposed, skip this case */
			if (nr==r && nc==c) continue; /* center */
			if (nr<0 || nr>=HEIGHT) continue; /* off top/bottom */
			if (nc<0 || nc>=WIDTH) continue; /* off left/right */
			if (minefield[nr][nc]&EXPOSED) continue;
			(*f)(r,c,nr,nc);
		}
}

/************************/
/* Board initialisation */
/************************/

void check_neighbour(int r, int c, int nr, int nc)
{
	if (minefield[nr][nc] & MINE)
		minefield[r][c]++;
}

void init_board(void)
{
	int r, c, n;
	/* Clear all squares */
	BOARDLOOP { minefield[r][c] = 0; }
	/* Shove the mines in randomly */
	randomize();
	for (n=MINES;n;) {
		r = random(HEIGHT);
		c = random(WIDTH);
		if (minefield[r][c]!=MINE) /* random square not mined yet */
		{
			/* lay a mine */
			n--;
			minefield[r][c] = MINE;
		}
	}
	/* Now compute the number of neighbouring mines for all
		non-mined squares */
	BOARDLOOP
	{
		if (minefield[r][c] != MINE)
			traverse(r,c,check_neighbour);
	}
}

/*****************************************/
/* Show the number of neighbouring mines */
/*****************************************/

void show_num(int r, int c)
{
	gotoxy(11+2*c, 2+2*r);
	if (minefield[r][c]<0x10)
	{
		/* Not a mine, flag or exposed */
		put_screen_ch('0'+(minefield[r][c]&0xF), BOARDATTR);
		exposed++;
		beep(3000,50);
		minefield[r][c]|=EXPOSED; /* Set expose flag */
	}
	else if (minefield[r][c]&FLAG)
		put_screen_ch(FLAGCHAR,FLAGATTR);
	else if (minefield[r][c]&MINE)
		put_screen_ch(MINECHAR,MINEATTR);
}

/****************************/
/* Expose a selected square */
/****************************/

void recursive_expose(int r, int c, int nr, int nc)
{
	/* In recursive expose, r & c are unused. The next line prevents the
		compiler warning */
	(void)r; (void)c;
	show_num(nr,nc);
	if (minefield[nr][nc]==EXPOSED) /* ie, exposed value 0 */
		traverse(nr,nc,recursive_expose);
}

int expose(int r, int c)
{
	recursive_expose(0,0,r,c); /* The 0's are dummy params needed by traverse */
	if (minefield[r][c]&MINE) return 1; /* blown up */
	else return 0;
}

/**************************/
/* Draw the initial board */
/**************************/

/* it's easier to understand these when looking at the board
	display on the screen! */

#define putboard(r,c,ch) gotoxy(c,r); put_screen_ch(ch,BOARDATTR)

void putline(int r,int left, int right, int mid, int join)
{
	int c;
	putboard(r,10,left);
	for (c=1;c<WIDTH;c++)
	{
		putboard(r,9+2*c,mid); putboard(r,10+2*c,join);
	}
	putboard(r,9+2*c,mid); putboard(r,10+2*c,right);
}

void print_board(void)
{
	int r;
	putline(1,201,187,205,209);
	for (r=0;r<HEIGHT;r++)
	{
		putline(2+r*2,186,186,219,179);
		if (r==(HEIGHT-1)) break;
		putline(3+r*2,199,182,196,197);
	}
	putline(3+r*2,200,188,205,207);
}

/****************/
/* Main Routine */
/****************/

void main(void)
{
	int r, c, f=0;
	init_board();
	mouse_status(NULL,NULL);
	mouse_shape(0,0xC701); /* Set cursor to a destructive red smiley */
	clrscr();
	gotoxy(60,10); puts("Quit");
	print_board();
	mouse_show();

	/* Main loop */

	for (;;)
	{
		int b=0, x, y;
		/* Read mouse until button is pressed... */

		while (b==0)
			mouse_read(&b,&x,&y);

		/* Figure out the mouse location */

		x = x/8+1; /* Pixel to column */
		y = y/8+1;

		/* Did user click on Quit? */

		if (x>59 && x<64 && y==10) break;

		/* Dis user click on board? */

		if (x>10 && x<(12+2*WIDTH) && (x%2))
		{
			if (y>1 && y<(3+2*HEIGHT) && (y%2==0))
			{
				x = (x-11)/2;
				y = (y-2)/2;
				mouse_hide();
				gotoxy(11+2*x, 2+2*y); /* put cursor where mouse clicked */

				/* If a flagged square, unflag it... */

				if (minefield[y][x]&FLAG)
				{
					minefield[y][x] ^= FLAG;
					put_screen_ch(219,BOARDATTR);
					gotoxy(60,12);
					printf("Flags: %d  ",--f);
					fflush(stdout);
				}

				/* else if middle mouse button, set a flag... */

				else if (b==2)
				{
					minefield[y][x] |= FLAG;
					put_screen_ch(FLAGCHAR,FLAGATTR);
					gotoxy(60,12);
					printf("Flags: %d  ",++f);
					fflush(stdout);
				}

				/* else expose the square and see what happens... */

				else if (expose(y,x))
				{
					int i;
					gotoxy(60,16);
					puts("B O O O O M !!!");
					for (i=0;i<200;i++)
						beep(random(800),5);
					break;
				} else
				{
					gotoxy(60,14);
					printf("Exposed: %d  ",exposed);
					fflush(stdout);
				}

				/* Have all the non-mines been exposed? */

				if ((exposed+MINES) == (HEIGHT*WIDTH))
				{
					gotoxy(60,16);
					puts("You Win !!!");
					break;
				}

				delay(200);
				mouse_show();
			}
		}
	}
	mouse_hide();
	gotoxy(0,22);
}

