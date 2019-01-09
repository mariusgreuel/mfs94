/* Modellflugsimulator V1.0
   Marius Greuel '94
   MATH_FIX.C
   Rechnen mit Fixkommazahlen im 386'er Modus
*/

#include <dos.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "types.h"
#include "memory.h"
#include "error.h"
#include "math_fix.h"

#define SIN_STEPS 1024                         /* Werte Sinustabelle 0-Pi/2 */
#define MAX_SIN_VALUE 1073741824

long *SinTable;                                      /* Sinustabelle 0-Pi/2 */
void interrupt (*OldMathError)() = NULL;

/****************************************************************************/

signed long QSin( signed long x )
/* QuickSinus, liefert Fixkomma PI/2 == 1/e-30 */
{
  int Neg = 0;

  if( x < 0 ) {
    x = -x;
    Neg ^= 1;
  }
  if( x & ( SIN_STEPS << 1 ))
    Neg ^= 1;
  if( x & SIN_STEPS )
    x = SinTable[ SIN_STEPS - x & ( SIN_STEPS - 1 )];
  else
    x = SinTable[ x & ( SIN_STEPS - 1 )];
  return Neg ? -x : x;
}

int InitSinTable( void )
/* initialisiert die Sinustable von 0 bis Pi/2 */
{
  float k1 = M_PI_2 / SIN_STEPS;
  float k2 = MAX_SIN_VALUE;
  long *Table, x;
  int i;

  Table = (long*)AllocMemory( SIN_STEPS * sizeof( long ));
  if( Table == NULL )
    return FALSE;
  SinTable = Table;
  for ( i=0; i<SIN_STEPS; i++ ) {
    x =( sin((float)i * k1 ) * k2 + 0.5 );
    *Table = x;
    ++Table;
  }
  return TRUE;
}

void interrupt MathError( void )
{
  RunError( FATDivisionByZero, INTERNAL, NULL );
}

void CloseMath( void )
{
  if( OldMathError )
    setvect( 0, OldMathError );
  FreeMemory( SinTable );
}

int InitMath( void )
{
  if( InitSinTable == FALSE )
    return FALSE;
  if( atexit( CloseMath ) != NULL )
    return FALSE;
  OldMathError = getvect( 0 );
  setvect( 0, MathError );
  return TRUE;
}

/****************************************************************************/

void CreateEMatrix( tMatrix *m )
/* erstellt eine Einheitsmatrix */
{
  int i, j;

  for( i=0; i<4; i++ )
    for( j=0; j<4; j++ )
      if( i==j )
	(*m)[i][j] = 1.0;
      else
	(*m)[i][j] = 0.0;
}

void MoveMatrix( tMatrix *dest, tMatrix *src )
/* kopiert Matrix von Source nach Dest */
{
  memmove( dest, src, sizeof( tMatrix ));
}

void MulMatrix( tMatrix *c, tMatrix *a, tMatrix *b )
/* Matrixmultiplikation C = A * B */
{
  int i, j, k;
  float s;

  for( i=0; i<4; i++ )
    for( k=0; k<4; k++ )
    {
      s=0;
      for( j=0; j<4; j++ )
	s += (*a)[i][j] * (*b)[j][k];
      (*c)[i][k] = s;
    }
}

void ScaleMatrix( tMatrix *m, tVector *v )
/* skaliert eine homogene Matrix */
{
  int i, j;

  for( i=0; i<4; i++)
    for( j=0; j<3; j++ )
      (*m)[i][j] *= (*v)[j];
}

void RotateMatrix( tMatrix *m, int c1, int c2, float s, float c )
/* rotiert homogene Matrix um eine Raum-Achse */
{
  int i;
  float t;

  for( i=0; i<4; i++) {
    t           = s * (*m)[i][c1] + c * (*m)[i][c2];
    (*m)[i][c1] = c * (*m)[i][c1] - s * (*m)[i][c2];
    (*m)[i][c2] = t;
  }
}

void RotateRelativeMatrix( tMatrix *m, int c1, int c2, float s, float c )
/* rotiert inhomogene! Matrix um ihre relative Achse */
{
  int i;
  float t;

  for( i=0; i<3; i++) {
    t           = s * (*m)[c1][i] + c * (*m)[c2][i];
    (*m)[c1][i] = c * (*m)[c1][i] - s * (*m)[c2][i];
    (*m)[c2][i] = t;
  }
}

/****************************************************************************/


