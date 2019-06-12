/*

wator.c

a fast implementation of the wa-tor simulation for RISC OS,
see http://home.cc.gatech.edu/biocs1/uploads/2/wator_dewdney.pdf

caution! for performance reasons, this thing uses the framebuffer
directly. it runs fine on my raspberry pi running risc os 5.24,
your mileage may vary...

licensed under MIT license, see LICENCE for details.


*/

#include "wator.h"
#include "oslib/osbyte.h"
#include <string.h>

int  wWidth, wHeight;
int noFish, noShark, noTicks;
char *screen;
int initialEnergyShark;
int reproduceCFish;
int reproduceCShark;
int resIndices[8];

void resetCanvas(char col);
void initScreen(void);
void fastPlot(int x,int y,char col);
void gotoxy(unsigned char x, unsigned char y);
char colourAtPos(int x, int y);
void drawWorld(world* theWorld);

world *newWorld(int width, int height);
void initWorld(world* theWorld, int numFish, int numSharks);

int newElement(int eType, int energy, int breedCountdown);

inline int breedCountdownOfElement(int element);
inline int energyOfElement(int element);
inline int* getElementAtPos(world* theWorld, int x, int y);
inline void setElementAtPos(world* theWorld, int x, int y, int theSpecies);
inline int isDirty(int* element);
inline void clearDirty(int* element);
inline char isFreePosition(world* theWorld, int x, int y);
int randomNeighborOfType(world* theWorld, int destType, int x, int y);

inline void doFish(int theFish, world *theWorld, int x, int y);
inline void doShark(int theShark, world *theWorld, int x, int y);
int doWorldTick(world* theWorld);

void initScreen(void)
{
  int vduVars[3];

  os_vdu_var_list var_list[3] = {
                  os_VDUVAR_DISPLAY_START,
                  os_VDUVAR_TOTAL_SCREEN_SIZE,
                  -1};

  osscreenmode_select_mode_with_mode_string("X320 Y256 C256");
  os_read_vdu_variables((os_vdu_var_list*)&var_list,vduVars);
  screen = (char*)vduVars[0];

  palettev_set_entry( 0,16,0x00000000,0x0);
  palettev_set_entry( 1,16,0x00ff0000,0x0);
  palettev_set_entry( 2,16,0x0000ff00,0x0);

  wWidth=320;
  wHeight=256;

}


world *newWorld(int width, int height) {
  int *newWorldCanvas;
  world *theNewWorld;
  int i;
  newWorldCanvas = (int*) malloc(sizeof(int)*width*height);
  for (i=0;i<(width*height);i++) {
    newWorldCanvas[i] = 0;
  }
  theNewWorld = (world*) malloc(sizeof(world));
  theNewWorld->width = width;
  theNewWorld->height = height;
  theNewWorld->size = width*height;
  theNewWorld->canvas = newWorldCanvas;
  return theNewWorld;
}

void initWorld(world* theWorld, int numFish, int numSharks) {
  int i, x, y;
  for (i=0; i<theWorld->size; i++) {
    theWorld->canvas[i] = 0;
  }
  for (i=0; i<numFish; i++) {
    do {
      x=rand()%theWorld->width;
      y=rand()%theWorld->height;
    } while (!isFreePosition(theWorld,x,y));
    setElementAtPos(theWorld,x,y,newElement(tFish,0,(rand()%reproduceCFish)) | 4);
  }
  for (i=0; i<numSharks; i++) {
    do {
      x=rand()%theWorld->width;
      y=rand()%theWorld->height;
    } while (!isFreePosition(theWorld,x,y));
    setElementAtPos(theWorld,x,y,newElement(tShark,(rand()%initialEnergyShark),(rand()%reproduceCShark)) | 4);
  }
}

int isDirty(int* element) {
  return ((*element) & 4);
}

void clearDirty(int* element) {
  *element &= ~4;
}

int newElement(int eType, int energy, int breedCountdown) {
  int e = eType;
  e = e | (energy<<8);
  e = e | (breedCountdown<<16);
  return e;
}

int energyOfElement(int element) {
  return (255 & (element>>8));
}

int breedCountdownOfElement(int element) {
  return (255 & (element>>16));
}


void drawWorld(world* theWorld) {
  register int *currentS;
  register int i;
  for (i=0;i<theWorld->size;i++) {
    currentS = &theWorld->canvas[i];
//     if (isDirty(currentS)) {
      clearDirty(currentS);
      *(screen+i)=*currentS & 3;
//    }
  }
}

int randomNeighborOfType(world* theWorld, int destType, int x, int y) {
  int xc,yc,xt,yt;
  int testElement;
  int resCount;
  int mx = theWorld->width-1;
  int my = theWorld->height-1;
  int idx;
  resCount = 0;
  for (xc=-1;xc<=1;xc++) {
    for (yc=-1;yc<=1;yc++) {
      xt = x + xc;
      yt = y + yc;
      if (xt<0) xt=mx;
      if (yt<0) yt=my;
      if (xt>mx) xt=0;
      if (yt>my) yt=0;
      idx = xt+(theWorld->width*yt);
      testElement = theWorld->canvas[idx];
      if ((testElement & 3) == destType) {
        resIndices[resCount++] = idx;
      }
    }
  }
  if (resCount>0) {
    int result = resIndices[rand()%resCount];
    return result;
  }
  return -1;
}

