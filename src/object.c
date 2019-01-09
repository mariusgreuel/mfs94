/* Modellflugsimulator V1.0
   Marius Greuel '94
   OBJECT.C
   Objektaufbau, Objektdarstellung
*/

#include <math.h>
#include <limits.h>
#include "types.h"
#include "error.h"
#include "memory.h"
#include "object.h"
#include "math_fix.h"
#include "lib\_ega16.h"

#define MAXPOINTS	100
#define MAXPOLYGONS	100

float disp( float f );
extern float FrameTime;

unsigned MaxPoints = 0, MaxPolygons = 0;
tDefList *stDefList = NULL;
tObject *stObject = NULL, *Aircraft = NULL, *Shadow = NULL;
tHVector RefPlane;
#define MAX_POINTS_PER_POLYGON 20
int PointBuffer[ MAX_POINTS_PER_POLYGON*2 ];
tP2D PointBuffer1[ MAX_POINTS_PER_POLYGON ];
tP2D PointBuffer2[ MAX_POINTS_PER_POLYGON ];
tVector ViewPoint = { -10.0, 1.8, -20.0 }, ViewPlane;
float ViewDistance = 0.7;
int Resolution[2] = { 640, 350 };
float Screen[2] = { 0.27, 0.17 };
int WindowMin[2] = { 0, 0 };
int WindowMax[2] = { 639, 349 };
float BackPlane = 0.7;
int View = 1;

float dd;

/****************************************************************************/

void CalcReferenzPolygonPlane( tPolygon *pg )
/* ermittelt die Ebene des Polygons anhand der ersten drei Punkte
   -> Referenzebene RefPlane (Normalvektorform) */
{
  float *p1, *p2, *p3;

  p1 = pg->Point[0]->op;
  p2 = pg->Point[1]->op;
  p3 = pg->Point[2]->op;
  RefPlane[X] = (p2[Y]-p1[Y]) * (p3[Z]-p1[Z]) - (p2[Z]-p1[Z]) * (p3[Y]-p1[Y]);
  RefPlane[Y] = (p2[Z]-p1[Z]) * (p3[X]-p1[X]) - (p2[X]-p1[X]) * (p3[Z]-p1[Z]);
  RefPlane[Z] = (p2[X]-p1[X]) * (p3[Y]-p1[Y]) - (p2[Y]-p1[Y]) * (p3[X]-p1[X]);
  RefPlane[H] = -( RefPlane[X]*p1[X] + RefPlane[Y]*p1[Y] + RefPlane[Z]*p1[Z] );
}

signed LocatePolygon( tPolygon *pg )
/* vergleicht das Polygon mit der Referenzebene */
{
  int Front = 0, Back = 0, i;
  float *pt, d;

  for( i = 0; i < pg->Points; i++) {
    pt = pg->Point[i]->op;
    d = pt[X] * RefPlane[X] + pt[Y] * RefPlane[Y] +
	pt[Z] * RefPlane[Z] + RefPlane[H];
    if( d > ROUNDOFF )
      Front++;
    else if ( d < -ROUNDOFF )
      Back++;
  }
  dd = d;
  if( Back == 0 && Front == 0 )
    return 0;                                   /* alle Punkte in der Ebene */
  if( Back == 0 )
    return 2;                                  /* alle Punkte vor der Ebene */
  if( Front == 0 )
    return -2;                              /* alle Punkte hinter der Ebene */
  if( Front > Back )
    return 1;                                /* einige Punkte vor der Ebene */
  else
    return -1;                            /* einige Punkte hinter der Ebene */
}

static void SetToTopPolygon( tPolygon **Root, tPolygon *RefPg )
/* setzt das Polygon an die Spitze der Polygonliste */
{
  tPolygon *pg;

  if(( pg = *Root ) == RefPg )
    return;
  while( pg->Front != RefPg )
    pg = pg->Front;
  pg->Front = RefPg->Front;
  RefPg->Front = *Root;
  *Root = RefPg;
}

