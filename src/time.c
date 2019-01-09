/* Modellflugsimulator V1.0
   Marius Greuel '94
   TIME.C
*/

#include "stdlib.h"
#include "lib\_timer.h"
#include "lib\_ega16.h"

unsigned long LastTime;
float Time = 0, FrameTime, RealFrameTime;
long Frames = 0;

/****************************************************************************/

void StartTimer( void )
{
  LastTime = ReadTimer();
}

void AdjustTime( void )
{
  unsigned long CurrentTime;
  char Text[80] = "Zeit: ";

  Frames++;
  CurrentTime = ReadTimer();
  Time += FrameTime = (CurrentTime - LastTime) / 65536.0;
  if(( RealFrameTime = FrameTime ) > 1.0 )
    RealFrameTime = 1.0;
  LastTime = CurrentTime;

  SetColor( WHITE );
  ltoa( Time*1000.0, Text+6, 10 );
  DrawText( 10,300, Text );
  ltoa( FrameTime * 1000.0, Text, 10 );
  DrawText( 10,310, Text );
  ltoa( 1.0/FrameTime, Text, 10 );
  DrawText( 10,320, Text );
  ltoa( Frames, Text, 10 );
  DrawText( 10,330, Text );
}

