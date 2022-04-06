	// include libraries
#include <libps.h>
#include "pad.h"
#include <stdio.h>
#include <sys/file.h>

	// constants

	// GPU packet space
#define PACKETMAX		(10000)
#define PACKETMAX2		(PACKETMAX*24)

	// size of ordering table: 2 << OT_LENGTH
	// i.e.	16384 levels of z resolution
#define OT_LENGTH		(14)	  


	// globals

	// Ordering Table handlers
GsOT		WorldOrderingTable[2];
	// actual Ordering Tables themselves							
GsOT_TAG	zSortTable[2][1<<OT_LENGTH];   
	// GPU packet work area
PACKET		GpuOutputPacket[2][PACKETMAX2];


/* PSX Takatron Stuff */

#define BUFFER 0x80080000

#define FILE int

#define OTHER 0
#define SPRITE 1

#define SAMP 2
#define MIDI 3

#define NUM_DATA 163
#define NUM_SPRITE 128
#define NUM_MIDI 3
#define NUM_SAMP 28

/* file sizes */
#define DATA_DAT 1304
#define SPRITE_DAT 4096
#define TT1A_TIM 131092
#define TT1B_TIM 64020
#define TT1C_TIM 129556
#define TT1D_TIM 65800
#define TT1E_TIM 131092

#define SPRITEMAX 300             /* This should be more than enough */     
GsSPRITE Sprites[SPRITEMAX];           


struct {
	int type,number;
       } data[NUM_DATA];

struct {
        int tex, x,y, w,h, rotation,sx,sy;
       } sprite[NUM_SPRITE];

struct {
	void *address;
       } sample[NUM_SAMP];

struct {
        void *address;
       } midi[NUM_MIDI];

int tex_x[6],tex_y[6];
GsIMAGE TexInfo[6];

int i;
char buf[80];

	// main function
	// see pad.h and pad.c for the controller pad interface

FILE in;
int i,x,y,x1,y1,d,w,h,r,sx,sy,dn,sn;
int spnum;
char buf[80],inp[80];

int main (void)
{
	u_long PadStatus = 0;		// state of controller pad
        u_long oldpad=0;
	int outputBufferIndex;		// which buffer is active (drawn on)

        
		// initialisation						
        SetVideoMode( MODE_NTSC );      // NTSC mode
        dn=sn=0;
        spnum=18;
        Sprites[1].x=Sprites[1].y=50;

		// graphical initialisation:
                // screen resolution 320 by 240,
		// non-interlace, use GsGPU offset
	ResetGraph(0);
        GsInitGraph(640 ,480, GsOFSGPU|GsNONINTER, 1, 0);                
        GsDefDispBuff(0, 0, 0, 0);

  printf("read data\n");
  in=open("pcdrv:data.dat",O_RDONLY);
  if (in)
  { read(in,(char*)(data),DATA_DAT);
    close(in);
    printf("good.\n");
  }
  printf("read sprite\n");
  in=open("pcdrv:sprite.dat",O_RDONLY);
  if (in)
  { read(in,(char*)(sprite),SPRITE_DAT);
    close(in);
    printf("good.\n");
  }

  printf("read image1\n");
  in=open("pcdrv:tt1a.tim",O_RDONLY);
  if (in)
  { read(in,(char*)BUFFER,TT1A_TIM);
    close(in);
    printf("good.\n");
    initTexture((u_long *)BUFFER,1);
  }

  printf("read image2\n");
  in=open("pcdrv:tt1b.tim",O_RDONLY);
  if (in)
  { read(in,(char*)BUFFER,TT1B_TIM);
    close(in);
    printf("good.\n");
    initTexture((u_long *)BUFFER,2);
  }

  printf("read image3\n");
  in=open("pcdrv:tt1c.tim",O_RDONLY);
  if (in)
  { read(in,(char*)BUFFER,TT1C_TIM);
    close(in);
    printf("good.\n");
    initTexture((u_long *)BUFFER,3);
  }
  printf("read image4\n");
  in=open("pcdrv:tt1d.tim",O_RDONLY);
  if (in)
  { read(in,(char*)BUFFER,TT1D_TIM);
    close(in);
    printf("good.\n");
    initTexture((u_long *)BUFFER,4);
  }
  printf("read image5\n");
  in=open("pcdrv:tt1e.tim",O_RDONLY);
  if (in)
  { read(in,(char*)BUFFER,TT1E_TIM);
    close(in);
    printf("good.\n");
    initTexture((u_long *)BUFFER,5);
  }

		// set up the controller pad
	PadInit();

		// set up the ordering table handlers
	WorldOrderingTable[0].length = OT_LENGTH;
	WorldOrderingTable[1].length = OT_LENGTH;
	WorldOrderingTable[0].org = zSortTable[0];
	WorldOrderingTable[1].org = zSortTable[1];

        initSprite();

        i=0;

	for (;;)			// main loop
		{
			// find status of controller pad
		PadStatus = PadRead();

                if (oldpad==PadStatus) PadStatus&=(PADLup|PADLdown|PADLright|PADLleft);
                else oldpad=PadStatus;

			// if 'select' pressed, exit main loop
		if (PadStatus & PADselect)
			break;

                if (PadStatus & PADLup)
                  Sprites[1].y--;

                if (PadStatus & PADLdown)
                  Sprites[1].y++;

                if (PadStatus & PADLright)
                  Sprites[1].x++;

                if (PadStatus & PADLleft)
                  Sprites[1].x--;

                if ((PadStatus & PADRdown)&&(spnum>0))
                { spnum--;
                  printf("Sprite %d, (%d, %d), %d x %d, Rot: %d, Scale %d x %d\n",spnum,sprite[spnum].x,sprite[spnum].y,sprite[spnum].w,sprite[spnum].h,sprite[spnum].rotation,sprite[spnum].sx,sprite[spnum].sy);
                  initSprite();
                }

                if ((PadStatus & PADRup)&&(spnum<NUM_SPRITE-1))
                { spnum++;
                  printf("Sprite %d, (%d, %d), %d x %d, Rot: %d, Scale %d x %d\n",spnum,sprite[spnum].x,sprite[spnum].y,sprite[spnum].w,sprite[spnum].h,sprite[spnum].rotation,sprite[spnum].sx,sprite[spnum].sy);
                  initSprite();
                }

                	// find which buffer is active
		outputBufferIndex = GsGetActiveBuff();

			// set address for GPU scratchpad area
		GsSetWorkBase( (PACKET*)GpuOutputPacket[outputBufferIndex]);

			// clear the ordering table
		GsClearOt(0, 0, &WorldOrderingTable[outputBufferIndex]);

                GsSortSprite(&Sprites[1], &WorldOrderingTable[outputBufferIndex], 1);

			// wait for end of drawing
		DrawSync(0);

			// wait for V_BLANK interrupt
		VSync(0);

			// swap double buffers
		GsSwapDispBuff();

			// register clear-command: clear to black
		GsSortClear(0x0, 0x0, 0x0,
				&WorldOrderingTable[outputBufferIndex]);

			// register request to draw ordering table
		GsDrawOt(&WorldOrderingTable[outputBufferIndex]);
       
              }


		// clean up
	ResetGraph(3);
	return 0;
}