static tPolygon *SearchSplitPolygon( tPolygon *List )
/* sucht ein Polygon, das am besten geeignet ist,
   die Polygone im Raum zu trennen */
{
  signed MaxSplits = INT_MAX, MaxTouches = INT_MAX, Splits, Touches, i;
  tPolygon *BestPg = List, *RefPg, *pg;

  RefPg = List;
  while( RefPg != NULL ) {
    CalcReferenzPolygonPlane( RefPg );
    pg = List;
    Splits = Touches = 0;
    while( pg != NULL ) {
      Splits += i = LocatePolygon( pg );
      if( i == 1 || i == -1 )
	Touches++;
      pg = pg->Front;
    }
    if( Touches < MaxTouches ||	Touches == MaxTouches && Splits < MaxSplits ) {
      MaxSplits = Splits;
      MaxTouches = Touches;
      BestPg = RefPg;
    }
    RefPg = RefPg->Front;
  }
  return BestPg;
}

static void BuildBspTree( tPolygon **Root )
/* erstellt aus der Polygonliste gem„á dem BinarySpacePartitioning-Algorithmus
   ( nach Henry Fuchs ) einen einfach verketteten bin„ren Baum */
{
  tPolygon *RefPg, *pg, *NextPg;
  if( *Root == NULL )
    return;
  RefPg = SearchSplitPolygon( *Root );
  CalcReferenzPolygonPlane( RefPg );
  SetToTopPolygon( Root, RefPg );
  pg = RefPg->Front;
  RefPg->Front = NULL;
  RefPg->Back = NULL;
  while( pg != NULL ) {
    NextPg = pg->Front;
    if( LocatePolygon( pg ) == 2 ) {
      pg->Front = RefPg->Front;
      RefPg->Front = pg;
    } else {
      pg->Front = RefPg->Back;
      RefPg->Back = pg;
    }
    pg = NextPg;
  }
  BuildBspTree( &RefPg->Front );
  BuildBspTree( &RefPg->Back );
}

/****************************************************************************/

int StorePoint( tVector *v )
/* schreibt einen Punkt in die Punktliste */
{
  tP3D *p;

  if( stDefList->Points >= MaxPoints ) {
    RunError( FATPointBufferExceeded, EXTERN, NULL );
    return FALSE;
  }
  p = &( stDefList->Point[ stDefList->Points ].op );
  (*p)[X] = (*v)[X];
  (*p)[Y] = (*v)[Y];
  (*p)[Z] = (*v)[Z];
  stDefList->Points++;
  return TRUE;
}

int StorePolygon( int pts, int pt[], tColor c, int Attr )
/* schreibt ein Polygon in die Polygonliste */
{
  tPolygon *p;
  int i;

  if( pts < 3 ) {
    RunError( ERRBadPolygon, IN_FILE, NULL );
    return FALSE;
  }
  if( stDefList->Polygons >= MaxPolygons ) {
    RunError( FATPolygonBufferExceeded, EXTERN, NULL );
    return FALSE;
  }
  p = &( stDefList->Polygon[ stDefList->Polygons ]);
  p->Front = stDefList->BspRoot;
  p->Back = NULL;
  p->Color = c;
  p->Points = pts;
  for( i=0; i<pts; i++ )
    p->Point[i] = &stDefList->Point[ pt[i] - 1 ];
  p->Attribut = Attr;
  stDefList->BspRoot = p;
  stDefList->Polygons++;
  CalcReferenzPolygonPlane( p );
  if( LocatePolygon( p ) != 0 ) {
    RunError( WARPointsNotPlane, IN_FILE, NULL );
    return FALSE;
  }
  return TRUE;
}

