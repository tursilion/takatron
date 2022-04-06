/* PSX Takatron Stuff */

#define ONE 0x1000
#define OTHER 0
#define SPRITE 1

#define SAMP 2
#define MIDI 3

#define NUM_DATA 163
#define NUM_SPRITE 128
#define NUM_MIDI 3
#define NUM_SAMP 28

struct {
	int type,number;
       } data[NUM_DATA];

struct {
        int x,y, w,h, rotation,sx,sy;
       } sprite[NUM_SPRITE];

struct {
	void *address;
       } sample[NUM_SAMP];

struct {
        void *address;
       } midi[NUM_MIDI];

#include <stdio.h>

FILE *fp,*in;
int i,x,y,x1,y1,d,w,h,r,sx,sy;
char buf[80];

void main()
{ fp=fopen("data.dat","wb");
  in=fopen("datatype.h","r");
  if ((fp)&&(in))
  { for (i=0; i<NUM_DATA; i++)
    { fscanf(in,"%s %d\n",buf,&x);
      printf("%s - %d... ",buf,x);
      y=-1;
      if (strcmp(buf,"SPRITE")==0) y=SPRITE;
      if (strcmp(buf,"SAMP")==0) y=SAMP;
      if (strcmp(buf,"MIDI")==0) y=MIDI;
      if (y==-1) y=OTHER;
      printf("%d , %d\n",y,x);
      data[i].type=y;
      data[i].number=x;
    }
    printf("Size: %d\n\n",sizeof(data));
    fwrite((void*)data,sizeof(data),1,fp);
    fclose(fp);
    fclose(in);
  }

/* image, x, y, x2, y2 */
/* Image 0 offset = 640,0 */
/* Image 1 offset = 0,480 */

  fp=fopen("sprite.dat","wb");
  in=fopen("sprdat.h","r");
  if ((fp)&&(in))
  { for (i=0; i<NUM_SPRITE; i++)
    { fscanf(in,"%d %d %d %d %d %d %d %d\n",&d,&x,&y,&x1,&y1,&r,&sx,&sy);
      w=x1-x+1;
      h=y1-y+1;
      if (d==0) x+=640;
      if (d==1) y+=480;
      r=r*512; /* (8 positions) */
      sx=sx*ONE;
      sy=sy*ONE;
      printf("%d , %d , %d, %d, %d, %d, %d, %d\n",d,x,y,w,h,r,sx,sy);
      sprite[i].x=x;
      sprite[i].y=y;
      sprite[i].w=w;
      sprite[i].h=h;
      sprite[i].rotation=r;
      sprite[i].sx=sx;
      sprite[i].sy=sy;
    }
    printf("Size: %d\n\n",sizeof(sprite));
    fwrite((void*)sprite,sizeof(sprite),1,fp);
    fclose(fp);
    fclose(in);
  }


}
      
