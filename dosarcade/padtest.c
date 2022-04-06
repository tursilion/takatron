#include <stdio.h>
#include <stdlib.h>

int base;

struct {
        int b,y,a,x,l,r,select,start;
        int up,down,left,right;
        } snes;

void setlpt(int lpt_number);
void setbase(int port_number);
void readpad(int pad_number);
void main();

/************************************************************************/
// SNESpad class
// Version 1.0
// Kerry High

#define SNES_PWR (128+64+32+16+8)
#define SNES_CLK 1 // base+0
#define SNES_LAT 2 // base+0
#define SIN (negate?((inp(base+1)&snes_din)?1:0):((inp(base+1)&snes_din)?0:1))

// Set base by lpt#: 1=0x378 2=0x278 3=0x3BC
void setlpt(int lpt_number)
{
switch(lpt_number)
{
case 1: setbase(0x378); break;
case 2: setbase(0x278); break;
case 3: setbase(0x3bc); break;
default:
printf("SNESpad: LPT%d invalid!\n",lpt_number);
printf("Defaulting to LPT1\n");
setbase(0x378);
}
return;
}

// Set to any base
void setbase(int port_number)
{
base=port_number;
return;
}

// Read pad number pad_number
// Pads are numbered from 0 to 4
// Original C code by Earle F. Philhower, III.
void readpad(int pad_number)
{
const int DIN[5] = {64,32,16,8,128};
const int NEGATE[5]={0, 0, 0,0, 1};

int snes_din=DIN[pad_number];
int negate=NEGATE[pad_number];

outp(base, SNES_PWR+SNES_CLK); // Power up!
outp(base, SNES_PWR+SNES_LAT+SNES_CLK); // Latch it!

outp(base, SNES_PWR+SNES_CLK);
snes.b = SIN;
outp(base, SNES_PWR);
outp(base, SNES_PWR+SNES_CLK);
snes.y = SIN;
outp(base, SNES_PWR);
outp(base, SNES_PWR+SNES_CLK);
snes.select = SIN;
outp(base, SNES_PWR);
outp(base, SNES_PWR+SNES_CLK);
snes.start = SIN;
outp(base, SNES_PWR);
outp(base, SNES_PWR+SNES_CLK);
snes.up = SIN;
outp(base, SNES_PWR);
outp(base, SNES_PWR+SNES_CLK);
snes.down = SIN;
outp(base, SNES_PWR);
outp(base, SNES_PWR+SNES_CLK);
snes.left = SIN;
outp(base, SNES_PWR);
outp(base, SNES_PWR+SNES_CLK);
snes.right = SIN;
outp(base, SNES_PWR);
outp(base, SNES_PWR+SNES_CLK);
snes.a = SIN;
outp(base, SNES_PWR);
outp(base, SNES_PWR+SNES_CLK);
snes.x = SIN;
outp(base, SNES_PWR);
outp(base, SNES_PWR+SNES_CLK);
snes.l = SIN;
outp(base, SNES_PWR);
outp(base, SNES_PWR+SNES_CLK);
snes.r = SIN;
outp(base, 0); // Power it down
}

void main()
{
setlpt(1);
while (1) {
  readpad(0);
  printf("%d%d%d%d%d%d%d%d%d%d%d%d\n", snes.up, snes.down, snes.left, snes.right,
  snes.a, snes.b, snes.x, snes.y, snes.l, snes.r, snes.select, snes.start);
}
}
  
