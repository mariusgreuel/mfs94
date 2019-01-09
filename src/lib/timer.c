/* Programm zum Test von _TIME.ASM
   Marius Greuel '95
*/

#include <stdio.h>
#include <conio.h>
#include "_timer.h"

#define FALSE 0

main()
{
  unsigned long t;
  printf( "Timer Test, MG'95\n" );
  if( InitTimer() == FALSE ) {
    printf( "Timer-Error\n" );
    return 2;
  }
  t = ReadTimer();
  while( !kbhit())
    printf("\rZeit: %f", (ReadTimer()-t) / 65536.0 );
  getch();
  return 0;
}