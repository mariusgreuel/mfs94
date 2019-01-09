/* Modellflugsimulator V1.0
   Marius Greuel '94
   ERROR.H
*/

#define TYPESHIFT	8               /* 2^8 Versatz zwischen Fehlertypen */

enum ErrorTypes { DOS_ERROR, WARNING, ERROR, FATAL };
enum ErrorSources { INTERNAL, EXTERN, COMMAND_LINE, IN_FILE };
enum DosErrors { _DOS_ERROR = DOS_ERROR << TYPESHIFT };
enum Warnings { _WARNING = WARNING << TYPESHIFT,
  WARRcNotFound,	WARExtraParameter,
  WARPointsNotPlane
};
enum Errors { _ERROR = ERROR << TYPESHIFT,
  ERRBadCommand,
  ERRIntExpected,	ERRRealExpected,	ERRVectorExpected,
  ERRStringExpected,	ERRBadString,
  ERRColorExpected,	ERRAttributExpected,
  ERRBadPolygon
};
enum Fatals { _FATAL = FATAL << TYPESHIFT,
  FATDivisionByZero,
  FATTooManyErrors,	FATTimerError,		FATGraphicError,
  FATMemoryError,	FATPointBufferExceeded,	FATPolygonBufferExceeded,
  FATCantOpenFile,	FATFileNotFound,	FATErrorInScriptFile
};
enum ExitCodes { EXIT_OK, EXIT_ON_BREAK, EXIT_ERROR, EXIT_FATAL };

void RunError( unsigned ErrorCode, unsigned Source, void *OptPara );
void ExitProgram( unsigned ExitCode );
