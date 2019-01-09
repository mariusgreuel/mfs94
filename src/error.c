/* Modellflugsimulator V1.0
   Marius Greuel '94
   ERROR.C
*/

#include <stdio.h>
#include <stdlib.h>
#include "error.h"
#include "script.h"
#include "lib\_mpxcom.h"
#include "lib\_ega16.h"

#define MAXERRORS	50

extern char MSGMfs[];
static char ErrorFileName[] = "MFS.ERR";
static char MSGXErrors[] = "Warnmeldungen:   %u\nFehlermeldungen: %u\n";
static char MSGProgramAborted[] = "Programm abbgebrochen.\n";
static char *ErrorSource[] = {
  "Intern: ", "\x0", "Befehlszeile: ", "%s(%u) " };
static char *MSGDosError[] = { "**DOS-Fehler**", "\0",
  "Datei nicht gefunden\n",
  "Verzeichnis nicht gefunden\n",
  "Zu viele Dateien offen\n",
  "Zugriff verweigert\n" };
static char *MSGWarning[] = { "**Warnung** ",
  "RC-PC Interface nicht gefunden\n",
  "Zu viele Parameter\n", "Punkte nicht in einer Ebene\n"
};
static char *MSGError[] = { "**Fehler** ",
  "'%s' Falscher Befehl\n",
  "Ganzzahl erwartet\n", "Flieákommazahl erwartet\n", "Vektor erwartet\n",
  "Zeichenkette erwartet\n",
  "Ungltige Zeichenkette\n",
  "Farbe erwartet\n",
  "Attribut erwartet\n",
  "Ungltiges Polygon\n"
};
static char *MSGFatal[] = { "**Fataler Fehler** ",
  "Division durch Null !\n",
  "Zu viele Fehler aufgetreten !\n",
  "Timer-Initialisierung fehlgeschlagen !\n",
  "Keine VGA-Karte gefunden !\n",
  "Zu wenig Speicher vorhanden !\n",
  "Punkte Puffer bergelaufen !\n",
  "Polgon Puffer bergelaufen !\n",
  "Datei '%s' kann nicht ge”ffnet werden !\n",
  "Datei '%s' nicht gefunden !\n",
  "Fehler in Scriptdatei '%s' aufgetreten !\n",
};
static char **ErrorMSG[] = { MSGDosError, MSGWarning, MSGError, MSGFatal };

extern FILE *InFile;
extern FILE *OutFile;
extern tLocation Location;
static FILE *ErrorFile = NULL;
unsigned Warnings = 0, Errors = 0;

/****************************************************************************/

void ExitProgram( unsigned ExitCode )
/* beendet das Programm ordentlich */
{
  ExitRC();
  ExitGraph();
  if( Warnings != 0 || Errors != 0 ) {
    if( ErrorFile )
      fprintf( ErrorFile, MSGXErrors, Warnings, Errors );
    if( ExitCode == EXIT_FATAL ) {
      fprintf( stderr, MSGProgramAborted );
      if( ErrorFile )
	fprintf( ErrorFile, MSGProgramAborted );
    }
  }
  if( InFile )
    fclose( InFile );
  if( OutFile )
    fclose( OutFile );
  if( ErrorFile )
    fclose( ErrorFile );
  exit( ExitCode );
}

void RunError( unsigned ErrorCode, unsigned Source, void *OptPara )
/* Laufzeitfehlerbehandlung */
{
  unsigned Type, Error;

  Type = ErrorCode >> TYPESHIFT;
  Error = ErrorCode & ( 1 << TYPESHIFT ) - 1;
  if( Type == WARNING )
    ++Warnings;
  else
    ++Errors;
  if( Type == FATAL )
    ExitGraph();
  fprintf( stderr, *ErrorMSG[ Type ] );
  fprintf( stderr, ErrorSource[ Source ], Location.Name, Location.Line );
  fprintf( stderr, ErrorMSG[ Type ][ Error ], OptPara );
  if( !ErrorFile ) {
    ErrorFile = fopen( ErrorFileName, "w" );
    if( ErrorFile )
      fprintf( ErrorFile, MSGMfs );
  }
  if( ErrorFile ) {
    fprintf( ErrorFile, *ErrorMSG[ Type ], Errors, Warnings );
    fprintf( ErrorFile, ErrorSource[ Source ], Location.Name, Location.Line );
    fprintf( ErrorFile, ErrorMSG[ Type ][ Error ], OptPara );
  }
  if( Type == FATAL ) {
    ExitProgram( EXIT_FATAL );
  }
  if( Errors >= MAXERRORS )
    RunError( FATTooManyErrors, EXTERN, NULL );
}
