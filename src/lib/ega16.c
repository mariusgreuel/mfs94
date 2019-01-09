/* Programm zum Test von _EGA16.ASM
   Marius Greuel '95
*/

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include "_ega16.h"

#define TRUE 1
#define FALSE 0

struct { int x, y; }
  Buffer1[10],
  Buffer2[] = { 50,150, 100,50, 200,200 };

void DrawLineDemo( void )
{
  SetColor( BLACK );
  ClearScreen();
  while( !kbhit()) {
    SetColor( random( 16 ));
    DrawLine( 100, 100, random( 640 ), random( 350 ));
  }
  getch();
}

void DrawRectangleDemo( void )
{
  SetColor( BLACK );
  ClearScreen();
  while( !kbhit()) {
    SetColor( random( 16 ));
    DrawRectangle( random( 540 ), random( 250 ), 100, 100 );
  }
  getch();
}

void FillRectangleDemo( void )
{
  SetColor( BLACK );
  ClearScreen();
  while( !kbhit()) {
    SetColor( random( 16 ));
    FillRectangle( random( 540 ), random( 250 ), 100, 100 );
  }
  getch();
}

void DrawPolygonDemo( void )
{
  int i;
  SetColor( BLACK );
  ClearScreen();
  while( !kbhit()) {
    SetColor( random( 16 ));
    for( i=0; i<3; i++ ) {
      Buffer1[i].x = random( 640 );
      Buffer1[i].y = random( 350 );
    }
    DrawPolygon( 3, (int far*)Buffer1 );
  }
  getch();
}

void FillPolygonDemo( void )
{
  int i;
  SetColor( BLACK );
  ClearScreen();
  while( !kbhit()) {
    SetColor( random( 16 ));
    for( i=0; i<3; i++ ) {
      Buffer1[i].x = random( 640 );
      Buffer1[i].y = random( 350 );
    }
    FillPolygon( 3, (int far*)Buffer1 );
  }
  getch();
}

void DrawCharDemo( void )
{
  SetColor( BLACK );
  ClearScreen();
  while( !kbhit()) {
    SetColor( random( 16 ));
    DrawChar( random( 640 ), random( 350 ), random( 128 ));
  }
  getch();
}

void DrawTextDemo( void )
{
  SetColor( BLACK );
  ClearScreen();
  while( !kbhit()) {
    SetColor( random( 16 ));
    DrawText( random( 640 ), random( 350 ), "Grafik-Demo" );
  }
  getch();
}

void MoveDemo( void )
{
  int p = 0, x=0, y=0, xi = 2, yi = 2, i;

  SetActivePage( 0 );
  SetColor( BLACK );
  ClearScreen();
  SetColor( WHITE );
  DrawText( 280,30, "Grafik-Demo" );
  SetActivePage( 1 );
  SetColor( BLACK );
  ClearScreen();
  SetColor( WHITE );
  DrawText( 280,30, "Grafik-Demo" );
  while( !kbhit()) {
    SetActivePage( p );
    SetVisiblePage( p^=1 );
    WaitForRetrace();
    SetColor( DARKGRAY );
    FillRectangle( 50, 50, 540, 250 );
    SetColor( LIGHTRED );
    FillPolygon( 3, (int far*)Buffer2 );
    SetColor( WHITE );
    DrawText( x+100, y+100, "Grafik-Demo" );
    x += xi;
    y += yi;
    for( i=0; i<3; i++ ) {
      Buffer2[i].x += xi;
      Buffer2[i].y += yi;
    }
    if( x <= 0 || x >= 390-xi )
      xi *= -1;
    if( y <= 0 || y >= 100-yi )
      yi *= -1;
  }
}

main()
{
  printf( "Grafik Test, MG'95\n" );
  if( DetectGraph() == FALSE ) {
    printf( "Keine VGA-Karte vorhanden !\n" );
    return 2;
  }
  InitGraph();
    DrawLineDemo();
    DrawRectangleDemo();
    FillRectangleDemo();
    DrawPolygonDemo();
    FillPolygonDemo();
    DrawCharDemo();
    DrawTextDemo();
  MoveDemo();
  ExitGraph();
  return 0;
}