void CreateDefinition( void )
{
  tDefList *d;

  d = (tDefList*)AllocMemory( sizeof( tDefList ));
  d->BspRoot = NULL;
  d->Points = 0;
  d->Point = (tPoint*)AllocMemory( sizeof( tPoint ) * MAXPOINTS );
  MaxPoints = MAXPOINTS;
  d->Polygons = 0;
  d->Polygon = (tPolygon*)AllocMemory( sizeof( tPolygon ) * MAXPOLYGONS );
  MaxPolygons = MAXPOLYGONS;
  stDefList = d;
}

void CloseDefinition( void )
{
  BuildBspTree( &stDefList->BspRoot );
/*  SizeBuffer( Polygon );
  SizeBuffer( Point );*/
}

void CreateObject( tVector *pos )
{
  int i;
  tObject *o;

  o = (tObject*)AllocMemory( sizeof( tObject ));
  o->Next = stObject;
  o->DefList = stDefList;
  o->Visible = TRUE;
  o->Modified = FALSE;
  CreateEMatrix( &o->Matrix );
  for( i=0; i<3; i++ )
    o->Matrix[H][i] = (*pos)[i];
  if( Aircraft == NULL )
    Aircraft = o;
  else if( Shadow == NULL )
    Shadow = o;
  stObject = o;
}

/****************************************************************************/

/* rotiert Objekt um seine relativen Achsen */
void RotateObject( tObject *o, tVector *v )
{
  tMatrix *m = &o->Matrix;
  float a, b, sx, cx, sy, cy;

  a = (*v)[Y]*(*v)[Y] + (*v)[Z]*(*v)[Z];
  b = sqrt( a + (*v)[X]*(*v)[X] );
  a = sqrt( a );
  if( fabs( a ) > ROUNDOFF ) {
    sx = (*v)[Y] / a;
    cx = (*v)[Z] / a;
  } else {
    sx = 0.0;
    cx = 1.0;
  }
  if( fabs( b ) > ROUNDOFF ) {
    sy = (*v)[X] / b;
    cy = a / b;
    RotateRelativeMatrix( m, RX, sx, cx );
    RotateRelativeMatrix( m, RY, sy, cy );
    RotateRelativeMatrix( m, RZ, sin( b ), cos( b ));
    RotateRelativeMatrix( m, NRY, sy, cy );
    RotateRelativeMatrix( m, NRX, sx, cx );
  }
  o->Modified = TRUE;
}

/* bewegt Object */
void MoveObject( tObject *o, tVector *v )
{
  tMatrix *m = &o->Matrix;
  int i;

  for( i=0; i<3; i++ )
    (*m)[H][i] += (*v)[i];
  o->Modified = TRUE;
}


/****************************************************************************/

/* transformiert die Weltkoordinaten in Ger„tekoordinaten und
   projeziert die Punkte */
static void TransformPoints( tPoint *pt, int pts, tMatrix *m )
{
  int i, j, k;
  float d, z;

  for( i=0; i<pts; i++ ) {
    for( j=0; j<3; j++ ) {
      d = 0.0;
      for( k=0; k<3; k++ )
	d += pt->op[k] * (*m)[k][j];
      d += (*m)[H][j];
      pt->tp[j] = d;
    }
    if( pt->tp[Z] > BackPlane ) {
      z = 1.0 / pt->tp[Z];
      pt->pp[X] = pt->tp[X] * z + ( Resolution[X] >> 1 );
      pt->pp[Y] = pt->tp[Y] * z + ( Resolution[Y] >> 1 );
    }
    pt++;
  }
}

static void BackIntersection( tPoint *p1, tPoint *p2, tP2D *b )
{
  float dx, dy, dz, d;

  dx = p2->tp[X]-p1->tp[X];
  dy = p2->tp[Y]-p1->tp[Y];
  dz = p2->tp[Z]-p1->tp[Z];
  d = BackPlane - p1->tp[Z];
  (*b)[X] = (p1->tp[X] + (d*dx/dz)) / BackPlane + ( Resolution[X] >> 1 );
  (*b)[Y] = (p1->tp[Y] + (d*dy/dz)) / BackPlane + ( Resolution[Y] >> 1 );
}

