/* Modellflugsimulator V1.0
   Marius Greuel '94
   SCRIPT.C
   Scriptdatei abarbeiten
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <graphics.h>
#include "types.h"
#include "error.h"
#include "memory.h"
#include "script.h"
#include "object.h"

#define PATHLEN		128
#define TEXTLEN		255

typedef struct Token
{
  unsigned char *Name;
  int Arg;
}tToken;

typedef struct FToken
{
  unsigned char *Name;
  int (*Do)( unsigned char **Line );
}tFToken;

static int CommandSet( unsigned char **Line );
static int CommandDefine( unsigned char **Line );
static int CommandObject( unsigned char **Line );
static int CommandMessage( unsigned char **Line );
static int CommandInclude( unsigned char **Line );
static int CommandEnd( unsigned char **Line );

static int DefinePoint( unsigned char **Line );
static int DefinePolygon( unsigned char **Line );
static int DefineLine( unsigned char **Line );
static int DefineMoveAbsolute( unsigned char **Line );
static int DefineMoveRelative( unsigned char **Line );
static int DefineEnd( unsigned char **Line );

void SkipWhiteSpace( unsigned char **Line );
int SplitLine( unsigned char **Line, unsigned char *Split );
int SearchToken( tToken Token[], unsigned char *Word );
void DecodeLine( unsigned char **Line );

static tFToken Command[] = {
  "SET",        CommandSet,
  "DEFINE",     CommandDefine,
  "OBJECT",	CommandObject,
  "ECHO",       CommandMessage,
  "INCLUDE",	CommandInclude,
  "END",        CommandEnd,
  "",           NULL
};
static tFToken Define[] = {
  "POINT",      DefinePoint,
  "POLYGON",    DefinePolygon,
  "LINE",       DefineLine,
  "MOVEA",      DefineMoveAbsolute,
  "MOVER",      DefineMoveRelative,
  "END",        DefineEnd,
  "",           NULL
};

static tToken Color[] = {
  "BLACK", BLACK, "BLUE", BLUE,	"GREEN", GREEN,	"CYAN", CYAN, "RED", RED,
  "MAGENTA", MAGENTA, "BROWN", BROWN, "LGRAY", LIGHTGRAY, "GRAY", DARKGRAY,
  "LBLUE", LIGHTBLUE, "LGREEN", LIGHTGREEN, "LCYAN", LIGHTCYAN,
  "LRED", LIGHTRED, "LMAGENTA", LIGHTMAGENTA, "YELLOW", YELLOW, "WHITE", WHITE
};

static tToken PolygonAttribut[] = {
  "DRAW", DRAW, "FILL", FILL, "TWOSIDES", TWO_SIDES, "BACKFACE", BACK_FACE,
  "DUMMY", DUMMY
};

extern unsigned char NulString[];
extern int Errors;
FILE *InFile   = NULL;
FILE *OutFile  = NULL;
tLocation Location = { 0, NulString, NulString };
static tFToken *TokenSet = Command;
static tVector RelativeMove = { 0.0, 0.0, 0.0 };

/****************************************************************************/

/* berspringt Leerzeichen, Tabs usw. */
static void SkipWhiteSpace( unsigned char **Line )
{
  register unsigned char *Text = *Line;

  while( *Text ) {
    if( *Text > ' ' )
      break;
    Text++;
  }
  if( *Text == ';' )
    *Line = NulString;
  else
    *Line = Text;
}

/* bricht von einer Zeile ein Wort ab */
static int SplitLine( unsigned char **Line, unsigned char *Split )
{
  register unsigned char *Text;
  unsigned char *Word;
  int Size;

  SkipWhiteSpace( Line );
  Text = *Line;
  Word = Text;
  while( *Text >= '0')
    Text++;
  Size = Text-Word;
  *Line = Text;
  strncpy( Split, Word, Size );
  Split[ Size ] = 0;
  return Size;
}

