/* Modellflugsimulator V1.0
   Marius Greuel '94
   OBJECT.H
*/

void CreateDefinition( void );
void CloseDefinition( void );
void CreateObject( tVector *pos );
int StorePoint( tVector *Vector );
int StorePolygon( int Points, int *PointList, tColor Color, int Attribut );
void DisplayScene( int wfr );
void RotateObject( tObject *Object, tVector *Vector );
void MoveObject( tObject *Object, tVector *Vector );