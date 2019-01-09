/* Modellflugsimulator V1.0
   Marius Greuel '94
   TYPES.H
*/

#define TRUE		1
#define FALSE		0
#define X		0
#define Y		1
#define Z		2
#define H		3

#ifndef NULL
  #if defined(__TINY__) || defined(__SMALL__) || defined(__MEDIUM__)
    #define NULL 0
  #else
    #define NULL 0L
  #endif
#endif

typedef float tP2D[2];				     /* 2D-Punkt oder Pixel */
typedef float tP3D[3];						/* 3D-Punkt */
typedef float tVector[3];					  /* Vektor */
typedef float tHVector[4];				/* homogener Vektor */
typedef float tMatrix[4][4];			     /* homogene 4*4 Matrix */
typedef int tColor;

typedef enum Region {					     /* Regioncodes */
  BACK = 1, LEFT = 2, RIGHT = 4, TOP = 8, BOTTOM = 16
}tRegion;

typedef enum PAttribut {				/* Polygonattribute */
  DRAW = 1,					       /* Umrandung zeichen */
  FILL = 2,						  /* Polygon fllen */
  TWO_SIDES = 4,			   /* Polygon mit gleiche Rckseite */
  BACK_FACE = 8,			 /* Polygon mit dunklerer Rckseite */
  LINE = 16,			       /* 1. und 2. Punkt als Linie zeichen */
  DUMMY = 32		      /* unsichtbares Polygon fr den BSP-Generator */
}tPAttribut;

typedef struct Point {
  tP3D op;						     /* Objektpunkt */
  tP3D tp;					      /* transformierter op */
  tP2D pp;						 /* projezierter tp */
  int Region;						 /* Lage des Punkte */
}tPoint;

typedef struct Polygon {
  struct     Polygon *Front;	     /* Zeiger auf alle Polygone vor diesem */
  struct     Polygon *Back;	    /* Zeiger auf alle Polygone nach diesem */
  tPAttribut Attribut;				   /* Attribut frs Polygon */
  tColor     Color;			/* ursprngliche Farbe des Polygons */
  tColor     RealColor;			    /* Farbe des Polygons mit Licht */
  unsigned   Points;					   /* Anzahl Punkte */
  tPoint     *Point[20];		     /* Zeiger auf bis zu 5 Punkten */
}tPolygon;

typedef struct DefList {
  tPolygon *BspRoot;		    /* Zeiger auf den BSP-Baum (1. Polygon) */
  unsigned Points;					   /* Anzahl Punkte */
  tPoint   *Point;				/* Zeiger auf Punktetabelle */
  unsigned Polygons;					 /* Anzahl Polygone */
  tPolygon *Polygon;			       /* Zeiger auf Polygontabelle */
}tDefList;

typedef struct Object {
  struct   Object *Next;		      /* Zeiger auf n„chstes Objekt */
  tDefList *DefList;			     /* Zeiger auf Definitionsliste */
  tMatrix  Matrix;		     /* Transformationsmatrix -> Objektlage */
  int      Visible;					 /* Objekt sichtbar */
  int      Modified;					 /* Objekt ge„ndert */
}tObject;