static int LeftIntersection( tP2D *p1, tP2D *p2 )
{
  return (*p1)[Y] + (long)( WindowMin[X]-(*p1)[X] )
	 * ((*p2)[Y]-(*p1)[Y]) / ((*p2)[X]-(*p1)[X]);
}

static int RightIntersection( tP2D *p1, tP2D *p2 )
{
  return (*p1)[Y] + (long)( WindowMax[X]-(*p1)[X] )
	 * ((*p2)[Y]-(*p1)[Y]) / ((*p2)[X]-(*p1)[X]);
}

static int TopIntersection( tP2D *p1, tP2D *p2 )
{
  return (*p1)[X] + (long)( WindowMin[Y]-(*p1)[Y] )
	 * ((*p2)[X]-(*p1)[X]) / ((*p2)[Y]-(*p1)[Y]);
}

static int BottomIntersection( tP2D *p1, tP2D *p2 )
{
  return (*p1)[X] + (long)( WindowMax[Y]-(*p1)[Y] )
         * ((*p2)[X]-(*p1)[X]) / ((*p2)[Y]-(*p1)[Y]);
}

static int ClipBack( int pts, tPoint **pt, tP2D *Buffer )
{
  int i;
  int r1, r2;
  tPoint *p1, *p2;
  tP2D *b;

  b = Buffer;
  p1 = pt[ pts - 1 ];
  r1 = p1->tp[Z] > BackPlane ? 0 : 1;
  for( i = 0; i < pts; i++ )
  {
    p2 = *pt; pt++;
    r2 = p2->tp[Z] > BackPlane ? 0 : 1;
    if( r1 == 0 ) {
      (*b)[X] = p1->pp[X];
      (*b)[Y] = p1->pp[Y];
      b++;
    }
    if( r1 ^ r2 ) {
      BackIntersection( p1, p2, b );
      b++;
    }
    p1 = p2;
    r1 = r2;
  }
  return b - Buffer;
}

static int ClipLeft( int pts, tP2D *pt, tP2D *Buffer )
{
  int i;
  int r1, r2;
  register tP2D *b, *p1, *p2;

  b = Buffer;
  p1 = pt + pts - 1;
  r1 = (*p1)[X] >= WindowMin[X] ? 0 : 1;
  p2 = pt;
  for( i = 0; i < pts; i++ )
  {
    r2 = (*p2)[X] >= WindowMin[X] ? 0 : 1;
    if( r1 == 0 ) {
      (*b)[X] = (*p1)[X];
      (*b)[Y] = (*p1)[Y];
      b++;
    }
    if( r1 ^ r2 ) {
      (*b)[X] = WindowMin[X];
      (*b)[Y] = LeftIntersection( p1, p2 );
      b++;
    }
    p1 = p2;
    r1 = r2;
    p2++;
  }
  return b - Buffer;
}

static int ClipRight( int pts, tP2D *pt, tP2D *Buffer )
{
  int i;
  int r1, r2;
  register tP2D *b, *p1, *p2;

  b = Buffer;
  p1 = pt + pts - 1;
  r1 = (*p1)[X] <= WindowMax[X] ? 0 : 1;
  p2 = pt;
  for( i = 0; i < pts; i++ )
  {
    r2 = (*p2)[X] <= WindowMax[X] ? 0 : 1;
    if( r1 == 0 ) {
      (*b)[X] = (*p1)[X];
      (*b)[Y] = (*p1)[Y];
      b++;
    }
    if( r1 ^ r2 ) {
      (*b)[X] = WindowMax[X];
      (*b)[Y] = RightIntersection( p1, p2 );
      b++;
    }
    p1 = p2;
    r1 = r2;
    p2++;
  }
  return b - Buffer;
}

