#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Board size and number of mines */

#define MINES		4

#define EXPOSED	0x10
#define FLAG		0x20
#define MINE		0x40

unsigned char	m[8][8];
int 	exposed=0;

#define BOARDLOOP	for (r=0;r<8;r++) \
							for (c=0;c<8;c++)
int(*f)();

traverse(r,c)
{
    int rd, cd;
    for (rd=-1;rd<2;rd++)
	for (cd = -1; cd<2; cd++) {
	    int nr = r+rd, nc = c+cd;
	    if ((cd|rd) && !((nr|nc)&0xF8) && !(m[nr][nc]&EXPOSED))
	        (*f)(r,c,nr,nc);
	}
}

check_neighbour(r,c,nr,nc)
{
	if (m[nr][nc] & MINE) m[r][c]++;
}

init_board(r,c,n)
{
	/* Clear all squares */
	BOARDLOOP { m[r][c] = 0; }
	/* Shove the mines in randomly */
	for (n=MINES;n;) {
		r = rand()%8;
		c = rand()%8;
		if (m[r][c]!=MINE) /* random square not mined yet */
		{
			/* lay a mine */
			n--;
			m[r][c] = MINE;
		}
	}
	/* Now compute the number of neighbouring mines for all
		non-mined squares */
	BOARDLOOP
	{
		if (m[r][c] != MINE)
			traverse(r,c);
	}
}

/****************************/
/* Expose a selected square */
/****************************/

recursive_expose(r,c,nr,nc)
{
	if (m[nr][nc]<0x10)
	{
		/* Not a mine, flag or exposed */
		exposed++;
		m[nr][nc]|=EXPOSED; /* Set expose flag */
	}
	if (m[nr][nc]==EXPOSED) /* ie, exposed value 0 */
		traverse(nr,nc);
}

expose(r,c)
{
	recursive_expose(0,0,r,c);
	return (m[r][c]&MINE);
}

/**************************/
/* Draw the board */
/**************************/

print_board()
{
    int r, c;
    printf("  12345678\n");
    for (r = 0; r < 8; r++)
    {
	printf("\n%d ", r+1);
        for (c = 0; c < 8; c++)
	{
	    if (m[r][c]&FLAG) putchar('F');
	    else if (m[r][c]&EXPOSED)
	    {
	        if (m[r][c]&MINE) putchar('*');
		else putchar('0'+(m[r][c]&0xF));
	    }
	    else putchar('#');
	}
    }
    putchar('\n');
}

/****************/
/* Main Routine */
/****************/

main()
{
	int r, c;
	f=check_neighbour;
	init_board();
	f=recursive_expose;

	/* Main loop */

	do
	{
	    print_board();
	    scanf("%d%d",&r,&c);
	}
	while (
	    (r<0 ? (m[-r-1][c-1] ^= FLAG) :
		(expose(r-1,c-1)==0)) && exposed<60);
	print_board();
        puts(exposed<60?"BOOM!":"CLEAR");
}

