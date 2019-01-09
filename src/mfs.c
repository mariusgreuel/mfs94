/* Modellflugsimulator V1.0
   Marius Greuel '94
   Hauptprogramm
*/

#include <stdio.h>
#include <conio.h>
#include "types.h"
#include "error.h"
#include "math_fix.h"
#include "time.h"
#include "script.h"
#include "object.h"
#include "heli.h"
#include "lib\_timer.h"
#include "lib\_ega16.h"
#include "lib\_mpxcom.h"

char NulString[] ="\x0";
char MSGMfs[] = "MFS V1.0, Marius Greuel '93\n";
char MSGBreakWithC[] = "Control-C bricht ab, Return zum Weitermachen\n";
static char ScriptFile[] = "MFS.SCR";

/****************************************************************************/

main( int argc )
{
  fprintf( stderr, MSGMfs );
  InitMath();
  if( DecodeFile( ScriptFile, COMMAND_LINE ) == FALSE )
    RunError( FATErrorInScriptFile, EXTERN, ScriptFile );
  if( DetectGraph() == FALSE )
    RunError( FATGraphicError, EXTERN, NULL );
  if( InitTimer() == FALSE )
    RunError( FATTimerError, EXTERN, NULL );
  if( InitRC() == RC_NOT_FOUND )
    RunError( WARRcNotFound, EXTERN, NULL );
  InitGraph();
  StartTimer();
  while( !kbhit() )
  {
    AdjustTime();
    CalcHeli();
    DisplayScene( argc <= 1 );
  }
  getch();
  ExitProgram( EXIT_OK );
  return 0;
}