void setElementAtPos(world* theWorld, int x, int y, int theSpecies) {
  theWorld->canvas[x+(theWorld->width*y)] = theSpecies;
}

int* getElementAtPos(world* theWorld, int x, int y) {
  return &(theWorld->canvas[x+(theWorld->width*y)]);
}

char isFreePosition(world* theWorld, int x, int y) {
  return (0==(*getElementAtPos(theWorld,x,y))&3);
}

void doFish(int theFish, world *theWorld, int x, int y) {
  int breedC;
  int newPos;
  int replacementElement = 4;
  int updatedFish;

  breedC = breedCountdownOfElement(theFish)-1;

  if (breedC<=0) {
    breedC=reproduceCFish;
    replacementElement = newElement(tFish,255,(rand()%reproduceCFish)) | 4;
  }

  newPos = randomNeighborOfType(theWorld,0,x,y);

  if (newPos>0) {
    updatedFish = newElement(tFish,128,breedC);
    theWorld->canvas[x+(theWorld->width*y)] = replacementElement;
    theWorld->canvas[newPos] = updatedFish | 4;
  }

}

void doShark(int theShark, world *theWorld, int x, int y) {
  int breedC;
  int energy;
  int newPos;
  int replacementElement = 4;
  int updatedFish;

  breedC = breedCountdownOfElement(theShark)-1;
  energy = energyOfElement(theShark)-1;

  if (energy<=0) {
    theWorld->canvas[x+(theWorld->width*y)] = 4;
    return;
  }

  if (breedC<=0) {
    breedC=reproduceCShark;
    replacementElement = newElement(tShark,(rand()%initialEnergyShark),(rand()%reproduceCShark)) | 4;
  }

  newPos = randomNeighborOfType(theWorld,tFish,x,y);
  if (newPos>0) {
    energy+=2;
    if (energy>255) energy=255;
    updatedFish = newElement(tShark,energy,breedC);
    theWorld->canvas[x+(theWorld->width*y)] = replacementElement;
    theWorld->canvas[newPos] = updatedFish | 4;
    return;
  }

  newPos = randomNeighborOfType(theWorld,0,x,y);
  if (newPos>0) {
    updatedFish = newElement(tShark,energy,breedC);
    theWorld->canvas[x+(theWorld->width*y)] = replacementElement;
    theWorld->canvas[newPos] = updatedFish | 4;
  }

}

int doWorldTick(world* theWorld) {
  register int x,y;
  int currentElement;
  noFish=0;
  noShark=0;
  noTicks++;
  for (x=0;x<theWorld->width;x++) {
    for (y=0;y<theWorld->height;y++) {
      currentElement = theWorld->canvas[x+((theWorld->width)*y)];
      if (isDirty(&currentElement)) {
        continue;
      }
      if ((currentElement & 3) == tFish) {
        noFish++;
        doFish(currentElement,theWorld,x,y);
      } else if ((currentElement & 3) == tShark) {
        noShark++;
        doShark(currentElement,theWorld,x,y);
      }
    }
  }

  return (noShark!=0) && (noFish!=0);
}

void resetCanvas(char col) {
  int x,y;
  for (x=0;x<wWidth;x++) {
    for (y=0;y<wHeight;y++) {
      fastPlot(x,y,col);
    }
  }
}

void fastPlot(int x,int y,char col) {
  *(screen+x+(wWidth*y))=col;
}

char colourAtPos(int x, int y) {
  return *(screen+x+(wWidth*y));
}

void gotoxy(unsigned char x, unsigned char y) {
  os_writec(31); os_writec(x); os_writec(y);
}

int main (void)
{
   world *theWorld;
   char input[80];
   int nFish;
   int nSharks;

   initScreen();
   theWorld = newWorld(wWidth,wHeight);
   printf("** K-Wa-Tor **\n");
   printf("   written by S. Kleinert @ K-Burg, 2015\n");
   printf("   with thanks to Frau K. and Buba K.\n\n");
   printf("screen at         0x%x\n",(int)screen);
   printf("new world at      0x%x\n",(int)theWorld);
   printf("world size is     %d\n\n",wWidth*wHeight);
   printf("number of fish   :");
   scanf("%s",input);
   nFish = atoi(input);
   printf("number of sharks :");
   scanf("%s",input);
   nSharks = atoi(input);
   printf("initial shark energy:");
   scanf("%s",input); initialEnergyShark = atoi(input);
   printf("fish reproduce ticks:");
   scanf("%s",input); reproduceCFish = atoi(input);
   printf("shark reproduce ticks:");
   scanf("%s",input); reproduceCShark = atoi(input);
   initWorld(theWorld,nFish,nSharks);
   os_vdu_on();
   os_writec(16);
   os_writec(23);os_writec(1);os_writec(0);os_writec(0);os_writec(0);
   os_writec(0);os_writec(0);os_writec(0);os_writec(0);
   gotoxy(0,0);
   noTicks=0;
   drawWorld(theWorld);
   while (doWorldTick(theWorld)) {
     drawWorld(theWorld);
   }
   gotoxy(0,0); printf("#%d - S:%d F:%d",noTicks,noShark,noFish);
   return 0;
}
