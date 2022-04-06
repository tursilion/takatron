/* Takatron for PSX - this code is a terrible mess, don't even try to read
 * it. Ported directly from the PC code and patched everywhere to work.
 */
 
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
        // i.e. 16 levels of z resolution
#define OT_LENGTH               (4)


	// globals

	// Ordering Table handlers
GsOT		WorldOrderingTable[2];
	// actual Ordering Tables themselves							
GsOT_TAG	zSortTable[2][1<<OT_LENGTH];   
	// GPU packet work area
PACKET		GpuOutputPacket[2][PACKETMAX2];

#define blue(x)   (x<<10)
#define green(x)  (x)
#define red(x)    (x<<5)

/* PSX Takatron Stuff */

#define BUFFER 0x80080000

#define FILE long

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
#define TT1D_TIM 99232
#define TT1E_TIM 131092
#define FONT_TIM 131092
#define NUMFONT_TIM 65556
#define SOUND_VB 0x558B0
#define SOUND_VH 16928

#define SPRITEMAX 300             /* This should be more than enough */     
GsSPRITE Sprites[SPRITEMAX];           

struct {
         int type, number;
       } data[NUM_DATA];

struct {
        int tex, x,y, w,h, rotation,sx,sy;
       } sprite[NUM_SPRITE];

int sample[NUM_SAMP]={ 8,0,18,2,25,10,14,4,6,5,17,3,7,23,9,26,13,15,16,12,11,19,20,21,22,1,24,27 };

char default_hi_name[10][15]={"Tursi","Simba","Mufasa","Taka","Vader",
                              "Nala", "Pumbaa","Vitani","Nuka","Ed"};

int midi[4]={ 0,1,2 };

int tex_x[9],tex_y[9];

	// main function
	// see pad.h and pad.c for the controller pad interface

u_long PadStatus = 0;           // state of controller pad
u_long oldpad=0;
int outputBufferIndex;          // which buffer is active (drawn on)

FILE in;
int i,x,y,x1,y1,d,w,h,r,sx,sy,dn,sn;
int spnum,INTITLE,INENDING;
char inp[80];
int retrace_count;
volatile u_char *pad_buf;

#include "takatron.h"

/* max number of bullets for player */
#define MAXBULL 20

/* some defines for IDing enemies */
#define type_Hyena 1
#define type_Buzzard 2
#define type_Lioness 3
#define type_Growl 4
#define type_Kitu 5
#define type_PWR 6
#define type_Rhino 7
#define type_Scar 8
#define type_Obstacle 9
#define type_Jag 10
#define type_Bullet 12
#define type_Wiley 13
#define type_ScarAnim 14
#define type_SBoss 15

/* used for colour cycling - extended to reduce math */
#define SINESIZE 28
#define SINESTEP 9
#define SINESTEP2 18
/* SINESTEP should be SINESIZE/3 - repeat table 3 times */
int sinewave[SINESIZE*3]={ 0,4,12,28,48,72,96,128,156,180,200,216,228,236,240,
                           236,228,216,200,180,156,128,96,72,48,28,12,4,
                           0,4,12,28,48,72,96,128,156,180,200,216,228,236,240,
                           236,228,216,200,180,156,128,96,72,48,28,12,4,
                           0,4,12,28,48,72,96,128,156,180,200,216,228,236,240,
                           236,228,216,200,180,156,128,96,72,48,28,12,4
                          };
int Score_Table[10]={0, 100, 250, 0, 500, 150, 0, 0, 200, 50 };
int cycle_red, cycle_blue, cycle_green;
int nosound,black,white,i,i2,x,y,h,px,py,playershape;
int level,enemyx[100],enemyy[100],enemytype[100],enemysize[100],enemyshape[100],numenemy;
int enemytargetx[100],enemytargety[100],enemylife[100];
int hi_score[10],hi_level[10];
char hi_name[10][15],buf[80],buf2[80];
int UP,DOWN,LEFT,RIGHT;
int FIRE, FIREUP, FIREDOWN, FIRELEFT, FIRERIGHT, GREETING, AFIRE;
int FRAME,Lives,Score,Continues;
int debug,ALWAYS,LOVE,EssCnt;
int HOTBULLETS,CHEATING,STARTLEVEL;
int NOCYCLE,RUNCONFIG,PLASMA;
int CONTROL,MIDIvol,DIGIvol;
int last_xd, last_yd;
int JAGCOL,SCARCOL,phase,oldcheat,oldplasma,FPS;
int PLASMASTEP,PlayAsScar,PlayAsNala,Timer;
int SEEALLEND,sprnum;
short VAB_ID,SEQ_ID;
char VAB_H[20000];
int OLDVOICE=-1,SPEED;
int xt[8],yt[8];
int datex,datey;
char scrolllines[10][60];

int cd_tracks[3][2] = {2, 0, 3, 0, 4, 0};

GsBOXF border;

struct {
        int x,y,xd,yd,shape;
        } bullet[MAXBULL*2];
FILE *fp;

void cycle(void);
void main();
void fail(char *);
void my_sprite(int,int,int,int);
int do_title(void);
int scroll_text(char *);
void mytextout(char[]);
void loadhi(void);
void savehi(void);
int do_game(void);
void fancy_clear(void);
void start_enemies(void);
void restart_enemies(void);
void center_mysprite(int, int, int, int);
void draw_enemies(void);
void move_enemies(void);
int sgn(int);
int abs(int);
void check_shots(void);
int check_enemies(void);
int type(int);
void simba_dies(int);
void game_over(void);
int collide(int,int,int,int,int,int,int);
void new_high(void);
void read_config(void);
void call_joystick(void);
void call_keyboard(void);
void call_1button(void);
void call_4button(void);
void call_keyjoy(void);
void call_joykey(void);
void set_fire(void);
void start8way(int,int);
void startaimedshot(int,int);
void start4wayA(int,int);
void start4wayB(int,int);
void gamewin(void);
void initialize(void);
void initTexture(u_long*,int);
void initSprite(int,int);
void registerborder(void);
void draw_screen(void);
void play_sample(int);
void play_midi(int);
void set_volume(int,int);
void reg_sprite(int,int,int,int,int,int,int,int);
void registercharacter(int,int,char,int);
void textout(int,int,char[],int);
void PlaySeq(int);
void fixdata(void);
int grey(unsigned short);
void center_textout(int,int,char*);
void sscanf(char*,char*,int*,int*,char*);
int text_length(char*);

void fail(char *s)
{
  printf("%s\n",s);

  set_volume(255,255);
  exit(5);
}

struct
{ int r,g,b;
} col[3];

void cycle()
{ /* colour cycling */

cycle_red++;
cycle_blue++;
cycle_green++;

if (cycle_red>=SINESIZE) cycle_red=0;
if (cycle_blue>=SINESIZE) cycle_blue=0;
if (cycle_green>=SINESIZE) cycle_green=0;

col[0].r=sinewave[cycle_red];
col[0].g=sinewave[cycle_green];
col[0].b=sinewave[cycle_blue];

col[1].r=sinewave[cycle_red+SINESTEP];
col[1].g=sinewave[cycle_green+SINESTEP];
col[1].b=sinewave[cycle_blue+SINESTEP];

col[2].r=sinewave[cycle_red+SINESTEP2];
col[2].g=sinewave[cycle_green+SINESTEP2];
col[2].b=sinewave[cycle_blue+SINESTEP2];

retrace_count++;

/* Speed adjust for PAL/NTSC... PC Version was 70fps */
if (retrace_count%SPEED==0) retrace_count++;

}

void main()
{
  int ctr;

 printf("\n\nInitialize Takatron:2098 v2.0PSX...\n");

 FRAME=0;
 debug=0;
 CHEATING=0;
 NOCYCLE=0;
 ALWAYS=-1; /* flag for testing 1 kind of enemy */
 STARTLEVEL=1;
 GREETING=0;
 nosound=0;
 PLASMA=1;
 PLASMASTEP=3;
 FPS=0;
 PlayAsScar=0;
 PlayAsNala=0;
 SEEALLEND=0;
 sprnum=0;

 RUNCONFIG=0;

 read_config();

 initialize();

 cycle_red=0;
 cycle_blue=SINESTEP;
 cycle_green=SINESTEP2;

 VSyncCallback(cycle);

 loadhi();

 for (y=0; y<MAXBULL; y++)
   bullet[y].x=-1;

 sprite[data[TitleDate].number].sx *= 3;
 sprite[data[TitleDate].number].sx /= 2;

 sprite[data[TitleDate].number].sy *= 3;
 sprite[data[TitleDate].number].sy /= 2;

gameloop:
 do_title();
 do_game();
 savehi();
goto gameloop;
}