initTexture(addr,data)
u_long *addr;
int data;

{ RECT rect1;

  addr++;

  GsGetTimInfo(addr, &TexInfo[data]);

  rect1.x=TexInfo[data].px;
  rect1.y=TexInfo[data].py;
  rect1.w=TexInfo[data].pw;
  rect1.h=TexInfo[data].ph;

  tex_x[data]=rect1.x;
  tex_y[data]=rect1.y;

  printf("Data %d at %d, %d\n",data,rect1.x,rect1.y);

  LoadImage(&rect1,TexInfo[data].pixel);
  /* dma xfer from memory to videoram */

  DrawSync(0);
  /* wait for finish */
}


initSprite()
{
                int i;
                u_long colorMode;
                u_short tPage;
                
                colorMode = TexInfo[sprite[spnum].tex].pmode&0x03;

                tPage = GetTPage(colorMode , 0, tex_x[sprite[spnum].tex], tex_y[sprite[spnum].tex]);
                       
                Sprites[1].attribute = (colorMode<<24);
                Sprites[1].w = sprite[spnum].w;              
                Sprites[1].h = sprite[spnum].h;
                
                Sprites[1].mx = sprite[spnum].w/2;       /* center f. rotation/scaling */
                Sprites[1].my = sprite[spnum].h/2;
                
                Sprites[1].tpage = tPage;
                
                /* offset of the sprite INSIDE */
                /* the 255X255 pixel TIM picture */
                Sprites[1].u = sprite[spnum].x;
                Sprites[1].v = sprite[spnum].y;

                Sprites[1].cx = TexInfo[sprite[spnum].tex].cx;
                Sprites[1].cy = TexInfo[sprite[spnum].tex].cy;
                
                Sprites[1].r = 0xa0;
                Sprites[1].g = 0xa0;
                Sprites[1].b = 0xa0;
                
                Sprites[1].rotate = sprite[spnum].rotation;
                Sprites[1].scalex = sprite[spnum].sx;
                Sprites[1].scaley = sprite[spnum].sy;
                                
}

