/* MPXCOM.C
   Programm zum Test von _MPXCOM.ASM
   Marius Greuel '95
*/

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include "_mpxcom.h"

#define STICKS 7

void my_exit( void )
{
  ExitRC();
}

int cb_hand( void )
{
  return( 1 );
}

int main( int _argc, char *_argv[] )
{
  int x, mode = RC_STD_MODE, mint = RC_STD_SHORT_PW, maxt = RC_STD_LONG_PW;

  atexit( my_exit );
  ctrlbrk( cb_hand );
  printf( "MPX-COM Test, MG'95\n");
  if( _argc == 2 ) {
    mode = atoi( _argv[ 1 ]);
  } else if( _argc == 4 ) {
    mode = atoi( _argv[ 1 ]);
    mint = atoi( _argv[ 2 ]);
    maxt = atoi( _argv[ 3 ]);
  } else if ( _argc != 1 ) {
    printf( "  Aufruf: MPXCOM [Modus<%u>[, MinTime<%uus>, MaxTime<%uus>]]\n",
	    mode, mint, maxt );
    return 2;
  }
  if( InitRC() == RC_NOT_FOUND ) {
    printf( "Kein Interface gefunden !\n" );
  }
  printf( "Port: %04xh, IRQ: %02xh\n", GetRCPort(), GetRCIRQ() );
  SetRCTiming( mode, mint, maxt );
  while( !kbhit()) {
    for( x = 0; x < STICKS; ++x )
      printf( "%4d ", GetRCStick( x ));
    if( GetRCState() == RC_NO_ERROR )
      printf( "             " );
    else
      printf( "Keine Daten !" );
    printf( "\r" );
  }
  printf( "\n" );
  getch();
  return 0;
}
