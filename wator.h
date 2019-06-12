#include <stdlib.h>
#include <stdio.h>
#include "oslib/osbyte.h"
#include "oslib/os.h"

extern int  wWidth, wHeight;
extern int  noShark, noFish, noTicks;
extern char *screen;
extern int  currentBank;
extern int  screenS;
extern int  resIndices[];

extern int reproduceCFish;
extern int reproduceCShark;
extern int initialEnergyShark;

extern const char tFish;
extern const char tShark;

const char tFish   = 1;
const char tShark  = 2;

typedef struct _world {
  int width;
  int height;
  int size;
  int *canvas;
} world;


void initScreen(void);
void gotoxy(unsigned char x, unsigned char y);