static int ClipTop( int pts, tP2D *pt, tP2D *Buffer )
{
  int i;
  int r1, r2;
  register tP2D *b, *p1, *p2;

  b = Buffer;
  p1 = pt + pts - 1;
  r1 = (*p1)[Y] >= WindowMin[Y] ? 0 : 1;
  p2 = pt;
  for( i = 0; i < pts; i++ )
  {
    r2 = (*p2)[Y] >= WindowMin[Y] ? 0 : 1;
    if( r1 == 0 ) {
      (*b)[X] = (*p1)[X];
      (*b)[Y] = (*p1)[Y];
      b++;
    }
    if( r1 ^ r2 ) {
      (*b)[X] = TopIntersection( p1, p2 );
      (*b)[Y] = WindowMin[Y];
      b++;
    }
    p1 = p2;
    r1 = r2;
    p2++;
  }
  return b - Buffer;
}

static int ClipBottom( int pts, tP2D *pt, tP2D *Buffer )
{
  int i;
  int r1, r2;
  register tP2D *b, *p1, *p2;

  b = Buffer;
  p1 = pt + pts - 1;
  r1 = (*p1)[Y] <= WindowMax[Y] ? 0 : 1;
  p2 = pt;
  for( i = 0; i < pts; i++ )
  {
    r2 = (*p2)[Y] <= WindowMax[Y] ? 0 : 1;
    if( r1 == 0 ) {
      (*b)[X] = (*p1)[X];
      (*b)[Y] = (*p1)[Y];
      b++;
    }
    if( r1 ^ r2 ) {
      (*b)[X] = BottomIntersection( p1, p2 );
      (*b)[Y] = WindowMax[Y];

      b++;
    }
    p1 = p2;
    r1 = r2;
    p2++;
  }
  return b - Buffer;
}

/* speichert die Polygonpunkte im Puffer */
static int StorePolygonPoints( tPolygon *pg )
{
  int i, pts;

  pts = ClipBack( pg->Points, pg->Point, PointBuffer1 );
  pts = ClipLeft( pts, PointBuffer1, PointBuffer2 );
  pts = ClipRight( pts, PointBuffer2, PointBuffer1 );
  pts = ClipTop( pts, PointBuffer1, PointBuffer2 );
  pts = ClipBottom( pts, PointBuffer2, PointBuffer1 );
  for( i = 0; i < pts; i++ ) {
    PointBuffer[ i<<1 ] = PointBuffer1[i][X];
    PointBuffer[ (i<<1) +1 ] = PointBuffer1[i][Y];
  }
  return pts;
}

static void DisplayPolygon( tPolygon *pg )
{
  int i, pts;

  pts = StorePolygonPoints( pg );
  if( pts < 3 )
    return;
  if( pg->Attribut & FILL ) {
    SetColor( pg->RealColor );
    FillPolygon( pts, PointBuffer );
    if( pg->Attribut & DRAW ) {
      SetColor( BLACK );
      DrawPolygon( pts, PointBuffer );
    }
  } else {
    SetColor( pg->Color );
    DrawPolygon( pts, PointBuffer );
  }
}

static void DisplayLine( tPolygon *pg )
{
  if( StorePolygonPoints( pg ) < 2 )
    return;
  SetColor( pg->Color );
  DrawPolygon( 2, PointBuffer );
}

static int PolygonVisible( tPolygon *Polygon )
{
  tP2D *p1, *p2, *p3;

  p1 = &Polygon->Point[0]->pp;
  p2 = &Polygon->Point[1]->pp;
  p3 = &Polygon->Point[2]->pp;
  return ((*p2)[X]-(*p1)[X])*((*p3)[Y]-(*p1)[Y]) -
	 ((*p2)[Y]-(*p1)[Y])*((*p3)[X]-(*p1)[X]) > 0;
}