/* sucht in einer Tabelle nach einem Wort */
static int SearchToken( tToken *Token, unsigned char *Word )
{
  int i=0, t=-1;

  strupr( Word );
  while( Token[i].Name[0] ) {
    if( strcmp( Word, Token[i].Name ) == 0 ) {
      t = i;
      break;
    }
    i++;
  }
  return( t );
}

/* decodiert eine Zeile */
static void DecodeLine( unsigned char **Line )
{
  int t, r;
  unsigned char Word[ TEXTLEN ];

  if( !SplitLine( Line, Word ))
    return;
  t = SearchToken( (tToken*)TokenSet, Word );
  if( t == -1 ) {
    RunError( ERRBadCommand, IN_FILE, Word );
    return;
  }
  TokenSet[ t ].Do( Line );
/*  if( r ) {
    SkipWhiteSpace( Line );
    if( **Line )
      RunError( WARExtraParameter, IN_FILE, NULL );
  }*/
}

/* decodiert eine Scriptdatei */
int DecodeFile( unsigned char *Name, int Source )
{
  FILE *InFile;
  tLocation OldLoc;
  unsigned char *Text;

  InFile = fopen( Name, "r" );
  if( InFile == NULL )
    RunError( FATFileNotFound, Source, Name );
  OldLoc = Location;
  Location.Text = AllocMemory( TEXTLEN+1 );
  Location.Name = AllocMemory( PATHLEN+1 );
  Location.Line = 0;
  strcpy( Location.Name, Name );
  while( fgets( Location.Text, TEXTLEN, InFile ) != NULL )
  {
    Location.Line++;
    Text = Location.Text;
    DecodeLine( &Text );
  }
  FreeMemory( Location.Name );
  FreeMemory( Location.Text );
  Location = OldLoc;
  fclose( InFile );
  return Errors == 0 ? TRUE : FALSE;
}

/****************************************************************************/

static void SkipSeperator( unsigned char **Line )
{
  SkipWhiteSpace( Line );
  if( **Line == ',' )
    ++*Line;
}

static int GetInt( unsigned char **Line )
{
  unsigned char Word[ TEXTLEN ];

  if( !SplitLine( Line, Word ))
    return -1;
  else
    return atoi( Word );
}

static float GetReal( unsigned char **Line )
{
  unsigned char Word[ TEXTLEN ];
  float f;

  if( !SplitLine( Line, Word )) {
    RunError( ERRRealExpected, IN_FILE, NULL );
    return -1;
  } else
    return atof( Word );
}

/* liefert einen String ( eingeschlossen in "" ) zurck */
static int GetString( unsigned char **Line, unsigned char *String )
{
  register unsigned char *Text;
  unsigned char *Word;
  int Size;

  SkipWhiteSpace( Line );
  Text = *Line;
  if( *Text != '"' ) {
    RunError( ERRStringExpected, IN_FILE, NULL );
    return FALSE;
  }
  Text++;
  Word = Text;
  while( *Text >= ' ' ) {
    if( *Text == '"' )
      break;
    Text++;
  }
  if( *Text != '"' ) {
    RunError( ERRBadString, IN_FILE, NULL );
    return FALSE;
  }
  Size = Text-Word;
  Text++;
  *Line = Text;
  if( !Size ) {
    RunError( ERRBadString, IN_FILE, NULL );
    return FALSE;
  }
  strncpy( String, Word, Size );
  String[ Size ] = 0;
  return Size;
}

static int GetColor( unsigned char **Line )
{
  unsigned char Word[ TEXTLEN ];
  int t;

  if( !SplitLine( Line, Word )) {
    RunError( ERRColorExpected, IN_FILE, NULL );
    return -1;
  }
  t = SearchToken( Color, Word );
  if( t == -1 ) {
    RunError( ERRColorExpected, IN_FILE, NULL );
    return -1;
  }
  return Color[t].Arg;
}