int do_title()
{ /* draw title pic */
  int si[8],tot,i,i2,x,y,fl,st;
  char buf[50];
  int flag=0;

t_start:

  st=5;

  tot=0;
  for (i=0; i<8; i++)
  { xt[i]=tot;
    tot=tot+(sprite[data[Title1+i].number].w*2) + 1;
    if (i==5) tot+=5;
    if (i==6) tot+=20;
    yt[i]=sprite[data[Title1+i].number].h;
  }
  x=320-(tot/2)+25;
  for (i=0; i<8; i++)
  { xt[i]+=x;
    si[i]=(i+2)*(st*3);
  }

  fancy_clear();

  registerborder();

  draw_screen();

  fl=1;
  play_sample(GO);

  retrace_count=0;
  while (retrace_count<35)
  { registerborder();
    draw_screen();
  }

  while (fl)
  { retrace_count=0;
    registerborder();
    fl=0;
    for (i=0; i<8; i++)
    { if (si[i]>1)
      { fl=1;
        si[i]-=st;
        if (si[i]<st)
        { si[i]=1;
          play_sample(BOOM);
        }
      }
      center_mysprite(Title1+i,xt[i],yt[i]+60,si[i]);
    }
    draw_screen();
    while (retrace_count<4);
  }
  registerborder();

  x=320-(sprite[data[TitleDate].number].w) / 2 +25;
  fl=sprite[data[TitleDate].number].h;
  for (i=19; i>=1; i=i-2)
  { retrace_count=0;
    registerborder();
    y=(140-((i-1)*fl)/2)-20;
    if (y<0) y=0;
    my_sprite(TitleDate,x,y,i);
    /* register takatron */
    for (i2=0; i2<8; i2++)
    { center_mysprite(Title1+i2,xt[i2],yt[i2]+60,1);
    }
    draw_screen();
    while (retrace_count<4);
  }
  play_sample(GROWL);

  datex=x; datey=y;

  PLASMA=1;
  CHEATING=0;
  PlayAsScar=0;
  PlayAsNala=0;

  INTITLE=1;
  INENDING=0;

  for (i=0; i<10; i++)
    scrolllines[i][0]=0;

  if (scroll_text("Top Players")) goto exit;
  play_sample(BraveLion);

  if (scroll_text("   Score  Lvl  Name")) goto exit;
  for (i=9; i>=0; i--)
  { if (i==0)
      sprintf(buf,"~2%2d%8d%3d%10s",i+1,hi_score[i],hi_level[i],&hi_name[i][0]);
    else
      sprintf(buf,"%2d%8d%3d%10s",i+1,hi_score[i],hi_level[i],&hi_name[i][0]);
    if (scroll_text(buf)) goto exit;
  }

  for (i=0; i<10; i++)
    if (scroll_text(" ")) goto exit;

  if (GREETING==0)
  { if (scroll_text("Takatron: 2098")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Driven by his neverending desire")) goto exit;
    if (scroll_text("to be King, in 2098 Scar perfects")) goto exit;
    if (scroll_text("the TAKATRON: a robot species so")) goto exit;
    if (scroll_text("advanced, even Mufasa falls")) goto exit;
    if (scroll_text("before them.")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Using their infallible logic, the")) goto exit;
    if (scroll_text("Takatrons conclude:")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Simba may yet become King,")) goto exit;
    if (scroll_text("and therefore must be eliminated.")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Simba is the last hope of the")) goto exit;
    if (scroll_text("Pridelands. Due to this bizarre")) goto exit;
    if (scroll_text("storyline, he possesses super")) goto exit;
    if (scroll_text("powers. Simba's mission is to")) goto exit;
    if (scroll_text("rescue the lionesses, and")) goto exit;
    if (scroll_text("stop the Takatrons.")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("The forces of HYENA Takatrons")) goto exit;
    if (scroll_text("seek to destroy you.")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("The Indestructible Rhino")) goto exit;
    if (scroll_text("Takatrons will seek out and")) goto exit;
    if (scroll_text("destroy the lionesses.")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Tufe Takatrons are programmed")) goto exit;
    if (scroll_text("to manufacture high speed")) goto exit;
    if (scroll_text("Buzzard Takatrons.")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Beware the ingenious CyberScar")) goto exit;
    if (scroll_text("Takatrons, who possess the power")) goto exit;
    if (scroll_text("to transform lionesses into")) goto exit;
    if (scroll_text("sinister Growls.")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("As you struggle to reach adulthood,")) goto exit;
    if (scroll_text("beware of dangerous obstacles")) goto exit;
    if (scroll_text("in your path.")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Good Luck!")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("All names, characters, and voice")) goto exit;
    if (scroll_text("clips are Copyrighted by Disney")) goto exit;
    if (scroll_text("or Capcom, and")) goto exit;
    if (scroll_text("used in parody. This program")) goto exit;
    if (scroll_text("may not be sold.")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Based on Robotron, which is")) goto exit;
    if (scroll_text("Copyrighted by Williams")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Many graphics from the Genesis")) goto exit;
    if (scroll_text("version of Lion King, done")) goto exit;
    if (scroll_text("by Virgin. (You can tell which")) goto exit;
    if (scroll_text("ones I did.)")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Scar boss (unintentionally) by")) goto exit;
    if (scroll_text("David Sauve")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("The bit that's left is")) goto exit;
    if (scroll_text("Copyrighted by M.Brent, 1998")) goto exit;
    if (scroll_text("tursi&neteng.bc.ca")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Written using GCC and Net Yaroze")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Press Fire to begin game...")) goto exit;
  } else {
    if (scroll_text("Ooh, someone gets the greet...")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("See, I used to read the")) goto exit;
    if (scroll_text("alt.fan.lionking newsgroup,")) goto exit;
    if (scroll_text("and one day when I looked after")) goto exit;
    if (scroll_text("a long absence, I saw there was")) goto exit;
    if (scroll_text("now a mailing list.")) goto exit;
    if (scroll_text("Cool!, says I, and I joined.")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Well, it was quite cool, and")) goto exit;
    if (scroll_text("very active despite the age.")) goto exit;
    if (scroll_text("A lot of people devised stories")) goto exit;
    if (scroll_text("and crossovers with Lion King")) goto exit;
    if (scroll_text("characters. I saw everything")) goto exit;
    if (scroll_text("from Aliens to Beavis and")) goto exit;
    if (scroll_text("Butthead, to SouthPark, to")) goto exit;
    if (scroll_text("an aborted Titanic!")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("So, one day as I'm driving,")) goto exit;
    if (scroll_text("this little muse says to me")) goto exit;
    if (scroll_text("Hey! How about a Llamatron")) goto exit;
    if (scroll_text("Lion King crossover!")) goto exit;
    if (scroll_text("(Llamatron, of course, is Jeff")) goto exit;
    if (scroll_text("Minter's furry version of")) goto exit;
    if (scroll_text("the classic Robotron. Tres")) goto exit;
    if (scroll_text("cool.)")) goto exit;
    if (scroll_text("I figured a quick, loose knock")) goto exit;
    if (scroll_text("off wouldn't take long, I just")) goto exit;
    if (scroll_text("want the muse to leave me alone so")) goto exit;
    if (scroll_text("I can do SSA. Naturally, though,")) goto exit;
    if (scroll_text("I just HAD to finish the PSX port.")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Alright, then, Hi! to everyone who")) goto exit;
    if (scroll_text("reads this far. C'ya!")) goto exit;
    if (scroll_text(" ")) goto exit;

  }

  for (i=0; i<10; i++)
    if (scroll_text(" ")) goto exit;

  goto t_start;

exit:

  INTITLE=0;
  return(0);

}

int scroll_text(char *s)
{ int i, i2, x , y, flag;
  GsBOXF mybox;

  mytextout(s);

  for (i=19; i>=0; i--)
  { retrace_count=0;

    registerborder();

    if (INTITLE)
    { my_sprite(TitleDate,datex,datey,1);
      for (i2=0; i2<8; i2++)
      { center_mysprite(Title1+i2,xt[i2],yt[i2]+60,1);
      }
    }

    if (INENDING)
    { textout(192,80,"CONGRATULATIONS!",0);
    }

    for (i2=2; i2<10; i2++)
    { y=i2*40+50+i+i;
      x=320-(text_length(&scrolllines[i2][0])>>1);
      textout(x,y,&scrolllines[i2][0],2);
    }
    mybox.attribute=0;
    mybox.x=15;
    mybox.y=65;
    mybox.w=610;
    mybox.h=20;
    mybox.r=0;
    mybox.g=0;
    mybox.b=0;

    GsSortBoxFill(&mybox,&WorldOrderingTable[outputBufferIndex],1);

    call_joystick();
    if (FIRE) i=-99;
    rand();

    if (i==-99)
    { flag=1;
      if ((PadStatus&PADL1)&&(PadStatus&PADL2)&&(PadStatus&PADR1)&&(PadStatus&PADR2))
      { play_sample(GROWL); PLASMA=0; flag=0; textout(50,400,"No Plasma",0); goto okay; }

      if ((PadStatus&PADL1)&&(PadStatus&PADL2)&&(PadStatus&PADR2))
      { play_sample(GROWL); CHEATING=1; flag=0; textout(50,400,"Cheat",0); goto okay; }

      if ((PadStatus&PADL1)&&(PadStatus&PADL2)&&(PadStatus&PADR1))
      { play_sample(GROWL); GREETING=1; flag=0; textout(50,400,"Greet",0); goto okay; }

      if ((PadStatus&PADL2)&&(PadStatus&PADR2))
      { play_sample(GROWL); PlayAsScar=1; flag=0; textout(50,400,"Scar",0); goto okay; }

      if ((PadStatus&PADL1)&&(PadStatus&PADR1))
      { play_sample(GROWL); PlayAsNala=1; flag=0; textout(50,400,"Nala",0); goto okay; }

      if ((PadStatus&PADL1)&&(PadStatus&PADR2))
      { play_sample(GROWL); SEEALLEND=1; flag=0; textout(50,400,"SeeAllEnd",0); goto okay; }

      if ((PadStatus&PADR1)&&(PadStatus&PADL2))
      { play_sample(GROWL); STARTLEVEL++; if (STARTLEVEL>50) STARTLEVEL=1; flag=0; sprintf(buf,"Start Level %d",STARTLEVEL); textout(50,400,buf,0); goto okay; }

okay:
      if (flag==0)
        i=0;
    }

    draw_screen();

    while (retrace_count<3);

  }

  for (i2=1; i2<10; i2++)
    strcpy(&scrolllines[i2][0],&scrolllines[i2+1][0]);

  if (i<-99)
    return(1);
  else
    return(0);
}

void mytextout(char s[])
{ int i,i2;
/* just going to ignore the ~x commands.. not using them */
i=0; i2=0;

while (s[i])
{ if (s[i]=='~')
    i+=2;
  else
    scrolllines[9][i2++]=s[i++];
}
scrolllines[9][i2]=0;

}

void loadhi()
{ /* load hi scores */
FILE fp;
int i,card,sndflag;
char buf[129];

registerborder();
center_textout(320,224,"Checking Memory Card(s)");
draw_screen();

card=-1;

if (TestCard(0)==1) card=0;
else if (TestCard(1)==1) card=1;

if (card!=-1)
{ 
  registerborder();
  sprintf(buf,"Loading Card slot %d\n",card);
  center_textout(320,224,buf);
  printf("%s\n",buf);
  draw_screen();

  if (card==0)
    strcpy(buf,"bu00:TAKATRON_CFG");
  else
    strcpy(buf,"bu10:TAKATRON_CFG");

  fp=open(buf,O_RDONLY);

  if (fp==-1)
  { fp=open(buf,O_CREAT|(1<<16));
    close(fp);
    fp=-1;
  }

  sndflag=0;

  if (fp!=-1)
  { for (i=0; i<10; i++)
    { read(fp,buf,128);
      sscanf(buf,"%d,%d,%s\n",&hi_score[i],&hi_level[i],&hi_name[i][0]);
      printf("%s\n",buf);
    }
    read(fp,buf,128);
    if (buf[0]=='M')
    { sndflag=1;
      MIDIvol=atoi(&buf[6]);
      if ((MIDIvol<0)||(MIDIvol>10)) MIDIvol=10;
      read(fp,buf,128);
      if (buf[0]=='D')
      { DIGIvol=atoi(&buf[6]);
        if ((DIGIvol<0)||(DIGIvol>10)) DIGIvol=10;
      }
    }

    if (sndflag==0)
    { MIDIvol=10;
      DIGIvol=10;
    }

    close(fp);
  }
  else
  { for (i=0; i<10; i++)
    { hi_score[i]=(11-i)*1000;
      hi_level[i]=0;
      strcpy(&hi_name[i][0],&default_hi_name[i][0]);
    }
  }
}
}

void savehi()
{ /* save hi scores to hi.dat */
FILE fp;
int i,card;
char buf[255];

registerborder();
center_textout(320,224,"Checking Memory Card(s)");
draw_screen();

card=-1;

if (TestCard(0)==1) card=0;
else if (TestCard(1)==1) card=1;

if (card!=-1)
{ 
  registerborder();
  sprintf(buf,"Saving Card slot %d\n",card);
  center_textout(320,224,buf);
  draw_screen();
  printf("%s\n",buf);

  if (card==0)
    strcpy(buf,"bu00:TAKATRON_CFG");
  else
    strcpy(buf,"bu10:TAKATRON_CFG");

  printf("%s\n",buf);

  fp=open(buf,O_WRONLY);

  if (fp!=-1)
  { for (i=0; i<10; i++)
    { sprintf(buf,"%d,%d,%s",hi_score[i],hi_level[i],&hi_name[i][0]);
      write(fp,buf,128);
      printf("Writing %s\n",buf);
    }
    sprintf(buf,"MIDI: %d",MIDIvol);
    write(fp,buf,128);
    sprintf(buf,"DIGI: %d",DIGIvol);
    write(fp,buf,128);
    close(fp);
  }
}
}

void fancy_clear()
{ /* do the Robotron clear */
int i,j,i2,k,c,yoff;
RECT myrect;

play_sample(GO);

for (i=10; i<320; i=i+10)
{ retrace_count=0;
  registerborder();

  j=i;
  c=253;
  registerborder();
  for (i2=0; i2<=j; i2+=20)
  { k=(i2*3)/8;
    border.attribute=0;
    border.x=320-i2;
    border.y=120-k;
    border.w=i2*2;
    border.h=k*2;
    border.r=col[c-253].r;
    border.g=col[c-253].g;
    border.b=col[c-253].b;
    GsSortBoxFill(&border,&WorldOrderingTable[outputBufferIndex],1);
    c++;
    if (c==256) c=253;
  }
  draw_screen();
  while (retrace_count<1);
}

yoff=PSDOFSY[!GsGetActiveBuff()];

for (i=10; i<320; i=i+10)
{ registerborder();
  retrace_count=0;
  k=(i*3)/8;
  myrect.x=320-i;
  myrect.y=120-k+yoff;
  myrect.w=i*2;
  myrect.h=k*2;
  ClearImage(&myrect,0,0,0);
  while (retrace_count<1);
}

myrect.x=0;
myrect.y=0;
myrect.w=640;
myrect.h=480;
ClearImage(&myrect,0,0,0);
DrawSync(0);

registerborder();

draw_screen();

}

void call_joystick()
{ int k;

  UP=0;
  DOWN=0;
  LEFT=0;
  RIGHT=0;
  FIRE=0;
  FIREUP=0;
  FIREDOWN=0;
  FIRELEFT=0;
  FIRERIGHT=0;
  AFIRE=0;
  FRAME++;
  call_4button(); 

  FIRE=((FIREUP)||(FIREDOWN)||(FIRELEFT)||(FIRERIGHT));
  if (AFIRE) FIRE=0;

  if (PadStatus & PADselect)  /* adjust volume */
  { while (PadStatus & PADselect)
      PadStatus=PadRead();
    play_midi(LOVEMIDI);
    while (!(PadStatus & PADselect))
    { registerborder();
      center_textout(320,100,"Adjust Volume");
      center_textout(320,150,"Up:Down to adjust SFX");
      center_textout(320,200,"Left:Right to adjust Music");
      sprintf(buf,"SFX   : %2d",DIGIvol);
      center_textout(320,300,buf);
      sprintf(buf,"Music : %2d",MIDIvol);
      center_textout(320,350,buf);
      draw_screen();
      VSync(0);
      UP=0;
      DOWN=0;
      LEFT=0;
      RIGHT=0;
      FRAME++;
      call_4button(); 
      if (UP)
      { if (DIGIvol<10) DIGIvol++;
        set_volume(DIGIvol*25,-1);
        play_sample(HIT);
        while (UP)
          call_joystick();
      }
      if (DOWN)
      { if (DIGIvol>0) DIGIvol--;
        set_volume(DIGIvol*25,-1);
        play_sample(HIT);
        while (DOWN)
          call_joystick();
      }
      if (LEFT)
      { while (LEFT)
          call_joystick();
        if (MIDIvol>0) MIDIvol--;
      }
      if (RIGHT)
      { while (RIGHT)
          call_joystick();
        if (MIDIvol<10) MIDIvol++;
      }
      set_volume(-1,MIDIvol*25);
    }
    play_midi(NULL);
    while (PadStatus & PADselect)
      PadStatus=PadRead();
    PadStatus=NULL;
  }

  if (PadStatus & PADstart)   /* PAUSE */
  { while (PadStatus & PADstart)
      PadStatus = PadRead();
    while (!(PadStatus & PADstart))
    { registerborder();
      draw_enemies();
      center_mysprite(playershape,px,py,1);
      center_textout(320,240,"Pause");
      sprintf(buf2,"Score: %d   Lives: %d",Score,Lives);
      y=390;
      x=320;
      center_textout(x,y,buf2);
      draw_screen();
      PadStatus = PadRead();
    }
    while (PadStatus & PADstart)
      PadStatus = PadRead();
  }
}

int do_game()
{ /* play the actual game, start at the title page */
int an,x,y,z,fl,zz;
int maxframe,up,down,left,right;
int index,bs,key_fl,old;
int shoot_fl,tempyd,tempxd;
int firexd, fireyd;
int tpage,xx,yy;

play_sample(Challenge);
retrace_count=0;
while (retrace_count<200)
{ registerborder();
  draw_screen();
}
LOVE=0;
fancy_clear();

level=STARTLEVEL;
Lives=3;
Continues=0;
Score=0;
play_midi(NULL);  /* stop MIDI if it's playing */

/* play */
px=320;
py=240;
an=1;
maxframe=4;
left=simleft;
right=simright;
up=simup;
down=simdown;
if (PlayAsScar)
{ maxframe=3;
  left=Scarleft1;
  right=Scarright1;
  up=Scarup1;
  down=Scardown1;
}
if (PlayAsNala)
{ maxframe=3;
  left=essleft1;
  right=essright1;
  up=essup1;
  down=essdown1;
}

old=left;
last_xd=-15;
last_yd=0;

start:

start_enemies();

start2:

/* restart Simba */
z=30;
px=320;
py=240;
an=1;
for (index=0; index<MAXBULL; index++)
  bullet[index].x=-1;

FRAME=0;

while (check_enemies())
{ retrace_count=0;
  if (LOVE>0) LOVE--;
  if (z>1) z=z-3;
  if (z<1) z=1;
  fl=0;
  call_joystick();
  tempxd=0;
  tempyd=0;

  if (UP)
  { tempyd=-15;
    if (py>40)
    { py=py-10;
      fl=up;
    }
  }

  if (DOWN)
  { tempyd=15;
    if (py<440)
    { py=py+10;
      fl=down;
    }
  }

  if (LEFT)
  { tempxd=-15;
    if (px>30)
    { px=px-10;
      fl=left;
    }
  }

  if (RIGHT)
  { tempxd=15;
    if (px<560)
    { px=px+10;
      fl=right;
    }
  }

  if (FIRE==0)
  { if ((tempxd)||(tempyd))
    { last_xd=tempxd;
      last_yd=tempyd;
    }
  }

  firexd=0;
  fireyd=0;
  if (FIREUP)
    fireyd=-15;
  if (FIREDOWN)
    fireyd=15;
  if (FIRELEFT)
    firexd=-15;
  if (FIRERIGHT)
    firexd=15;
  
  registerborder();

  shoot_fl=!(shoot_fl);
  if ((shoot_fl)&&((firexd)||(fireyd)))
  { for (index=0; index<MAXBULL; index++)
    { if (bullet[index].x==-1)
      { /* new bullet */
        bullet[index].x=px;
        bullet[index].y=py;
        bullet[index].xd=firexd;
        bullet[index].yd=fireyd;
        index=MAXBULL;
      }
    }
  }
  for (index=0; index<MAXBULL; index++)
  {
    if (bullet[index].x==-1) goto skipskip;

    bullet[index].x+=bullet[index].xd;
    bullet[index].y+=bullet[index].yd;

    if ((bullet[index].x<10)||(bullet[index].x>630)||(bullet[index].y<10)||(bullet[index].y>470))
    { bullet[index].x=-1;
      goto skipskip;
    }

    bs=ShotUD;
    if ((bullet[index].xd)&&(bullet[index].yd==0)) bs=ShotLR;
    if ((bullet[index].xd==bullet[index].yd)&&(bullet[index].xd)) bs=ShotD1;
    if ((bullet[index].xd==(-1)*bullet[index].yd)&&(bullet[index].xd)) bs=ShotD2;
    center_mysprite(bs,bullet[index].x,bullet[index].y,1);
    bullet[index].shape=bs;

  skipskip:
  }

  if (fl)
  { an++;
    if (an>=maxframe) an=1;
    center_mysprite(fl+an,px,py,z);
    playershape=fl+an;
    old=fl;
  }
  else
  { center_mysprite(old,px,py,z);
    playershape=old;
  }

  if (check_simba(z))
  { simba_dies(old);
    if (Lives<0)
    { game_over();
      return(1);
    }
    restart_enemies();
    goto start2;
  }

  check_shots();
  move_enemies();
  draw_enemies();

  if (FRAME<45)
  { y=150;
    x=320;

    center_textout(x,y,buf);

    if (strncmp(buf,"Lev",3)==0)
    { sprintf(buf2,"Score: %d   Lives: %d",Score,Lives);
      y=390;
      x=320;
      center_textout(x,y,buf2);
    }
  }

  draw_screen();
  while (retrace_count<4);
}

/*** new level ***/

if (level!=50)
{ fancy_clear();
  level++;
  FRAME=0;
  EssCnt=0;
  HOTBULLETS=0;  /* turn off hot bullets */
  px=320;
  py=240;
  an=1;
  for (index=0; index<MAXBULL; index++)
    bullet[index].x=-1;

  sprintf(buf,"Level %d",level);

  if (level==11)
  { retrace_count=0;
    play_sample(QuiverFear);
    while (retrace_count<130)
    { registerborder();
      draw_screen();
    }
  }

  if ((level>=35)&&(left!=simaleft)&&(!PlayAsScar)&&(!PlayAsNala))
  { retrace_count=0;
    while (retrace_count<80)
    { registerborder();
      draw_screen();
    }
    play_sample(CircleLife);
    retrace_count=0;
    while (retrace_count<150)
    { registerborder();
      draw_screen();
    }
    maxframe=5;
    left=simaleft;
    right=simaright;
    up=simaup;
    down=simadown;
    old=left;
  }

  if (level==45)
    play_midi(BePrepared);

  goto start;
}
else
{ /* level 50 complete */
  gamewin();
  return(1);
}
}

void start_enemies()
{ /* create the enemies for the level */
int i,j,t;

if (level<49) /* no boss */
{ numenemy=(level/2+5);

  /* always ONE Hyena at least */
  enemytype[0]=Hyenaleft1;
  enemysize[0]=1;
  enemytargetx[0]=rand()%400+120;
  enemytargety[0]=rand()%300+90;

  for (i=1; i<numenemy; i++)
  { t=type_Growl;
    while ((t==type_Growl)||(t==type_PWR)||(t==type_Buzzard))
      t=rand()%9;
    if (ALWAYS!=-1) t=ALWAYS;
    switch (t)
    { case type_Buzzard: enemytype[i]=Buzzard1;   break;
      case type_Lioness: enemytype[i]=essleft1;   break;
      case type_Kitu   : enemytype[i]=Kitu1;      break;
      case type_Rhino  : enemytype[i]=Rhinoleft1; break;
      case type_Scar   : enemytype[i]=Scarleft1;  break;
      default          : enemytype[i]=Hyenaleft1; break;
    }
    enemysize[i]=1;
    enemytargetx[i]=rand()%400+120;
    enemytargety[i]=rand()%300+90;
  }

  j=rand()%((numenemy/3)+1);

  for (i=numenemy; i<numenemy+j; i++)
  { enemytype[i]=obstacle1+(rand()%2);
    enemysize[i]=1;
  }
  numenemy+=j;
} else
{ /* level is 49 or 50 */
  if (level==49)
  { numenemy=75;
    for (i=0; i<numenemy; i++)
    { enemytype[i]=-1;
      enemysize[i]=21;
    }
    enemytype[0]=Jag3;
    enemysize[0]=1;
    enemytargetx[0]=rand()%400+120;
    enemytargety[0]=rand()%300+90;
    phase=0;
  }
  else
  { /* level should be 50 */
    for (i=(MIDIvol*25); i>0; i-=5)
    { set_volume(-1,i);
      VSync(0);
    }

    play_midi(NULL);

    retrace_count=0;
    play_sample(Surprised);
    while (retrace_count<350)
    { registerborder();
      draw_screen();
    }

    set_volume(-1,MIDIvol*25);
    play_midi(FinalBoss);

    numenemy=75;
    for (i=0; i<numenemy; i++)
    { enemytype[i]=-1;
      enemysize[i]=21;
    }
    enemytype[1]=FScarBossA;
    enemysize[1]=1;
    phase=0;
  }
}
restart_enemies();
}

void restart_enemies()
{ /* make the enemies appear */
int i,j;

for (i=0; i<numenemy; i++)
{ if (enemytype[i]!=-1)
  { enemyx[i]=rand()%180;
    if (rand()%10>5) enemyx[i]+=320;
    enemyy[i]=rand()%150;
    if (rand()%10>5) enemyy[i]+=240;
  }
  if (type(enemytype[i])==type_Kitu)
  { enemylife[i]=10;
  }
  if (type(enemytype[i])==type_Jag)
  { enemylife[i]=150;
    if (enemyy[i]>240) enemyy[i]-=240;
  }
  if (type(enemytype[i])==type_SBoss)
  { enemylife[i]=300;
    if (enemyy[i]>240) enemyy[i]-=240;
    enemytargetx[i]=640-enemyx[i];
    enemytargety[i]=enemyy[i];
    Timer=0;
  }
}

for (j=30; j>=1; j=j-3)
{ retrace_count=0;
  registerborder();
  for (i=0; i<numenemy; i++)
  { if (enemytype[i]!=-1)
      center_mysprite(enemytype[i],enemyx[i],enemyy[i],j);
      enemyshape[i]=enemytype[i];
  }

  sprintf(buf,"Level %d",level);

  draw_screen();
  while (retrace_count<4);
}
}

void center_mysprite(int spr, int x, int y, int scale)
{ /* y= y coordinate
     z= scale factor
     s= sprite bitmap
  */
  int y2,tspr;

  tspr=data[spr].number;

/*  y2=(y-((scale*(sprite[tspr].h))/2));*/
  y2=y;
  if (y2<0) y2=0;
  my_sprite(spr,x,y2,scale);
}

void my_sprite(int spr, int x, int y, int scale)
{
  u_long colorMode;
  u_short tPage;
  int spnum,ts;

  sprnum++;
  colorMode = 2;
  spnum=data[spr].number;

  tPage = GetTPage(colorMode , 0, tex_x[sprite[spnum].tex], tex_y[sprite[spnum].tex]);
                       
  Sprites[sprnum].x=x;
  Sprites[sprnum].y=y>>1;

  Sprites[sprnum].attribute = (colorMode<<24);
  Sprites[sprnum].w = sprite[spnum].w;              
  Sprites[sprnum].h = sprite[spnum].h;
               
  Sprites[sprnum].mx = sprite[spnum].w/2;  /* center for rot/scaling */
  Sprites[sprnum].my = sprite[spnum].h/2;
                
  Sprites[sprnum].tpage = tPage;
              
  /* offset of the sprite INSIDE */
  /* the 255X255 pixel TIM picture */
  Sprites[sprnum].u = sprite[spnum].x;
  Sprites[sprnum].v = sprite[spnum].y;

  Sprites[sprnum].cx = 0;
  Sprites[sprnum].cy = 0;

  scale--;
  ts=scale;
  if (ts>10) ts=10;

  Sprites[sprnum].r = 0xb0-(ts*0x10);
  Sprites[sprnum].g = 0xb0-(ts*0x10);
  Sprites[sprnum].b = 0xb0-(ts*0x10);

  Sprites[sprnum].rotate = sprite[spnum].rotation;
  if (((sprite[spnum].rotation>1290240)||(sprite[spnum].rotation<184320))||
      ((sprite[spnum].rotation>552960)&&(sprite[spnum].rotation<921600)))
  { Sprites[sprnum].scalex = sprite[spnum].sx;
    Sprites[sprnum].scaley = (sprite[spnum].sy+(scale*4096))>>1;
  } else
  { Sprites[sprnum].scalex = sprite[spnum].sx>>1;
    Sprites[sprnum].scaley = sprite[spnum].sy+(scale*4096);
  }

  if (scale>1)
  { GsSortSprite(&Sprites[sprnum],&WorldOrderingTable[outputBufferIndex],2);
  } else
  { GsSortSprite(&Sprites[sprnum],&WorldOrderingTable[outputBufferIndex],1);
  }

}

void draw_enemies()
{ int i,x,et,ax,ay,speed;

  speed=-10;
  for (i=0; i<numenemy; i++)
  { x=0;
    et=type(enemytype[i]);
    switch (et)
    { case type_Hyena:    x=FRAME%2; break;
      case type_Buzzard:  x=FRAME%3; break;
      case type_Lioness:  x=FRAME%3; break;
      case type_Kitu:     x=FRAME%3; break;
      case type_Rhino:    x=FRAME%2; break;
      case type_Scar:     x=FRAME%3; break;
      case type_SBoss:    x=(Timer==0 ? 0 : 1); break;
    }
    if (enemytype[i]!=-1)
    { if (et==type_SBoss)
      {
        center_mysprite(enemytype[i]+1,enemyx[i]+8,enemyy[i]+(50+x*8),enemysize[i]);
        center_mysprite(enemytype[i],enemyx[i],enemyy[i],enemysize[i]);
      }
      else
      {
        center_mysprite(enemytype[i]+x,enemyx[i],enemyy[i],enemysize[i]);
        enemyshape[i]=enemytype[i]+x;
        if ((LOVE>0)&&(et==type_Lioness))
          center_mysprite(Love_Heart,enemyx[i],enemyy[i]-12,enemysize[i]);
      }
    }
  }
}

void move_enemies()
{ int i,k,ax,ay,fl;
  int speed,tmp,tmp2;

  speed=(level/10)+1;
  for (i=0; i<numenemy; i++)
  { if (enemysize[i]==1)
    { tmp=type(enemytype[i]);
      if (tmp==type_Hyena)
      { ax=sgn(px-enemyx[i])*speed;
        ay=sgn(py-enemyy[i])*speed;
        if (past_target(px,enemyx[i]+ax,ax))
          ax=0;
        if (past_target(py,enemyy[i]+ay,ay))
          ay=0;
        enemyx[i]+=ax;
        enemyy[i]+=ay;
        if (ay<0) enemytype[i]=Hyenaup1;
        if (ay>0) enemytype[i]=Hyenadown1;
        if (ax>0) enemytype[i]=Hyenaright1;
        if (ax<0) enemytype[i]=Hyenaleft1;
      }
      if (tmp==type_Buzzard)
      { ax=sgn(px-enemyx[i])*(speed*2);
        ay=sgn(py-enemyy[i])*speed;
        enemyx[i]+=ax;
        enemyy[i]+=ay;
        if (past_target(px,enemyx[i]+ax,ax))
          ax=0;
        if (past_target(py,enemyy[i]+ay,ay))
          ay=0;
        if (ax<0) enemytype[i]=BuzzardR1;
        if (ax>0) enemytype[i]=Buzzard1;
      }
      if (tmp==type_Lioness)
      { if (LOVE==0)
        { ax=sgn(enemytargetx[i]-enemyx[i])*((speed/2)+1);
          ay=sgn(enemytargety[i]-enemyy[i])*((speed/2)+1);
          if (past_target(enemytargetx[i],enemyx[i]+ax,ax))
            ax=0;
          if (past_target(enemytargety[i],enemyy[i]+ay,ay))
            ay=0;
        } else
        { ax=sgn(px-enemyx[i])*((speed/2)+1);
          ay=sgn(py-enemyy[i])*((speed/2)+1);
        }
        enemyx[i]+=ax;
        enemyy[i]+=ay;
        if (ay<0) enemytype[i]=essup1;
        if (ay>0) enemytype[i]=essdown1;
        if (ax>0) enemytype[i]=essright1;
        if (ax<0) enemytype[i]=essleft1;
        if ((ax==0)&&(ay==0))
        { enemytargetx[i]=rand()%400+120;
          enemytargety[i]=rand()%300+90;
        }
      }
      if (tmp==type_Growl)
      { ax=sgn(enemytargetx[i]-enemyx[i])*(speed*3);
        ay=sgn(enemytargety[i]-enemyy[i])*(speed*3);
        if (past_target(enemytargetx[i],enemyx[i]+ax,ax))
          ax=0;
        if (past_target(enemytargety[i],enemyy[i]+ay,ay))
          ay=0;
        if (ax) ay=0;
        enemyx[i]+=ax;
        enemyy[i]+=ay;
        if (ay<0) enemytype[i]=Growlup;
        if (ay>0) enemytype[i]=Growldown;
        if (ax>0) enemytype[i]=Growlright;
        if (ax<0) enemytype[i]=Growlleft;
        if ((ax==0)&&(ay==0))
        { enemytargetx[i]=px;
          enemytargety[i]=py;
        }
      }
      if (tmp==type_Kitu)
      { ax=sgn(enemytargetx[i]-enemyx[i])*(speed+1);
        ay=sgn(enemytargety[i]-enemyy[i])*(speed+1);
        if (past_target(enemytargetx[i],enemyx[i]+ax,ax))
          ax=0;
        if (past_target(enemytargety[i],enemyy[i]+ay,ay))
          ay=0;
        enemyx[i]+=ax;
        enemyy[i]+=ay;
        if ((ax==0)&&(ay==0))
        { /* start buzzard */
          k=0;
          while ((k<numenemy)&&(enemytype[k]!=-1)) k++;
          if ((k<numenemy)&&(enemylife[i]))
          { if (enemysize[k]>15)
            { enemylife[i]--;
              enemytype[k]=Buzzard1;
              enemysize[k]=1;
              enemyx[k]=enemyx[i];
              enemyy[k]=enemyy[i];
              play_sample(Start_Buzzard);
            }
          } 
          /* choose a new corner */
          fl=rand()%4;
          switch (fl)
          { case 0: enemytargetx[i]=60;
                    enemytargety[i]=60;
                    break;
            case 1: enemytargetx[i]=560;
                    enemytargety[i]=60;
                    break;
            case 2: enemytargetx[i]=60;
                    enemytargety[i]=430;
                    break;
            case 3: enemytargetx[i]=560;
                    enemytargety[i]=430;
                    break;
          }
          if (enemylife[i]==0)
          { enemytargetx[i]=px;
            enemytargety[i]=py;
          }
        }
      }
      if (tmp==type_PWR)
      { ax=sgn(px-enemyx[i])*speed;
        ay=sgn(py-enemyy[i])*speed;
        if (past_target(px,enemyx[i]+ax,ax))
          ax=0;
        if (past_target(py,enemyy[i]+ay,ay))
          ay=0;
        enemyx[i]+=ax;
        enemyy[i]+=ay;
        enemylife[i]--;
        if (enemylife[i]<1)
        { enemytype[i]=-1;
          enemysize[i]=21;
        }
      }
      if (tmp==type_Rhino)
      { ax=sgn(enemytargetx[i]-enemyx[i])*(speed+2);
        ay=sgn(enemytargety[i]-enemyy[i])*(speed+1);
        if (past_target(enemytargetx[i],enemyx[i]+ax,ax))
          ax=0;
        if (past_target(enemytargety[i],enemyy[i]+ay,ay))
          ay=0;
        enemyx[i]+=ax;
        enemyy[i]+=ay;
        if (ay<0) enemytype[i]=Rhinoup1;
        if (ay>0) enemytype[i]=Rhinodown1;
        if (ax>0) enemytype[i]=Rhinoright1;
        if (ax<0) enemytype[i]=Rhinoleft1;
        if ((ax==0)&&(ay==0))
        { enemytargetx[i]=px;
          enemytargety[i]=py;
          fl=0;
          while (fl<numenemy)
          { if (enemytype[fl]!=-1)
            if ((type(enemytype[fl])==type_Lioness)&&(enemysize[fl]==1))
            { if (collide(enemytype[i],enemyx[i],enemyy[i],enemytype[fl],enemyx[fl],enemyy[fl],1))
              { /* got one! */
                play_sample(HIT);
                enemysize[fl]=2;
              }
              enemytargetx[i]=enemyx[fl];
              enemytargety[i]=enemyy[fl];
            }
            fl++;
          }
        }
      }
      if (tmp==type_Scar)
      { ax=sgn(enemytargetx[i]-enemyx[i])*(speed+2);
        ay=sgn(enemytargety[i]-enemyy[i])*(speed+2);
        if (past_target(enemytargetx[i],enemyx[i]+ax,ax))
          ax=0;
        if (past_target(enemytargety[i],enemyy[i]+ay,ay))
          ay=0;
        enemyx[i]+=ax;
        enemyy[i]+=ay;
        if (ay<0) enemytype[i]=Scarup1;
        if (ay>0) enemytype[i]=Scardown1;
        if (ax>0) enemytype[i]=Scarright1;
        if (ax<0) enemytype[i]=Scarleft1;
        if ((ax==0)&&(ay==0))
        { fl=0;
          enemytargetx[i]=px;
          enemytargety[i]=py;
          while (fl<numenemy)
          { if (enemytype[fl]!=-1)
            if (type(enemytype[fl])==type_Lioness)
            { if (collide(enemytype[i],enemyx[i],enemyy[i],enemytype[fl],enemyx[fl],enemyy[fl],1))
              { /* got one! */
                play_sample(GROWL);
                enemytype[fl]=Growlleft;
                enemytargetx[fl]=enemyx[fl];
                enemytargety[fl]=enemyy[fl];
              }
              enemytargetx[i]=enemyx[fl];
              enemytargety[i]=enemyy[fl];
            }
            fl++;
          }
        }
      }
      if (tmp==type_Bullet)
      { enemyx[i]+=enemytargetx[i];
        enemyy[i]+=enemytargety[i];
        if ((enemyx[i]<5)||(enemyx[i]>630)||(enemyy[i]<5)||(enemyy[i]>470))
        { enemyx[i]=-1;
          enemytype[i]=-1;
          enemysize[i]=21;
        }
      }
      if (tmp==type_Jag)
      {
        ax=sgn(enemytargetx[i]-enemyx[i])*(speed*2)+(speed/2);
        ay=sgn(enemytargety[i]-enemyy[i])*(speed*2)+(speed/2);
        if (past_target(enemytargetx[i],enemyx[i]+ax,ax))
          ax=0;
        if (past_target(enemytargety[i],enemyy[i]+ay,ay))
          ay=0;
        if (ax) ay=0;
        enemyx[i]+=ax;
        enemyy[i]+=ay;
        if (ax>0) enemytype[i]=Jag3;
        if (ax<0) enemytype[i]=JagL3;
        if ((ax==0)&&(ay==0))
        { /* at this point the Jag should attack */
          phase++;
          tmp2=phase;
          if ((tmp2>=4)&&(tmp2<=6))
          { if (enemytype[i]>=JagL1)
              enemytype[i]=JagL4;
            else
              enemytype[i]=Jag4;
          }
          if ((tmp2>=7)&&(tmp2<=9))
          { if (enemytype[i]>=JagL1)
              enemytype[i]=JagL3;
            else
              enemytype[i]=Jag3;
          }
          if ((tmp2>=10)&&(tmp2<=12))
          { if (enemytype[i]>=JagL1)
              enemytype[i]=JagL4;
            else
              enemytype[i]=Jag4;
          }
          if (tmp2==13)
          { if (rand()%3==0) phase=32;
          }
          if ((tmp2>=13)&&(tmp2<=15))
          { if (enemytype[i]>=JagL1)
              enemytype[i]=JagL3;
            else
              enemytype[i]=Jag3;
          }
          if ((tmp2>=16)&&(tmp2<=18))
          { if (enemytype[i]>=JagL1)
              enemytype[i]=JagL4;
            else
              enemytype[i]=Jag4;
          }
          if ((tmp2>=19)&&(tmp2<=21))
          { if (enemytype[i]>=JagL1)
              enemytype[i]=JagL3;
            else
              enemytype[i]=Jag3;
          }
          if (tmp2==22)
            start8way(enemyx[i],enemyy[i]);
          if (tmp2==25)
            start8way(enemyx[i],enemyy[i]);
          if (tmp2==28)
            start8way(enemyx[i],enemyy[i]);
          if ((tmp2==81)||(tmp2==31))
          { enemytargetx[i]=px;
            enemytargety[i]=py;
            phase=0;
          }
          if ((tmp2>=32)&&(tmp2<=34))
          { if (enemytype[i]>=JagL1)
              enemytype[i]=JagL1;
            else
              enemytype[i]=Jag1;
          }
          if ((tmp2>=35)&&(tmp2<=37))
          { if (enemytype[i]>=JagL1)
              enemytype[i]=JagL2;
            else
              enemytype[i]=Jag2;
          }
          if ((tmp2>=38)&&(tmp2<=40))
          { if (enemytype[i]>=JagL1)
              enemytype[i]=JagL1;
            else
              enemytype[i]=Jag1;
          }
          if ((tmp2>=41)&&(tmp2<=79)&&(tmp2%2==1))
          { startaimedshot(enemyx[i],enemyy[i]);
          }
        }
      }
      if (tmp==type_Wiley)
      { if (phase<30) phase++;
        if ((phase/5)%2==0)
          enemytype[i]=WileyA;
        else
          enemytype[i]=WileyB;
        if (phase==30)
        { k=0;
          while ((k<numenemy)&&(enemytype[k]!=-1)) k++;
          if (k<numenemy)
          { enemyx[k]=enemyx[i]+35;
            enemyy[k]=0;
            enemytargety[k]=enemyy[i];
            enemysize[k]=1;
            enemytype[k]=ScarALand;
            phase=31;
          }
        }
      }
      if (tmp==type_ScarAnim)
      { /* phase starts at 31 */
        switch (phase)
        { case 31: /* falling */
                   enemyy[i]+=30;
                   if (enemyy[i]>=enemytargety[i])
                   { enemyy[i]=enemytargety[i];
                     phase++;
                   }
                   break;
          case 32:
          case 33:
          case 34: enemytype[i]=ScarASlash1;
                   phase++;
                   break;
          case 35: enemytype[i]=ScarASlash2;
                   phase++;
                   break;
          case 36: enemytype[i]=ScarASlash3;
                   phase++;
                   break;
          case 37: k=0;
                   while ((k<numenemy)&&(type(enemytype[k])!=type_Wiley)) k++;
                   play_sample(HIT);
                   if (k<numenemy)
                     enemysize[k]=2;
                   phase++;
                   break;
          case 38:
          case 39:
          case 40: phase++;
                   break;
          case 41: enemytype[i]=ScarASlash1;
                   phase++;
                   break;
          case 42:
          case 43:
          case 44:
          case 45:
          case 46:
          case 47: phase++;
                   break;
          case 48:
          case 49: enemytype[i]=ScarAJump1;
                   phase++;
                   break;
          case 50: enemytype[i]=ScarAJump2;
                   enemyy[i]-=20;
                   if (enemyy[i]<0)
                   { enemysize[i]=21;
                     enemytype[i]=-1;
                     enemyx[i]=-1;
                     CHEATING=oldcheat;
                     PLASMA=oldplasma;
                   }
                   break;
        }
      }
      if (tmp==type_SBoss)
      { phase++;
        if (Timer)
          Timer--;
        ax=sgn(enemytargetx[i]-enemyx[i])*(phase);
        ay=sgn(enemytargety[i]-enemyy[i])*(phase);
        if (past_target(enemytargetx[i],enemyx[i]+ax,ax))
          ax=0;
        if (past_target(enemytargety[i],enemyy[i]+ay,ay))
          ay=0;
        enemyx[i]+=ax;
        enemyy[i]+=ay;
        if (phase%13==0)
          start4wayA(enemyx[i],enemyy[i]);
        if (phase%17==0)
          start4wayB(enemyx[i],enemyy[i]);

        if ((phase%20==0)&&(Timer==0)&&(rand()%5==0))
        { if (rand()%2)
          { Timer=10;
            play_sample(Run);
          } else
          { Timer=20;
            play_sample(RunAway);
          }
        }

        if ((ax==0)&&(ay==0))
        { /* reached target */
          phase=0;
          enemytargetx[i]=px;
          enemytargety[i]=py;
        }
      } /* end case */
    } else
    { enemysize[i]+=3;
      /* special case for end of final boss */
      if ((level==50)&&(type(enemytype[i])==type_SBoss))
      { /* slow down the explosion */
        enemysize[i]-=1;
        phase--;
        /* repeating the explosion */
        if ((phase%6==0)&&(phase>0))
        { /* start new */
          k=0;
          while ((k<numenemy)&&(enemytype[k]!=-1)) k++;
          if (k<numenemy)
          { enemytype[k]=FScarBossA;
            Timer=999;
            enemysize[k]=2;
            enemyx[k]=enemyx[i]+((rand()%10)-5);
            enemyy[k]=enemyy[i]+((rand()%10)-5);
            play_sample(BOOM);
          }
        }
      }
      if (enemysize[i]>19) enemytype[i]=-1;
    }
  }
}

int sgn(int x)
{ if (x<0) return(-1);
  if (x==0) return(0);
  return(1);
}

int abs(int x)
{ if (x<0) return(-1*x);
  return(x);
}

void check_shots()
{ /* check bullet collisions */
int i,j,k,t;

for (i=0; i<numenemy; i++)
  if (enemysize[i]==1)
    for (j=0; j<MAXBULL; j++)
    { if (bullet[j].x!=-1)
        if (collide(bullet[j].shape,bullet[j].x,bullet[j].y,enemyshape[i],enemyx[i],enemyy[i],1))
        { t=type(enemytype[i]);
          if ((t==type_Hyena)||(t==type_Buzzard)||(t==type_Growl)||(t==type_Kitu)||(t==type_Scar)||(t==type_Obstacle))
          { enemysize[i]=2;
            play_sample(HIT);
            if (HOTBULLETS==0)
              bullet[j].x=-1;
            Score=Score+Score_Table[type(enemytype[i])];
            if (rand()%level<1)
            { /* start powerup */
              k=0;
              while ((k<numenemy)&&(enemytype[k]!=-1)) k++;
              if (k<numenemy)
                if (enemysize[k]>15)
                { enemytype[k]=PWR_1up+(rand()%5);
                  enemysize[k]=1;
                  enemylife[k]=75;
                  enemyx[k]=enemyx[i];
                  enemyy[k]=enemyy[i];
                }
            } /* end pwrup start */
          }
          if (t==type_Rhino)
          { if (HOTBULLETS==0)
              bullet[j].x=-1;
            enemyx[i]+=bullet[j].xd;
            if ((enemyx[i]>600)||(enemyx[i]<30))
              enemyx[i]-=bullet[j].xd;
            enemyy[i]+=bullet[j].yd;
            if ((enemyy[i]>450)||(enemyy[i]<30))
              enemyy[i]-=bullet[j].yd;
          }
          if (t==type_Jag)
          { bullet[j].x=-1;
            play_sample(HIT);
            enemylife[i]--;
            if (enemylife[i]==0)
            { enemysize[i]=2;
              play_sample(BOOM);
              Score+=5000;
              k=0;
              while (k<numenemy)
              { if (type(enemytype[k])==type_Bullet)
                { enemytype[k]=-1;
                  enemyx[k]=-1;
                  enemysize[k]=21;
                }
                k++;
              }
              k=0;
              while ((k<numenemy)&&(enemytype[k]!=-1)) k++;
              if (k<numenemy)
              { enemyx[k]=enemyx[i]+30;
                enemyy[k]=enemyy[i];
                enemysize[k]=1;
                enemytype[k]=WileyA;
                phase=0;
                oldcheat=CHEATING;
                CHEATING=1;
                oldplasma=PLASMA;
                PLASMA=0;
              }
            }
          }
          if (t==type_SBoss)
          { bullet[j].x=-1;
            play_sample(HIT);
            enemyx[i]+=bullet[j].xd;
            if ((enemyx[i]>600)||(enemyx[i]<30))
              enemyx[i]-=bullet[j].xd;
            enemyy[i]+=bullet[j].yd;
            if ((enemyy[i]>450)||(enemyy[i]<30))
              enemyy[i]-=bullet[j].yd;
            enemylife[i]--;
            if (enemylife[i]==0)
            { enemysize[i]=2;
              for (k=(MIDIvol*25); k>0; k-=5)
              { set_volume(-1,i);
                VSync(0);
              }
              play_midi(NULL);
              set_volume(-1,MIDIvol*25);
              play_sample(BOOM);
              Score+=20000;
              phase=4999;
              k=0;
              while (k<numenemy)
              { if (type(enemytype[k])==type_Bullet)
                { enemytype[k]=-1;
                  enemyx[k]=-1;
                  enemysize[k]=21;
                }
                k++;
              }
              k=0;
            }
          }
        }
    }
}

int check_enemies()
{ int i,f,t;

f=0;
for (i=0; i<numenemy; i++)
{ t=type(enemytype[i]);
  if ((enemysize[i]<19)&&(t!=type_Lioness)&&(t!=type_PWR)&&(t!=type_Obstacle)&&(t!=type_Rhino))
    f=1;
}
return(f);
}

int type(int x)
{ /* return type of enemy */
if ((x>=Hyenadown1)&&(x<=Hyenaup2)) return(type_Hyena);
if ((x>=Buzzard1)&&(x<=BuzzardR3)) return(type_Buzzard);
if ((x>=essdown1)&&(x<=essup3)) return(type_Lioness);
if ((x>=Growldown)&&(x<=Growlup)) return(type_Growl);
if ((x>=Kitu1)&&(x<=Kitu3)) return(type_Kitu);
if ((x>=PWR_1up)&&(x<=PWR_Warp)) return(type_PWR);
if ((x>=Rhinodown1)&&(x<=Rhinoup2)) return(type_Rhino);
if ((x>=Scardown1)&&(x<=Scarup3)) return(type_Scar);
if ((x>=obstacle1)&&(x<=obstacle2)) return(type_Obstacle);
if ((x>=Jag1)&&(x<=JagL4)) return(type_Jag);
if ((x>=ShotD1)&&(x<=ShotUD)) return(type_Bullet);
if ((x==WileyA)||(x==WileyB)) return(type_Wiley);
if ((x>=ScarAJump1)&&(x<=ScarASlash3)) return(type_ScarAnim);
if ((x==FScarBossA)||(x==FScarBossB)) return(type_SBoss);
return(0);
}

int check_simba(int z)
{ /* check enemy collisions */
int i,r,t,j,k;

r=0;
rand();
if (z==1) /* eg: Simba is done appearing */
  for (i=0; i<numenemy; i++)
    if ((enemysize[i]==1)&&(enemytype[i]!=-1))
    { t=type(enemytype[i]);
      if (collide(enemyshape[i],enemyx[i],enemyy[i],playershape,px,py,8))
      { if ((t!=type_Lioness)&&(t!=type_PWR))
          r=1;
        if (t==type_PWR)
        { /* picked up a powerup */
          switch (enemytype[i])
          { case PWR_1up : play_sample(Got_1up);
                           Lives++;
                           FRAME=20;
                           strcpy(buf,"Got extra Life!");
                           break;
            case PWR_Continue: play_sample(Got_Continue);
                           Continues++;
                           FRAME=20;
                           strcpy(buf,"Got extra Continue!");
                           break;
            case PWR_Love: if (LOVE<1)
                             play_sample(FEELLOVE);
                           LOVE=300;
                           FRAME=20;
                           strcpy(buf,"Lioness Love!");
                           break;
            case PWR_Roar: play_sample(ROAR);
                           for (j=0; j<numenemy; j++)
                             enemysize[j]=2;
                           retrace_count=0;
                           while (retrace_count<50)
                           { registerborder();
                             draw_enemies();
                             center_mysprite(playershape,px,py,z);
                             draw_screen();
                           }
                           Score+=500;
                           break;
            case PWR_Warp: play_sample(GetOut);
                           retrace_count=0;
                           while (retrace_count<75)
                           { registerborder();
                             draw_enemies();
                             center_mysprite(playershape,px,py,z);
                             draw_screen();
                           }
                           Score+=150;
                           for (j=0; j<numenemy; j++)
                           { enemysize[j]=21;
                             enemytype[j]=-1;
                           }
                           fancy_clear();
                           fancy_clear();
                           fancy_clear();
                           fancy_clear();
                           level+=4;
                           if (level>44)
                             play_midi(BePrepared);
                           for (j=0; j<numenemy; j++)
                           { enemysize[j]=20;
                             enemytype[j]=-1;
                           }
                           if (level>48) level=48;
                           break;
          }
          enemytype[i]=-1;
          enemysize[i]=21;
        }
        if (t==type_Lioness)
        { play_sample(Get_Lioness);
          enemytype[i]=-1;
          enemysize[i]=21;
          k=1;
          for (j=0; j<numenemy; j++)
            if (type(enemytype[j])==type_Lioness)
              k=0;
          EssCnt++;
          Score+=100*EssCnt;
          if (k)
          { play_sample(WOW);
            HOTBULLETS=1;
            FRAME=20;
            strcpy(buf,"Power Shots!");
          }
        }
      }
    }
if (CHEATING) r=0; /* never die */
return(r);
}

void simba_dies(int old)
{ /* Simba bites it */
  int z,time;
  char buf[5],buf2[50];

  play_sample(SIMBADIE);

  for (z=1; z<19; z=z+3)
  { /* scale Simba off */
    retrace_count=0;
    registerborder();
    center_mysprite(old,px,py,z);
    draw_enemies();
    draw_screen();
    while (retrace_count<4);
  }
  Lives--;
  FRAME=0;

  /* clear up still-dying enemies and power ups and enemy shots */
  for (z=0; z<numenemy; z++)
  { if ((enemysize[z]!=1)||(type(enemytype[z])==type_PWR)||(type(enemytype[z])==type_Bullet))
    { enemysize[z]=21;
      enemytype[z]=-1;
    }
  }

  while (retrace_count<75)
  { registerborder();
    draw_screen();
  }

  if (Lives==2)
  { play_sample(NotFair);
    while (retrace_count<150)
    { registerborder();
      draw_screen();
    }
  }

  if ((Lives<0)&&(Continues))
  {
    sprintf(buf2,"Continue? (%d)",Continues);
    play_sample(ComesBack);
    time=9;
    buf[1]=0;
    call_joystick();
    while (FIRE)
      call_joystick();
    while (time>0)
    { retrace_count=0;
      buf[0]=time+'0';
      while (retrace_count<50)
      { registerborder();
        center_textout(320,150,buf2);
        center_textout(320,180,buf);
        draw_screen();
        call_joystick();
        if (FIRE) time=-1;
      }
      time--;
    }
    if (time<0)
    { retrace_count=0;
      play_sample(HesAlive);
      while (retrace_count<250)
      { registerborder();
        draw_screen();
      }
      Lives=3;
      Continues--;
    }
  }
}

void game_over()
{ /* that's all, folks */
  char buf[80];
 
  if (level<10)
    play_sample(Humiliating);
  else
    play_sample(LiveKing);
  call_joystick();
  while (FIRE)
    call_joystick();
  retrace_count=0;
  if (Score>hi_score[9]) retrace_count=250;
  while (retrace_count<500)
  { registerborder();
    center_textout(320,150,"GAME OVER");
    sprintf(buf,"Score: %d   Level: %d",Score,level);
    y=390;
    x=320;
    center_textout(x,y,buf2);
    draw_screen();
    call_joystick();
    if (FIRE) retrace_count=510;
  }
  if (Score>hi_score[9]) new_high();
}

int collide(int s1, int x1a, int y1a, int s2, int x2a, int y2a, int overlap)
{ /* return true if two sprites are touching more than overlap */
  /* y co-ordinates are all centered! */
  /* x is now, too - PSX */
int r,sx1,sy1,sx2,sy2,x1b,y1b,x2b,y2b;

if ((x1a==-1)||(x2a==-1)) return(0);

s1=data[s1].number;
s2=data[s2].number;

r=0;
sx1=sprite[s1].w;
sy1=sprite[s1].h;
x1a=x1a-(sx1/2);
y1a=y1a-(sy1/2);
x1b=x1a+sx1;
y1b=y1a+sy1;
x1a+=overlap;
x1b-=overlap;
if (x1b<=x1a) x1b=x1a+1;
y1a+=overlap;
y1b-=overlap;
if (y1b<=y1a) y1b=y1a+1;

sx2=sprite[s2].w;
sy2=sprite[s2].h;
x2a=x2a-(sx2/2);
y2a=y2a-(sy2/2);
x2b=x2a+sx2;
y2b=y2a+sy2;
x2a+=overlap;
x2b-=overlap;
if (x2b<x2a) x2b=x2a+1;
y2a+=overlap;
y2b-=overlap;
if (y2b<y2a) y2b=y2a+1;

if (((x1a<=x2a)&&(x1b>=x2a))||((x1a<=x2b)&&(x1b>=x2b)))
  if (((y1a<=y2a)&&(y1b>=y2a))||((y1a<=y2b)&&(y1b>=y2b)))
    r=1;

if (((x2a<=x1a)&&(x2b>=x1a))||((x2a<=x1b)&&(x2b>=x1b)))
  if (((y2a<=y1a)&&(y2b>=y1a))||((y2a<=y1b)&&(y2b>=y1b)))
    r=1;

return(r);
}

int past_target(int tx, int px, int ax)
{ /* target, present, added */
int r;

r=0;
if ((ax>0)&&(px>tx)) r=1;
if ((ax<0)&&(px<tx)) r=1;

return(r);
}
  
void new_high()
{ /* new high score */
char buf[10],buf2[80];
int i,j,k,fl;
GsBOXF mybox;

play_sample(NiceOne);
fancy_clear();

fl=0;
i=0;
j=0;

mybox.attribute=(1<<28)|(1<<30);
mybox.w=16;
mybox.h=16;

while ((i<8)&&(fl==0))
{
  retrace_count=0;

  buf[i]=0;

  registerborder();
  center_textout(320,50,"Enter your name (8 letters)");

  center_textout(320,100,"X: Enter     O: Delete");

  mybox.r=col[0].r;
  mybox.g=col[0].g;
  mybox.b=col[0].b;
  mybox.x=(j<<4)+104;
  mybox.y=150;

  GsSortBoxFill(&mybox,&WorldOrderingTable[outputBufferIndex],0);

  center_textout(320,300,"ABCDEFGHIJKLMNOPQRSTUVWXYZ[");

  center_textout(320,200,buf);

  sprintf(buf2,"Score: %d   Level: %d",Score,level);
  y=390;
  x=320;
  center_textout(x,y,buf2);

  draw_screen();

  call_joystick();

  if (LEFT)
  { j--;
    if (j<0) j=26;
  }

  if (RIGHT)
  { j++;
    if (j>26) j=0;
  }

  if ((FIRERIGHT)&&(i>0))
  { i--;
    while (FIRERIGHT)
      call_joystick();
  }

  if (FIREDOWN)
  { buf[i++]=j+'A';
    if (j==26)
    { i--;
      fl=1;
    }
    while (FIREDOWN)
      call_joystick();
  }

  buf[i]=0;

  while (retrace_count<6);

}

i=9;
while ((Score>hi_score[i-1])&&(i>0))
  i--;
sprintf(buf2,"Number %d score!",i+1);
if (i<9)
  for (j=9; j>=i; j--)
  { hi_score[j]=hi_score[j-1];
    hi_level[j]=hi_level[j-1];
    strcpy(&hi_name[j+1][0],&hi_name[j][0]);
  }
hi_score[i]=Score;
hi_level[i]=level;
strcpy(&hi_name[i][0],buf);
if (i==0)
{ play_sample(MyKingdom);
}
retrace_count=0;
while (retrace_count<200)
{ registerborder();
  center_textout(320,200,buf);
  center_textout(320,400,buf2);
  draw_screen();
}

}

void read_config()
{ int *intp,i;
  char string[85];

  CONTROL=NULL;
  MIDIvol=10;
  DIGIvol=10;
  intp=NULL;

  for (i=0; i<10; i++)
  { hi_score[i]=(10-i)*1000;
    hi_level[i]=0;
    strcpy(&hi_name[i][0],"Tursi");
  }
}

void call_4button()
{ unsigned char temp;

PadStatus = PadRead();

if (PadStatus & PADLup)
  UP=1;
if (PadStatus & PADLdown)
  DOWN=1;
if (PadStatus & PADLleft)
  LEFT=1;
if (PadStatus & PADLright)
  RIGHT=1;

if (PadStatus & PADRleft)
  FIRELEFT=1;
if (PadStatus & PADRup)
  FIREUP=1;
if (PadStatus & PADRdown)
  FIREDOWN=1;
if (PadStatus & PADRright)
  FIRERIGHT=1;

if ((*(pad_buf+1)&0xf0)==0x70)  /* dual analog */
{ temp=*(pad_buf+4);
  if (temp<64) AFIRE=FIRELEFT=1;
  if (temp>196) AFIRE=FIRERIGHT=1;

  temp=*(pad_buf+5);
  if (temp<64) AFIRE=FIREUP=1;
  if (temp>196) AFIRE=FIREDOWN=1;

  temp=*(pad_buf+6);
  if (temp<64) LEFT=1;
  if (temp>196) RIGHT=1;

  temp=*(pad_buf+7);
  if (temp<64) UP=1;
  if (temp>196) DOWN=1;
}

}

void start8way(int x, int y)
{ /* start 8 enemy shots from x,y, offsetted */
int i,k;

for (i=0; i<8; i++)
{ k=0;
  while ((k<numenemy)&&(enemytype[k]!=-1)) k++;
  if (k<numenemy)
  { enemyx[k]=x;
    enemyy[k]=y;
    enemysize[k]=1;
    switch (i)
    { case 0: enemytargetx[k]=-15;
              enemytargety[k]=-15;
              enemytype[k]=ShotD1;
              break;
      case 1: enemytargetx[k]=0;
              enemytargety[k]=-15;
              enemytype[k]=ShotUD;
              break;
      case 2: enemytargetx[k]=15;
              enemytargety[k]=-15;
              enemytype[k]=ShotD2;
              break;
      case 3: enemytargetx[k]=-15;
              enemytargety[k]=0;
              enemytype[k]=ShotLR;
              break;
      case 4: enemytargetx[k]=15;
              enemytargety[k]=0;
              enemytype[k]=ShotLR;
              break;
      case 5: enemytargetx[k]=-15;
              enemytargety[k]=15;
              enemytype[k]=ShotD2;
              break;
      case 6: enemytargetx[k]=0;
              enemytargety[k]=15;
              enemytype[k]=ShotUD;
              break;
      case 7: enemytargetx[k]=15;
              enemytargety[k]=15;
              enemytype[k]=ShotD1;
              break;
    }
  }
}
}

void startaimedshot(int x,int y)
{ int k;

  k=0;
  while ((k<numenemy)&&(enemytype[k]!=-1)) k++;
  if (k<numenemy)
  { enemyx[k]=x;
    enemyy[k]=y;
    enemysize[k]=1;
    enemytargetx[k]=15*sgn((px/80)-(x/80));
    enemytargety[k]=15*sgn((py/60)-(y/60));
    if (sgn(enemytargetx[k])==sgn(enemytargety[k]))
      enemytype[k]=ShotD1;
    else
      enemytype[k]=ShotD2;
    if (enemytargetx[k]==0) enemytype[k]=ShotUD;
    if (enemytargety[k]==0) enemytype[k]=ShotLR;
  }
}

void start4wayA(int x, int y)
{ /* start 4 square enemy shots from x,y */
int i,k;

for (i=0; i<4; i++)
{ k=0;
  while ((k<numenemy)&&(enemytype[k]!=-1)) k++;
  if (k<numenemy)
  { enemyx[k]=x;
    enemyy[k]=y;
    enemysize[k]=1;
    switch (i)
    { case 0: enemytargetx[k]=0;
              enemytargety[k]=-15;
              enemytype[k]=ShotUD;
              break;
      case 1: enemytargetx[k]=-15;
              enemytargety[k]=0;
              enemytype[k]=ShotLR;
              break;
      case 2: enemytargetx[k]=15;
              enemytargety[k]=0;
              enemytype[k]=ShotLR;
              break;
      case 3: enemytargetx[k]=0;
              enemytargety[k]=15;
              enemytype[k]=ShotUD;
              break;
    }
  }
}
}
void start4wayB(int x, int y)
{ /* start 4 diagonal enemy shots from x,y, offsetted */
int i,k;

for (i=0; i<4; i++)
{ k=0;
  while ((k<numenemy)&&(enemytype[k]!=-1)) k++;
  if (k<numenemy)
  { enemyx[k]=x;
    enemyy[k]=y;
    enemysize[k]=1;
    switch (i)
    { case 0: enemytargetx[k]=-15;
              enemytargety[k]=-15;
              enemytype[k]=ShotD1;
              break;
      case 1: enemytargetx[k]=15;
              enemytargety[k]=-15;
              enemytype[k]=ShotD2;
              break;
      case 2: enemytargetx[k]=-15;
              enemytargety[k]=15;
              enemytype[k]=ShotD2;
              break;
      case 3: enemytargetx[k]=15;
              enemytargety[k]=15;
              enemytype[k]=ShotD1;
              break;
    }
  }
}
}

void gamewin()
{ int index;

  for (index=0; index<25; index++)
  { retrace_count=0;
    FRAME++;
    registerborder();

    if (PlayAsScar)
      center_mysprite(Scarright1,px,py,1);
    else
      if (PlayAsNala)
        center_mysprite(essright1,px,py,1);
      else
        center_mysprite(simaright,px,py,1);
    draw_screen();
    while (retrace_count<4);
  }
  retrace_count=0;
  play_sample(AllClear);
  while (retrace_count<350)
  { registerborder();
    if (PlayAsScar)
      center_mysprite(Scarright1,px,py,1);
    else
      if (PlayAsNala)
        center_mysprite(essright1,px,py,1);
      else
        center_mysprite(simaright,px,py,1);
    draw_screen();
  }

  fancy_clear();

  INENDING=1;

  play_midi(LOVEMIDI);

  index=rand()%4;
  if (PlayAsScar) index=rand()%2+5;
  if (PlayAsNala) index=7;
  if (SEEALLEND) index=0;

  for (i=0; i<10; i++)
    scrolllines[i][0]=0;

  switch (index)
  { case 0: scroll_text("(Cheesy ending 1 of 4)");
            scroll_text(" ");
            scroll_text("Rafiki woke with a start..");
            scroll_text("'What a nightmare!'");
            scroll_text("He looked out towards");
            scroll_text("Pride Rock where");
            scroll_text("the newborn Simba");
            scroll_text("slept peacefully with");
            scroll_text("his parents.");
            scroll_text(" ");
            scroll_text("'Ah, well. It was");
            scroll_text("only a dream, nothing");
            scroll_text("to worry over.'");
            scroll_text(" ");
            if (!SEEALLEND)
              break;
    case 1: scroll_text("(Cheesy ending 2 of 4)");
            scroll_text(" ");
            scroll_text("Simba stood tall over");
            scroll_text("the shattered remains of");
            scroll_text("Scar's evil devices.");
            scroll_text("He had defeated them all,");
            scroll_text("and once again the");
            scroll_text("Pridelands were safe");
            scroll_text("from Scar's plots.");
            scroll_text(" ");
            scroll_text("'If he'd only used his");
            scroll_text("genius for GOOD instead");
            scroll_text("of evil...'");
            scroll_text(" ");
            scroll_text("Simba shook his head,");
            scroll_text("and turned back towards");
            scroll_text("Pride Rock, knowing");
            scroll_text("the next confrontation");
            scroll_text("was only a matter of time...");
            scroll_text(" ");
            if (!SEEALLEND)
              break;
    case 2: scroll_text("(Cheesy ending 3 of 4)");
            scroll_text(" ");
            scroll_text("Scar leapt from the");
            scroll_text("exploding debris of his");
            scroll_text("giant Takatron, landing");
            scroll_text("on a nearby ledge, leering");
            scroll_text("down at Simba.");
            scroll_text(" ");
            scroll_text("'I'll get you next time,");
            scroll_text("Simba. NEXT TIIMEEE!!'");
            scroll_text(" ");
            scroll_text("Scar turned, and vanished");
            scroll_text("into a small hole. Simba");
            scroll_text("peered in that direction,");
            scroll_text("wondering.");
            scroll_text(" ");
            scroll_text("'That voice... where have I");
            scroll_text("heard that voice before?'");
            scroll_text(" ");
            if (!SEEALLEND)
              break;
    case 3: scroll_text("(Cheesy ending 4 of 4)");
            scroll_text(" ");
            scroll_text("MegaMan teleported in as");
            scroll_text("the last echos of the massive");
            scroll_text("explosion faded.");
            scroll_text(" ");
            scroll_text("'Great work, Simba! You've");
            scroll_text("saved the Pridelands!'");
            scroll_text(" ");
            scroll_text("Simba nodded.");
            scroll_text(" ");
            scroll_text("'And it looks like Scar");
            scroll_text("took care of Dr Wiley");
            scroll_text("for you.'");
            scroll_text(" ");
            scroll_text("Megaman shook his head.");
            scroll_text(" ");
            scroll_text("'Dr Wiley has survived");
            scroll_text("too many times for me");
            scroll_text("to believe that. I");
            scroll_text("must not let my guard");
            scroll_text("down. Neither should");
            scroll_text("you.'");
            scroll_text(" ");
            scroll_text("The two shook hand in paw,");
            scroll_text("and wished each other");
            scroll_text("well for the future.");
            scroll_text("With that, Megaman");
            scroll_text("returned to Dr.Light,");
            scroll_text("while Simba returned");
            scroll_text("to Pride Rock,");
            scroll_text("wondering if he really");
            scroll_text("would meet Scar again...");
            scroll_text(" ");
            if (!SEEALLEND)
              break;
    case 5: scroll_text("(Scar's Cheesy Ending 1 of 2)");
            scroll_text(" ");
            scroll_text("Scar smiled a feral grin");
            scroll_text("as the massive war machine");
            scroll_text("exploded, marking the final");
            scroll_text("defeat of the evil imposter.");
            scroll_text(" ");
            scroll_text("All his life he'd wondered why");
            scroll_text("nothing went his way, never");
            scroll_text("dreaming the size of the");
            scroll_text("conspiracy against him.");
            scroll_text(" ");
            scroll_text("Now, however, all that was");
            scroll_text("left of that was rubble,");
            scroll_text("and Scar had new hope in");
            scroll_text("his heart, and a new future");
            scroll_text("of which only he was in control.");
            scroll_text(" ");
            if (!SEEALLEND)
              break;
    case 6: scroll_text("(Scar's Cheesy Ending 2 of 2)");
            scroll_text(" ");
            scroll_text("Scar stared balefully at");
            scroll_text("the small man in the white frock,");
            scroll_text("as he finished his explanation.");
            scroll_text(" ");
            scroll_text("'You see, Scar, with the");
            scroll_text("help of my robots, the two of");
            scroll_text("us can defeat Meg.. err, Mufasa,");
            scroll_text("and rule the Pridelands!'");
            scroll_text(" ");
            scroll_text("Scar's green eyes burned.");
            scroll_text(" ");
            scroll_text("'Do you take me for a fool,");
            scroll_text("Wiley? Machines, Takatrons");
            scroll_text("indeed! Now.. your interruption");
            scroll_text("has made me lose my lunch...'");
            scroll_text(" ");
            scroll_text("Scar advanced on Dr Wiley");
            scroll_text("slowly, stalking him. An instant");
            scroll_text("later he pounced, as the small");
            scroll_text("man cried for help.");
            scroll_text(" ");
            scroll_text("'Scar!' called an unfamiliar");
            scroll_text("voice. Scar turned. It was");
            scroll_text("a small blue man. 'Drop him.'");
            scroll_text(" ");
            scroll_text("Scar shrugged and obliged.");
            scroll_text(" ");
            scroll_text("'Impeccable timing, Megaman,'");
            scroll_text("sighed Dr Wiley.");
            scroll_text(" ");
            scroll_text("Scar watched in amusement as");
            scroll_text("Megaman carried Dr Wiley");
            scroll_text("off through the air, till he");
            scroll_text("was no longer visible.");
            scroll_text(" ");
            if (!SEEALLEND)
              break;
    case 7: scroll_text("(Nala's Cheesy Ending)");
            scroll_text(" ");
            scroll_text("Nala fell to the ground as");
            scroll_text("Scar was finally defeated,");
            scroll_text("panting for breath. Exhausted,");
            scroll_text("bruised and beaten, but");
            scroll_text("triumphant.");
            scroll_text(" ");
            scroll_text("Sarabi came to her.");
            scroll_text(" ");
            scroll_text("'Nala, it is time for the hunt.'");
            scroll_text(" ");
            scroll_text("Nala looked up at Sarabi,");
            scroll_text("trying to understand how she");
            scroll_text("could be expected to hunt after");
            scroll_text("all her hard work. She realized");
            scroll_text("quickly, it was not Sarabi,");
            scroll_text("it was the way things were, that");
            scroll_text("said she would do the hunting,");
            scroll_text("the birthing and the weaning,");
            scroll_text("the stalking and the killing,");
            scroll_text("the feeding, the guarding...");
            scroll_text(" ");
            scroll_text("Nala sighed and got to her");
            scroll_text("feet. It seems a lioness'");
            scroll_text("work is never done...");
            scroll_text(" ");
            break;
  }

  scroll_text(" ");
  scroll_text(" ");
  scroll_text(" ");

  scroll_text("Credits");
  
  scroll_text(" ");
  if (scroll_text("Takatron: 2098")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("Conceived, designed and")) goto skipcreds;
  if (scroll_text("programmed by")) goto skipcreds;
  if (scroll_text("M.Brent (Tursi)")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("Inspired by crossover")) goto skipcreds;
  if (scroll_text("discussions on")) goto skipcreds;
  if (scroll_text("The Lion King Mailing")) goto skipcreds;
  if (scroll_text("List, hosted at")) goto skipcreds;
  if (scroll_text("www.lionking.org")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("No part of this work has been")) goto skipcreds;
  if (scroll_text("endorsed or is representative")) goto skipcreds;
  if (scroll_text("of any individual or")) goto skipcreds;
  if (scroll_text("organization, except M.Brent.")) goto skipcreds;
  if (scroll_text("The sources listed below did not")) goto skipcreds;
  if (scroll_text("sanction this project nor")) goto skipcreds;
  if (scroll_text("approve the use of their")) goto skipcreds;
  if (scroll_text("products. This work is a")) goto skipcreds;
  if (scroll_text("parody and is not sold")) goto skipcreds;
  if (scroll_text("for profit of any kind.")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("All characters and voice")) goto skipcreds;
  if (scroll_text("clips are (C) Disney,")) goto skipcreds;
  if (scroll_text("from the movie The Lion")) goto skipcreds;
  if (scroll_text("King")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("Most of the artwork is")) goto skipcreds;
  if (scroll_text("from the game The Lion")) goto skipcreds;
  if (scroll_text("King, published by")) goto skipcreds;
  if (scroll_text("Virgin Interactive,")) goto skipcreds;
  if (scroll_text("on the Sega Genesis")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("Dr Wiley and the")) goto skipcreds;
  if (scroll_text("MechaJaguar are from")) goto skipcreds;
  if (scroll_text("MegaMan 2 by Capcom")) goto skipcreds;
  if (scroll_text("(MegaMan and Dr.Light")) goto skipcreds;
  if (scroll_text("are also trademarks of")) goto skipcreds;
  if (scroll_text("Capcom)")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("Additional artwork by")) goto skipcreds;
  if (scroll_text("M.Brent, and")) goto skipcreds;
  if (scroll_text("David Sauve")) goto skipcreds;
  if (scroll_text("(final boss)")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("The concept of the game")) goto skipcreds;
  if (scroll_text("is based on Williams")) goto skipcreds;
  if (scroll_text("Robotron:2084, and")) goto skipcreds;
  if (scroll_text("Jeff Minter's update")) goto skipcreds;
  if (scroll_text("Llamatron.")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("Special Thanks to:")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("David Sauve")) goto skipcreds;
  if (scroll_text("for letting me use the face")) goto skipcreds;
  if (scroll_text("from his Scar 007 pictures")) goto skipcreds;
  if (scroll_text("as the final boss."));
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("Michael Ponce'")) goto skipcreds;
  if (scroll_text("for his Scar WWW Page")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("Jurrel and Binky")) goto skipcreds;
  if (scroll_text("for suggestions and testing")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("Foxxfire")) goto skipcreds;
  if (scroll_text("for suggestions,")) goto skipcreds;
  if (scroll_text("patience listening to the")) goto skipcreds;
  if (scroll_text("same sounds over and over and over,")) goto skipcreds;
  if (scroll_text("and being my love.")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("Lawrence Wright")) goto skipcreds;
  if (scroll_text("for having positive things")) goto skipcreds;
  if (scroll_text("to say :)")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("PSXDEV for tidbits that helped")) goto skipcreds;
  if (scroll_text("make this work, even")) goto skipcreds;
  if (scroll_text("though I got no direct")) goto skipcreds;
  if (scroll_text("answers...")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("Hold L1 & L2 to")) goto skipcreds;
  if (scroll_text("play as Scar")) goto skipcreds;
 
  for (index=10; index>0; index--)
  { if (scroll_text(" ")) goto skipcreds;
    if (index<MIDIvol)
      set_volume(-1,index*25);
  }


skipcreds:

  INENDING=0;

  play_midi(NULL);
  set_volume(-1,MIDIvol*25);

  if (Score>hi_score[9]) new_high();

}

/** PSX Routines **/

void initialize()
{ int buff,c,cnt;

                // initialisation
        SetVideoMode( MODE_NTSC );      // NTSC mode

        SPEED=6;  /* for NTSC */

        /* because of the mode change program, we check here what */
        /* mode we're in and adjust if it's PAL. PAL will be off */
        /* slightly, but probably not noticable. */
        if (GetVideoMode()==MODE_PAL)
          SPEED=3;

        dn=sn=0;

		// graphical initialisation:
                // screen resolution 640 by 240,
                // interlace, use GsGPU offset
	ResetGraph(0);
        GsInitGraph(640 ,240, GsOFSGPU, 0, 0);                
        GsDefDispBuff(0, 0, 0, 240);

/* now the sound system */
SsSetMVol(127,127);
SsUtSetReverbType(SS_REV_TYPE_ROOM);
SsUtReverbOn();
SsUtSetReverbDepth(64,64);

SsSetSerialAttr(SS_CD,SS_MIX,SS_SON);
SsSetSerialVol(SS_CD,127,127);

SEQ_ID=-2;
play_midi(NULL);

/**CDROM**
  printf("read data\n");
  CdReadFile("\\DATA.DAT;1",(u_long*)(data),0);
  CdReadSync(0,0);
  printf("good.\n");

  printf("read sprite\n");
  CdReadFile("\\SPRITE.DAT;1",(u_long*)(sprite),0);
  CdReadSync(0,0);
  printf("good.\n");

  printf("read sound header\n");
  CdReadFile("\\TAKATRON.VH;1",(u_long*)(VAB_H),0);
  CdReadSync(0,0);
  printf("good.\n");

  printf("read sound data\n");
  CdReadFile("\\TAKATRON.VB;1",(u_long*)BUFFER,0);
  CdReadSync(0,0);
  printf("good.\n");

  VAB_ID=SsVabTransfer((unsigned char*)VAB_H,(unsigned char*)BUFFER,-1,1);
  if (VAB_ID<0) printf("\nSound Transfer failed!\n");

  printf("read image1\n");
  CdReadFile("\\TT1A.TIM;1",(u_long*)BUFFER,0);
  CdReadSync(0,0);
  printf("good.\n");
  initTexture((u_long *)BUFFER,1);

  printf("read image2\n");
  CdReadFile("\\TT1B.TIM;1",(u_long*)BUFFER,0);
  CdReadSync(0,0);
  printf("good.\n");
  initTexture((u_long *)BUFFER,2);

  printf("read image3\n");
  CdReadFile("\\TT1C.TIM;1",(u_long*)BUFFER,0);
  CdReadSync(0,0);
  printf("good.\n");
  initTexture((u_long *)BUFFER,3);

  printf("read image4\n");
  CdReadFile("\\TT1D.TIM;1",(u_long*)BUFFER,0);
  CdReadSync(0,0);
  printf("good.\n");
  initTexture((u_long *)BUFFER,4);

  printf("read image5\n");
  CdReadFile("\\TT1E.TIM;1",(u_long*)BUFFER,0);
  CdReadSync(0,0);
  printf("good.\n");
  initTexture((u_long *)BUFFER,5);

  printf("read font\n");
  CdReadFile("\\FONT.TIM;1",(u_long*)BUFFER,0);
  CdReadSync(0,0);
  printf("good.\n");
  initTexture((u_long *)BUFFER,6);

  printf("read font2\n");
  CdReadFile("\\NUMFONT.TIM;1",(u_long*)BUFFER,0);
  CdReadSync(0,0);
  printf("good.\n");
  initTexture((u_long *)BUFFER,7);
**/

/***** CAETLA *****/
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

  printf("read sound header\n");
  in=open("pcdrv:takatron.vh",O_RDONLY);
  if (in)
  { read(in,(char*)(VAB_H),SOUND_VH);
    close(in);
    printf("good.\n");
  }

  printf("read sound data\n");
  in=open("pcdrv:takatron.vb",O_RDONLY);
  if (in)
  { read(in,(char*)BUFFER,SOUND_VB);
    close(in);
    printf("good.\n");
  }
  VAB_ID=SsVabTransfer((unsigned char*)VAB_H,(unsigned char*)BUFFER,-1,1);
  if (VAB_ID<0) printf("\nSound Transfer failed!\n");

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
  { read(in,(char*)BUFFER,500000/*TT1B_TIM*/);
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
  printf("read font\n");
  in=open("pcdrv:font.tim",O_RDONLY);
  if (in)
  { read(in,(char*)BUFFER,FONT_TIM);
    close(in);
    printf("good.\n");
    initTexture((u_long *)BUFFER,6);
  }
  printf("read font2\n");
  in=open("pcdrv:numfont.tim",O_RDONLY);
  if (in)
  { read(in,(char*)BUFFER,NUMFONT_TIM);
    close(in);
    printf("good.\n");
    initTexture((u_long *)BUFFER,7);
  }
		// set up the controller pad
        pad_buf=PadInit();

		// set up the ordering table handlers
	WorldOrderingTable[0].length = OT_LENGTH;
	WorldOrderingTable[1].length = OT_LENGTH;
	WorldOrderingTable[0].org = zSortTable[0];
	WorldOrderingTable[1].org = zSortTable[1];

  /* ensure buffers setup */
  buff=GsGetActiveBuff();
  GsSetWorkBase((PACKET*)GpuOutputPacket[buff]);
  sprnum=0;

  /* this routine adjusts some sprites with corrupt data */
  fixdata();

  printf("\n** Init Okay **\n");

}

void initTexture(addr,data)
u_long *addr;
int data;

{ RECT rect1;
  GsIMAGE TexInfo;

  addr++;

  GsGetTimInfo(addr, &TexInfo);

  rect1.x=TexInfo.px;
  rect1.y=TexInfo.py;
  rect1.w=TexInfo.pw;
  rect1.h=TexInfo.ph;

  tex_x[data]=rect1.x;
  tex_y[data]=rect1.y;

  printf("Data %d at %d, %d\n",data,rect1.x,rect1.y);

  LoadImage(&rect1,TexInfo.pixel);
  /* dma xfer from memory to videoram */

  DrawSync(0);
  /* wait for finish */
}


void initSprite(int x,int spnum)
{
int i;
u_long colorMode;
u_short tPage;

colorMode = 2;

tPage = GetTPage(colorMode , 0, tex_x[sprite[spnum].tex], tex_y[sprite[spnum].tex]);
                       
Sprites[x].attribute = (colorMode<<24);
Sprites[x].w = sprite[spnum].w;              
Sprites[x].h = sprite[spnum].h;
                
Sprites[x].mx = sprite[spnum].w/2;       /* center f. rotation/scaling */
Sprites[x].my = sprite[spnum].h/2;
                
Sprites[x].tpage = tPage;
               
/* offset of the sprite INSIDE */
/* the 255X255 pixel TIM picture */
Sprites[x].u = sprite[spnum].x;
Sprites[x].v = sprite[spnum].y;

Sprites[x].cx = 0;
Sprites[x].cy = 0;
                
Sprites[x].r = 0x80;
Sprites[x].g = 0x80;
Sprites[x].b = 0x80;
                
Sprites[x].rotate = sprite[spnum].rotation;
Sprites[x].scalex = sprite[spnum].sx;
Sprites[x].scaley = sprite[spnum].sy;
                                
}

void registerborder()
{ int i;

  GsClearOt(0, 0, &WorldOrderingTable[outputBufferIndex]);

  border.attribute=0;
  border.x=0;
  border.y=0;
  border.w=640;
  border.h=5;
  border.r=col[0].r;
  border.g=col[0].g;
  border.b=col[0].b;
  GsSortBoxFill((GsBOXF*)&border,&WorldOrderingTable[outputBufferIndex],1);

  border.attribute=0;
  border.x=2;
  border.y=0;
  border.w=8;
  border.h=240;
  border.r=col[0].r;
  border.g=col[0].g;
  border.b=col[0].b;
  GsSortBoxFill((GsBOXF*)&border,&WorldOrderingTable[outputBufferIndex],1);

  border.attribute=0;
  border.x=630;
  border.y=0;
  border.w=10;
  border.h=240;
  border.r=col[0].r;
  border.g=col[0].g;
  border.b=col[0].b;
  GsSortBoxFill((GsBOXF*)&border,&WorldOrderingTable[outputBufferIndex],1);

  border.attribute=0;
  border.x=0;
  border.y=235;
  border.w=640;
  border.h=5;
  border.r=col[0].r;
  border.g=col[0].g;
  border.b=col[0].b;
  GsSortBoxFill((GsBOXF*)&border,&WorldOrderingTable[outputBufferIndex],1);

  border.attribute=0;
  border.x=10;
  border.y=5;
  border.w=620;
  border.h=5;
  border.r=col[1].r;
  border.g=col[1].g;
  border.b=col[1].b;
  GsSortBoxFill((GsBOXF*)&border,&WorldOrderingTable[outputBufferIndex],1);

  border.attribute=0;
  border.x=10;
  border.y=5;
  border.w=10;
  border.h=230;
  border.r=col[1].r;
  border.g=col[1].g;
  border.b=col[1].b;
  GsSortBoxFill((GsBOXF*)&border,&WorldOrderingTable[outputBufferIndex],1);

  border.attribute=0;
  border.x=620;
  border.y=5;
  border.w=107;
  border.h=230;
  border.r=col[1].r;
  border.g=col[1].g;
  border.b=col[1].b;
  GsSortBoxFill((GsBOXF*)&border,&WorldOrderingTable[outputBufferIndex],1);

  border.attribute=0;
  border.x=10;
  border.y=230;
  border.w=620;
  border.h=5;
  border.r=col[1].r;
  border.g=col[1].g;
  border.b=col[1].b;
  GsSortBoxFill((GsBOXF*)&border,&WorldOrderingTable[outputBufferIndex],1);

  border.attribute=0;
  border.x=20;
  border.y=10;
  border.w=600;
  border.h=5;
  border.r=col[2].r;
  border.g=col[2].g;
  border.b=col[2].b;
  GsSortBoxFill((GsBOXF*)&border,&WorldOrderingTable[outputBufferIndex],1);

  border.attribute=0;
  border.x=20;
  border.y=10;
  border.w=10;
  border.h=220;
  border.r=col[2].r;
  border.g=col[2].g;
  border.b=col[2].b;
  GsSortBoxFill((GsBOXF*)&border,&WorldOrderingTable[outputBufferIndex],1);

  border.attribute=0;
  border.x=610;
  border.y=10;
  border.w=10;
  border.h=220;
  border.r=col[2].r;
  border.g=col[2].g;
  border.b=col[2].b;
  GsSortBoxFill((GsBOXF*)&border,&WorldOrderingTable[outputBufferIndex],1);

  border.attribute=0;
  border.x=20;
  border.y=225;
  border.w=600;
  border.h=5;
  border.r=col[2].r;
  border.g=col[2].g;
  border.b=col[2].b;
  GsSortBoxFill((GsBOXF*)&border,&WorldOrderingTable[outputBufferIndex],1);

}

void draw_screen()
{  int buff;
   GsBOXF mybox;

   DrawSync(0);
   VSync(0);
   GsSwapDispBuff();
   if ((PLASMA==0)||(INTITLE)||(INENDING))
   { GsSortClear(0x0, 0x0, 0x0, &WorldOrderingTable[outputBufferIndex]);
   } else
   { mybox.attribute= (2<<28) | (1<<30);
     mybox.x=0;
     mybox.y=0;
     mybox.w=640;
     mybox.h=240;
     mybox.r=128;
     mybox.g=128;
     mybox.b=64;
     GsSortBoxFill(&mybox,&WorldOrderingTable[outputBufferIndex],3);
   }

   GsDrawOt(&WorldOrderingTable[outputBufferIndex]);
   buff=GsGetActiveBuff();
   GsSetWorkBase((PACKET*)GpuOutputPacket[buff]);
   sprnum=0;
}

void reg_sprite(int tpage, int xoff, int yoff, int w, int h, int x, int y, int pri)
{
  u_long colorMode;
  int spnum,ts;
  GsSPRITE Sprite;
  
  colorMode = 2;

  Sprite.x=x;
  Sprite.y=y>>1;

  Sprite.attribute = (colorMode<<24);
  Sprite.w = w;              
  Sprite.h = h;
               
  Sprite.mx = w/2;  /* center for rot/scaling */
  Sprite.my = h/2;
                
  Sprite.tpage = tpage;
              
  /* offset of the sprite INSIDE */
  /* the 255X255 pixel TIM picture */
  Sprite.u = xoff;
  Sprite.v = yoff;

  Sprite.cx = 0;
  Sprite.cy = 0;

  Sprite.r = 0x80;
  Sprite.g = 0x80;
  Sprite.b = 0x80;

  Sprite.rotate = ONE;

  Sprite.scalex = ONE;
  Sprite.scaley = ONE;

  GsSortFastSprite(&Sprite,&WorldOrderingTable[outputBufferIndex],pri);
}

void play_sample(int sam)
{ int a,b,c,fl;
  char buf[80];

a=0;
b=34;
c=0;
fl=1;

OLDVOICE=SsUtKeyOn(VAB_ID,sample[data[sam].number]+1,a,b,c,DIGIvol*12,DIGIvol*12);

/** uncomment this for debugging the sound values for tuning *
while (1)
{ if (fl)
  { SsUtAllKeyOff(0);
    OLDVOICE=SsUtKeyOn(VAB_ID,sample[data[sam].number]+1,a,b,c,127,127);
  }
  fl=0;
  registerborder();
  sprintf(buf,"A = %d",a);
  textout(50,50,buf,0);
  sprintf(buf,"B = %d",b);
  textout(50,100,buf,0);
  sprintf(buf,"C = %d",c);
  textout(50,150,buf,0);
  draw_screen();
  VSync(0);
  call_joystick();
  if (UP) { a++; fl=1; }
  if (DOWN) { a--; fl=1; }
  if (LEFT) { b--; fl=1; }
  if (RIGHT) { b++; fl=1; }
  if (FIREUP) { c++; fl=1; }
  if (FIREDOWN) { c--; fl=1; }
  if (FIRERIGHT) fl=1;
  if (FIRELEFT) { fl=1; data[sam].number++; printf("Sample %d\n",data[sam].number); if (data[sam].number>27) data[sam].number=0; }

}
**/

}

void play_midi(int mid)
{ /* make sure to stop MIDI if NULL is passed */

if (mid==0)
  PlaySeq(-1);
else
  PlaySeq(midi[data[mid].number]);
}

void set_volume(int dig, int mid)
{ /* ignore channel if it's -1 */

/* we ignore digi sound volume here anyway... */

if (mid!=-1)
{ SsSetSerialVol(SS_CD,mid>>1,mid>>1);
}

}

void registercharacter(int x, int y, char c, int pri)
{ int tpage, xx, yy, hh;

  tpage=GetTPage(2,0,256,256);

  c=toupper(c);

  hh=16;

  xx=0;
  if (c<'L') { xx+=80; xx+=16*(c-'A'); }
  if (c>'K') { xx+=16*(c-'L'); }
  yy=224;
  if (c>'K') { yy+=16; }
  if (c==':') { xx=0; yy=224; }
  if (c=='!') { xx=16; yy=224; }
  if (c=='\"') { xx=32; yy=224; }
  if (c==',') { xx=48; yy=224; }
  if (c=='.') { xx=48; yy=224; hh=13; }
  if (c=='?') { xx=64; yy=224; }
  if ((c>='0')&&(c<='9'))
  { tpage=GetTPage(2,0,512,256);
    xx=0;
    if (c>'7') { xx+=16*(c-'8'); }
    if (c<'8') { xx+=16*(c-'0'); }
    yy=224;
    if (c>'7') { yy+=16; }
  }
  if (c=='(') { xx=32; yy=240; tpage=GetTPage(2,0,512,256); }
  if (c==')') { xx=48; yy=240; tpage=GetTPage(2,0,512,256); }
  if (c=='\'') {xx=64; yy=240; tpage=GetTPage(2,0,512,256); }
  if (c=='&') { xx=80; yy=240; tpage=GetTPage(2,0,512,256); }

  if (c!=' ')
    reg_sprite(tpage,xx,yy,16,hh,x,y,pri);
}

void textout(int x, int y, char str[],int pri)
{ int zz;

zz=0;
while (str[zz])
{ registercharacter(x,y,str[zz],pri);
  x+=16;
  zz++;
}
}

void PlaySeq(int x)
{ /* actually play a CD track */

  if ((SEQ_ID!=-2)||(x==-1)) CdPlay(0,(int*)&cd_tracks,0);
  SEQ_ID=1;
  if (x!=-1)
    CdPlay(2,(int*)&cd_tracks[x][0],0);
}

void fixdata()
{ /* fix broken sprites since I don't remember how the data is built */
int s1, s2;

s1=data[WileyA].number;
s2=data[WileyB].number;

sprite[s1].tex=sprite[s2].tex;
sprite[s1].x=2;
sprite[s1].y=224;
sprite[s1].w=38;
sprite[s1].h=32;
sprite[s1].rotation=sprite[s2].rotation;
sprite[s1].sx=sprite[s2].sx;
sprite[s1].sy=sprite[s2].sy;

s1=data[Kitu1].number;
s2=data[PWR_Love].number;

sprite[s1].tex=sprite[s2].tex;
sprite[s1].x=0;
sprite[s1].y=223;
sprite[s1].w=39;
sprite[s1].h=32;
sprite[s1].rotation=sprite[s2].rotation;
sprite[s1].sx=sprite[s2].sx;
sprite[s1].sy=sprite[s2].sy;

s1=data[Kitu2].number;

sprite[s1].tex=sprite[s2].tex;
sprite[s1].x=41;
sprite[s1].y=223;
sprite[s1].w=46;
sprite[s1].h=32;
sprite[s1].rotation=sprite[s2].rotation;
sprite[s1].sx=sprite[s2].sx;
sprite[s1].sy=sprite[s2].sy;

s1=data[Kitu3].number;

sprite[s1].tex=sprite[s2].tex;
sprite[s1].x=89;
sprite[s1].y=223;
sprite[s1].w=35;
sprite[s1].h=33;
sprite[s1].rotation=sprite[s2].rotation;
sprite[s1].sx=sprite[s2].sx;
sprite[s1].sy=sprite[s2].sy;

s1=data[Hyenaright2].number;
sprite[s1].w-=4;

s1=data[Hyenaleft2].number;
sprite[s1].w-=4;

s1=data[Hyenaup2].number;
sprite[s1].w-=4;

s1=data[Hyenadown2].number;
sprite[s1].w-=4;

}

int grey(unsigned short in)
{ int x;

x=(((in&0x7c00)+(in&0x3e0)+(in&0x1f))/3);

return(x);
}

void center_textout(int x,int y,char *s)
{ x-=(strlen(s)<<3);
  textout(x,y,s,0);
}

void sscanf(char buf[], char *foo, int *a, int *b, char *c)
{ int i;

  i=0;

  *a=atoi(&buf[i]);
  while ((buf[i]!=',')&&(buf[i])) i++;
  i++;
  *b=atoi(&buf[i]);
  while ((buf[i]!=',')&&(buf[i])) i++;
  i++;
  strcpy(c,&buf[i]);
}

int text_length(char *s)
{ return(strlen(s)<<4);
}