static void RunBsp( tPolygon *Polygon )
{
  if( Polygon == NULL )
    return;
  if( PolygonVisible( Polygon )) {
    RunBsp( Polygon->Back );
    if( !( Polygon->Attribut & DUMMY ))
      if( Polygon->Attribut & DRAW+FILL ) {
	Polygon->RealColor = Polygon->Color;
	DisplayPolygon( Polygon );
      } else
	DisplayLine( Polygon );
    RunBsp( Polygon->Front );
  } else {
    RunBsp( Polygon->Front );
    if( Polygon->Attribut & TWO_SIDES ) {
      Polygon->RealColor = Polygon->Color;
      DisplayPolygon( Polygon );
    } else if( Polygon->Attribut & BACK_FACE ) {
      Polygon->RealColor = Polygon->Color & 7;
      DisplayPolygon( Polygon );
    } else if( Polygon->Attribut & LINE )
      DisplayLine( Polygon );
    RunBsp( Polygon->Back );
  }
}

static void DisplayHorizont( void )
{
  SetColor( LIGHTGRAY );
  ClearScreen();
}

static void CalcViewPlane( void )
{
  int i;

  if( View )
    for( i=0; i<3; i++ )
      ViewPlane[i] = Aircraft->Matrix[H][i] - ViewPoint[i];
  else
    for( i=0; i<3; i++ )
      ViewPlane[i] = - ViewPoint[i];
}

static void CalcViewMatrix( tMatrix *m )
{
  int i;
  float a, b, c, sx, cx, sy, cy;
  tVector v = { 0.0, 0.0, 0.0 };

  CreateEMatrix( m );
  (*m)[H][X] -= ViewPoint[X];
  (*m)[H][Y] -= ViewPoint[Y];
  (*m)[H][Z] -= ViewPoint[Z];
  a = ViewPlane[X]*ViewPlane[X] + ViewPlane[Z]*ViewPlane[Z];
  b = sqrt( a + ViewPlane[Y]*ViewPlane[Y] );
  a = sqrt( a );
  if( fabs( a ) > ROUNDOFF ) {
    sy = ViewPlane[X] / a;
    cy = ViewPlane[Z] / a;
  } else {
    sy = 0.0;
    cy = 1.0;
  }
  if( fabs( b ) > ROUNDOFF ) {
    sx = ViewPlane[Y] / b;
    cx = a / b;
    RotateMatrix( m, RY, sy, cy );
    RotateMatrix( m, RX, sx, cx );
  }
  v[X] = Resolution[X] / Screen[X];
  v[Y] = -Resolution[Y] / Screen[Y];
  v[Z] = 1.0;
  ScaleMatrix( m, &v );
}

static void CalcShadowMatrix( tMatrix *m )
{
  MoveMatrix( m, &Aircraft->Matrix );
  (*m)[H][X] += (*m)[H][Y] * 0.2;
  (*m)[H][Y] = 0.0;
}

void DisplayScene( int wfr )
{
  static int page = 0;
  tObject *o;
  tMatrix Matrix, ViewMatrix, ShadowMatrix;

  CalcViewPlane();
  CalcViewMatrix( &ViewMatrix );
  SetActivePage( page );
  SetVisiblePage( page^=1 );
  if( wfr )
    WaitForRetrace();
  DisplayHorizont();
  o = stObject;
  while( o != NULL ) {
    if( o->Visible ) {
      tDefList *dl = o->DefList;
      if( o == Shadow ) {
	CalcShadowMatrix( &ShadowMatrix );
	MulMatrix( &Matrix, &ShadowMatrix, &ViewMatrix );
      } else
	MulMatrix( &Matrix, &o->Matrix, &ViewMatrix );
      TransformPoints( dl->Point, dl->Points, &Matrix );
      RunBsp( dl->BspRoot );
    }
    o = o->Next;
  }
  disp( FrameTime );
}