static int GetPAttr( unsigned char **Line )
{
  unsigned char Word[ TEXTLEN ];
  int t, Attribut = 0;

  do {
    if( !SplitLine( Line, Word )) {
      RunError( ERRAttributExpected, IN_FILE, NULL );
      return -1;
    }
    t = SearchToken( PolygonAttribut, Word );
    if( t == -1 ) {
      RunError( ERRAttributExpected, IN_FILE, NULL );
      return -1;
    }
    Attribut |= PolygonAttribut[t].Arg;
    SkipWhiteSpace( Line );
    if( **Line == '+' ) {
      (*Line)++;
      t = -1;
    }
  }
  while( t == -1 );
  return Attribut;
}

static int GetVector( tVector *v, char *Text )
{
  int k;
  float x, y, z;

  k = sscanf( Text, "%f,%f,%f", &x, &y, &z );
  if ( k != 3 ) {
    RunError( ERRVectorExpected, IN_FILE, NULL );
    return FALSE;
  }
  (*v)[X] = x;
  (*v)[Y] = y;
  (*v)[Z] = z;
  return( TRUE );
}

/****************************************************************************/

static int DefinePoint( unsigned char **Line )
{
  tVector v;
  if( GetVector( &v, *Line ) == FALSE )
    return FALSE;
  v[X] += RelativeMove[X];
  v[Y] += RelativeMove[Y];
  v[Z] += RelativeMove[Z];
  StorePoint( &v );
  return TRUE;
}

static int DefinePolygon( unsigned char **Line )
{
  int Color, Attribut, pts, i;
  int pt[20];

  Color = GetColor( Line );
  if( Color == -1 )
    return FALSE;
  SkipSeperator( Line );
  Attribut = GetPAttr( Line );
  if( Attribut == -1 )
    return FALSE;
  SkipSeperator( Line );
  pts=0;
  do
  {
    i = GetInt( Line );
    if( i!=-1 ){
      pt[pts] = i;
      pts++;
    }
    SkipSeperator( Line );
  }
  while( i!=-1);
  StorePolygon( pts, pt, Color, Attribut );
  return TRUE;
}

static int DefineLine( unsigned char **Line )
{
  int Color, p, i;
  int pt[20];

  Color = GetColor( Line );
  if( Color == -1 )
    return FALSE;
  SkipSeperator( Line );
  for( p=0; p<3; p++ ) {
    i = GetInt( Line );
    if( i!=-1 )
      pt[p] = i;
    SkipSeperator( Line );
  }
  StorePolygon( p, pt, Color, LINE );
  return TRUE;
}

static int DefineMoveAbsolute( unsigned char **Line )
{
  return GetVector( &RelativeMove, *Line );
}

static int DefineMoveRelative( unsigned char **Line )
{
  tVector v;

  if(!GetVector( &v, *Line ))
    return FALSE;
  RelativeMove[X] += v[X];
  RelativeMove[Y] += v[Y];
  RelativeMove[Z] += v[Z];
  return TRUE;
}

static int CommandDefine()
{
  CreateDefinition();
  TokenSet = Define;
  return TRUE;
}

static int DefineEnd()
{
  CloseDefinition();
  TokenSet = Command;
  return TRUE;
}

/****************************************************************************/

static int CommandBadToken()
{
  RunError( ERRBadCommand, IN_FILE, NULL );
  return FALSE;
}

static int CommandSet()
{
  return TRUE;
}

static int CommandMessage( unsigned char **Line )
{
  unsigned char String[ TEXTLEN ];

  if( !GetString( Line, String ))
    return FALSE;
  fprintf( stdout, String );
  fprintf( stdout, "\n" );
  return TRUE;
}

static int CommandInclude( unsigned char **Line )
{
  unsigned char FilePath[ TEXTLEN ];

  if( !GetString( Line, FilePath ))
    return FALSE;
  DecodeFile( FilePath, IN_FILE );
  return TRUE;
}

static int CommandObject( unsigned char ** Line )
{
  tVector v;

  if(!GetVector( &v, *Line ))
    return FALSE;
  CreateObject( &v );
  return TRUE;
}

static int CommandEnd()
{
  return TRUE;
}

