/* Funktionsprototypen
   _MPXCOM.ASM
   Marius Greuel '94
*/

#define RC_ONLY_PULSE	1
#define RC_NEG_PULSE	2
#define RC_SHOW_PULSE	4
#define RC_STD_MODE	0
#define RC_MIN_PW	500
#define RC_MAX_PW	2500
#define RC_MIN_PW_DIFF	500
#define RC_STD_SHORT_PW	1050
#define RC_STD_LONG_PW	2150
#define RC_NO_ERROR     0
#define RC_NOT_ENABLED	1
#define RC_NOT_FOUND	2
#define RC_NO_DATA	4

extern int far InitRC( void );
extern void far ExitRC( void );
extern void far ZeroRCSticks( void );
extern void far SetRCTiming( int mode, int short_pw, int long_pw );
extern int far GetRCStick( int nr );
extern int far GetRCState( void );
extern int far GetRCPort( void );
extern char far GetRCIRQ( void );


