/* Modellflugsimulator V1.0
   Marius Greuel '94
   MATH_FIX.H
   Rechnen mit Fixkommazahlen im 386'er Modus
*/

#define _PI		3.14159265359
#define _2PI		6.18318530718
#define ROUNDOFF	1e-4			/* kleinster Rundungsfehler */
#define RX		1, 2				 /* Rotationsachsen */
#define NRX		2, 1
#define RY		0, 2
#define NRY		2, 0
#define RZ		0, 1
#define NRZ		1, 0

int InitMath( void );
signed long QSin( signed long x );
void CreateEMatrix( tMatrix *m );
void MoveMatrix( tMatrix *dest, tMatrix *src );
void MulMatrix( tMatrix *c, tMatrix *a, tMatrix *b );
void ScaleMatrix( tMatrix *m, tVector *v );
void RotateMatrix( tMatrix *m, int c1, int c2, float s, float c );
void RotateRelativeMatrix( tMatrix *m, int c1, int c2, float s, float c );
