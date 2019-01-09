/* Modellflugsimulator V1.0
   Marius Greuel '94
   SCRIPT.H
*/

typedef struct Location {
  int Line;
  unsigned char *Text;
  unsigned char *Name;
}tLocation;

int DecodeFile( unsigned char *Name, int Source );
