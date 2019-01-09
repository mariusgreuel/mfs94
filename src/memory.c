/* Modellflugsimulator V1.0
   Marius Greuel '94
   MEMORY.C
*/

#include <alloc.h>
#include "error.h"
#include "memory.h"

/****************************************************************************/

void *AllocMemory( unsigned int b )
{
  void *p;

  p = malloc( b );
  if( p == NULL )
    RunError( FATMemoryError, EXTERN, NULL );
  return p;
}

void *ResizeMemory( void *p, unsigned int b )
{
  return realloc( p, b );
}

void FreeMemory( void *p )
{
  free( p );
}

