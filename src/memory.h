/* Modellflugsimulator V1.0
   Marius Greuel '94
   MEMORY.H
*/

void *AllocMemory( unsigned int Bytes );
void *ResizeMemory( void *Block, unsigned int Bytes );
void FreeMemory( void *Block );
