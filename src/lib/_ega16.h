/* Funktionsprototypen
   _EGA16.ASM
   Marius Greuel '94
*/

#if	!defined(__COLORS)
#define __COLORS
enum COLORS {
    BLACK,		    /* dark colors */
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    BROWN,
    LIGHTGRAY,
    DARKGRAY,		    /* light colors */
    LIGHTBLUE,
    LIGHTGREEN,
    LIGHTCYAN,
    LIGHTRED,
    LIGHTMAGENTA,
    YELLOW,
    WHITE
};
#endif

extern int far DetectGraph( void );
extern void far InitGraph( void );
extern void far ExitGraph( void );
extern void far SetColor( int Color );
extern void far WaitForRetrace( void );
extern void far SetActivePage( int Page );
extern void far SetVisiblePage( int Page );
extern void far SetColor( int Color );
extern void far ClearScreen( void );
extern void far DrawLine( int x1, int y1, int x2, int y2 );
extern void far DrawRectangle( int x1, int y1, int x2, int y2 );
extern void far FillRectangle( int x1, int y1, int x2, int y2 );
extern void far DrawPolygon( int Count, int far *PointList );
extern void far FillPolygon( int Count, int far *PointList );
extern void far DrawChar( int x, int y, int Char );               
extern void far DrawText( int x, int y, char far *Text );

