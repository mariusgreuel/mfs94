/* Modellflugsimulator V1.0
   Marius Greuel '94
   HELI.C
*/

#include <stdlib.h>
#include <math.h>
#include "types.h"
#include "math_fix.h"
#include "object.h"
#include "heli.h"
#include "lib\_mpxcom.h"
#include "lib\_ega16.h"

#define NICK            2
#define ROLL            0
#define GIER            1
#define PITCH           3

extern tObject *Aircraft;
extern float RealFrameTime;

float Stick[4];
float MotorRPM = 0.0;
tVector Moment;
tVector Force;
tVector RotateSpeed = { 0.0, 0.0, 0.0 };
tVector GroundSpeed = { 0.0, 0.0, 0.0 };
tVector WindSpeed = { 0.0, 0.0, 0.0 };
tVector AirSpeed;

float Mass = .2;
float Gravity = 9.81;
float MotorRiseRate = 0.5;
float MainRotorThrust = 2.0;
float TailRotorThrust = 0.05;
float MaxHeight = 10.0;
float GroundEffectHight = 1.0;
float GroundEffectAmount = 0.1;
tVector Drag = { 0.1, 0.05, 0.1 };
tVector Sensitivity = { 0.6, -2.0, 0.6 };
tVector Inertia = { 1.0, 0.6, 1.0 };
tVector Spin = { 1.2, 2.0, 1.2 };
tVector RotateWithWind = { -0.001, 0.07, -0.008 };

int y;

/****************************************************************************/

float disp( float f )
{
  char Text[80];
  SetColor( WHITE );
  ltoa( f*1000.0, Text, 10 );
  DrawText( 10,y, Text );
  y += 10;
  return f;
}

static void CalcStickValues( void )
{
  Stick[ NICK ] = GetRCStick( 3 ) / 256.0;
  Stick[ ROLL ] = GetRCStick( 2 ) / 256.0;
  Stick[ GIER ] = GetRCStick( 0 ) / 256.0;
  Stick[ PITCH ] = GetRCStick( 1 ) / -256.0;
}

static void CalcAirSpeed( void )
{
  int i;

  for( i=0; i<3; i++ )
    AirSpeed[i] = GroundSpeed[i] - WindSpeed[i];
}

/****************************************************************************/

static void CalcRotorMoments( void )
{
  int i;

  for( i=0; i<3; i++ )
    Moment[i] = Sensitivity[i] * Stick[i];
}

static void CalcWindMoments( void )
{
  tMatrix *m = &Aircraft->Matrix;
  int i, j, axe[3] = { X, Z, X };
  float d;

  for( i = 0; i < 3; i++ ) {
    d=0.0;
    for( j = 0; j < 3; j ++ )
      d +=(*m)[ axe[i]][j] * AirSpeed[j];
    Moment[i] += d * RotateWithWind[i];
  }
}

static void CalcSpin( void )
{
 int i;

  for( i=0; i<3; i++ )
    Moment[i] -= Spin[i] * RotateSpeed[i];
}

static void CalcRotation( void )
{
  int i;

  for( i=0; i<3; i++ )
    RotateSpeed[i] += Moment[i] / Inertia[i] * RealFrameTime;
}

static void RotateHeli( void )
{
  tVector Rotation;
  int i;

  for( i=0; i<3; i++ )
    Rotation[i] = RotateSpeed[i] * RealFrameTime * _2PI;
  RotateObject( Aircraft, &Rotation );
}

/****************************************************************************/

static void CalcMotorRPM( void )
{
  float d1, d2;

  d1 = Stick[ PITCH ] + 1 - MotorRPM;
  d2 = RealFrameTime / MotorRiseRate;
  if( d1 > d2 )
    d1 = d2;
  else if( d1 < -d2 )
    d1 = -d2;
  MotorRPM += d1;
  if( GetRCState() != RC_NO_ERROR )
    MotorRPM = 0.0;
}

static void CalcRotorForces( void )
{
  tMatrix *m = &Aircraft->Matrix;
  int i;
  float mrf, trf, h;

  mrf = MotorRPM * MainRotorThrust;
  trf = MotorRPM * TailRotorThrust;
  h = ( MaxHeight - (*m)[H][Y] ) / MaxHeight;
  if( h > 0.0 )
    mrf *= h;
  h = ( GroundEffectHight - (*m)[H][Y] ) * GroundEffectAmount;
  if( h > 0.0 )
    mrf *= 1.0 + h;
  for( i=0; i<3; i++ )
    Force[i] =(*m)[Y][i] * mrf;
  for( i=0; i<3; i++ )
    Force[i] +=(*m)[Z][i] * trf;
}

static void CalcGravity( void )
{
  Force[ Y ] -= Mass * Gravity;
}

static void CalcDrag( void )
{
  tMatrix *m = &Aircraft->Matrix;
  int i, j;
  float d;

  for( i=0; i<3; i++ ) {
    d = 0.0;
    for( j=0; j<3; j++ ) {
      d += fabs((*m)[i][j]) * AirSpeed[i] * Drag[j];
    }
    Force[i] -= d;
  }
}

static void CalcMovement( void )
{
  int i;

  for( i=0; i<3; i++ )
    GroundSpeed[i] += Force[i] / Mass * RealFrameTime;
}

static void MoveHeli( void )
{
  tVector Step;
  int i;

  for( i=0; i<3; i++ )
    Step[i] = GroundSpeed[i] * RealFrameTime;
  MoveObject( Aircraft, &Step );
}

/****************************************************************************/

static void AdjustPosition( void )
{
  tMatrix *Matrix = &Aircraft->Matrix;
  int i;

  if((*Matrix)[H][Y] < 0.0 ) {
    (*Matrix)[H][Y] = 0.0;
    for( i=0; i<3; i++ )
      GroundSpeed[i] = 0.0;
  }
}

void CalcHeli( void )
{
  y = 20;
  if( Aircraft == NULL )
    return;
  CalcStickValues();
  CalcAirSpeed();
  CalcRotorMoments();
  CalcWindMoments();
  CalcSpin();
  CalcRotation();
  RotateHeli();
  CalcMotorRPM();
  CalcRotorForces();
  CalcGravity();
  CalcDrag();
  CalcMovement();
  MoveHeli();
  AdjustPosition();
}
