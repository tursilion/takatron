/* TakaTron: 2098  - by Mike Brent */
/* *undo* the Windows port. Guess I lost the original source */
/* this is for DOS                                           */
/* 16 Sep 2000 */

#include <stdio.h>
#include <crt0.h>
#include <allegro.h>
#include <string.h>
#include "takatron.h"

#define SCREENOFFY 480

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
/* #define type_Shadow 11 - deleted */
#define type_Bullet 12
#define type_Wiley 13
#define type_ScarAnim 14
#define type_SBoss 15

/* defines for control types */
#define CONTROL_KEYBOARD 1
#define CONTROL_1BUTTON  2
#define CONTROL_4BUTTON  3
#define CONTROL_KEY_JOY  4
#define CONTROL_JOY_KEY  5

/* set some flags to fix CWSDPMI problems (?)*/
int _crt0_startup_flags=_CRT0_FLAG_NULLOK | _CRT0_FLAG_FILL_SBRK_MEMORY;

/* used for colour cycling - extended to reduce math */
#define SINESIZE 28
#define SINESTEP 9
#define SINESTEP2 18
/* SINESTEP should be SINESIZE/3 - repeat table 3 times */
int sinewave[SINESIZE*3]={ 0,1,3,7,12,18,24,32,39,45,50,54,57,59,60,
                           59,57,54,50,45,39,32,24,18,12,7,3,1,
                           0,1,3,7,12,18,24,32,39,45,50,54,57,59,60,
                           59,57,54,50,45,39,32,24,18,12,7,3,1, 
                           0,1,3,7,12,18,24,32,39,45,50,54,57,59,60,
                           59,57,54,50,45,39,32,24,18,12,7,3,1 
                          };
int Score_Table[10]={0, 100, 250, 0, 500, 150, 0, 0, 200, 50 };
int cycle_red, cycle_blue, cycle_green;
int nosound,black,white,i,i2,x,y,h,px,py,playershape;
int level,enemyx[100],enemyy[100],enemytype[100],enemysize[100],enemyshape[100],numenemy;
int enemytargetx[100],enemytargety[100],enemylife[100];
DATAFILE *data;
BITMAP *work,*work2;
int hi_score[10],hi_level[10];
char hi_name[10][15],buf[80],buf2[80];
FONT *scrollfont;
int UP,DOWN,LEFT,RIGHT;
int FIRE, FIREUP, FIREDOWN, FIRELEFT, FIRERIGHT, GREETING;
int FRAME,Lives,Score,Continues;
int debug,ALWAYS,LOVE,EssCnt;
int HOTBULLETS,CHEATING,STARTLEVEL;
int NOCYCLE,RUNCONFIG,PLASMA;
int CONTROL,MIDIvol,DIGIvol;
int last_xd, last_yd;
RGB JAGNORM,JAGWHITE,SCARNORM,BLACKCOL;
int JAGCOL,SCARCOL,phase,oldcheat,oldplasma,FPS;
int PLASMASTEP,PlayAsScar,PlayAsNala,Timer;
int SEEALLEND;

struct {
        int x,y,xd,yd,shape;
        } bullet[MAXBULL*2];
FILE *fp;

void cycle(void);
void main(int,char*[]);
void fail(char *);
void my_sprite(BITMAP*,BITMAP*,int,int,int);
int do_title(void);
int scroll_text(char *);
void mytextout(char*);
void loadhi(void);
void savehi(void);
int do_game(void);
void fancy_clear(void);
void start_enemies(void);
void restart_enemies(void);
void center_mysprite(BITMAP *, BITMAP *, int, int, int);
void draw_enemies(void);
void call_joystick(void);
void move_enemies(void);
int sgn(int);
int myabs(int);
void check_shots(void);
int check_enemies(void);
int check_simba(int);
int type(int);
void simba_dies(int);
void game_over(void);
int collide(int,int,int,int,int,int,int);
void new_high(void);
void do_config(void);
int past_target(int, int, int);
void read_config(void);
void call_keyboard(void);
void call_1button(void);
void call_4button(void);
void call_keyjoy(void);
void call_joykey(void);
void set_fire(void);
void start8way(int,int);
void startaimedshot(int,int);
void do_plasma(BITMAP*);
void start4wayA(int,int);
void start4wayB(int,int);
void gamewin(void);
void my_sprite(BITMAP *,BITMAP *, int, int, int);

void fail(char *s)
{ allegro_exit();
  printf("%s\n",s);

  set_volume(255,255);
  exit(5);
}

void cycle()
{ /* -- Interrupt Routine -- */
  /* colour cycle colour 253-255 - called 50 times/sec */
RGB col, colt;

cycle_red++;
cycle_blue++;
cycle_green++;

if (cycle_red>=SINESIZE) cycle_red=0;
if (cycle_blue>=SINESIZE) cycle_blue=0;
if (cycle_green>=SINESIZE) cycle_green=0;

col.r=sinewave[cycle_red];
col.g=sinewave[cycle_green];
col.b=sinewave[cycle_blue];
set_color(253,&col);

col.r=sinewave[cycle_red+SINESTEP];
col.g=sinewave[cycle_green+SINESTEP];
col.b=sinewave[cycle_blue+SINESTEP];
set_color(254,&col);

col.r=sinewave[cycle_red+SINESTEP2];
col.g=sinewave[cycle_green+SINESTEP2];
col.b=sinewave[cycle_blue+SINESTEP2];
set_color(255,&col);

}
END_OF_FUNCTION(cycle);

void main(int argc, char *argv[])
{ RGB col;

 printf("\n\nInitialize Takatron:2098 v1.0...\n");

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

 RUNCONFIG=0;
 fp=fopen("takatron.cfg","r");  /* check existance of file */
 if (fp==NULL)
 { RUNCONFIG=1;
   CONTROL=CONTROL_KEYBOARD;
   MIDIvol=10;
   DIGIvol=10;
 } else
 { fclose(fp);
   printf("\nRead Config...\n");
   read_config();
 }

 if (*argv[1]=='?')
 { fail("TAKATRON.EXE [!]\n  ! - no sound (use if it locks)\nOther options in TAKATRON.DOC\n");
 }
 allegro_init();
 install_timer();
 install_keyboard();
 LOCK_VARIABLE(cycle_red);
 LOCK_VARIABLE(cycle_blue);
 LOCK_VARIABLE(cycle_green);
 LOCK_FUNCTION(cycle);

 set_color_depth(8);
 if (set_gfx_mode(GFX_AUTODETECT,640,480,640,480))
   fail("Can not set SVGA 640x480x256 graphics mode\n");

 for (x=1; x<argc; x++)
 { 
   if (*argv[x]=='!')
   { nosound=1;
     printf("No sound!\n");
   }
   if (strcmp(argv[x],"-greet")==0)
   { GREETING=1;
     printf("Greets!\n");
   }
   if (strcmp(argv[x],"debug")==0)
   { debug=1;
     printf("Debug mode on.\n");
   }
   if (strcmp(argv[x],"always")==0)
   { x++;
     ALWAYS=atoi(argv[x]);
     printf("Always enemy type %d\n",ALWAYS);
   }
   if (strcmp(argv[x],"cheat")==0)
   { CHEATING=1;
     printf("Cheat mode on.\n");
   }
   if (strcmp(argv[x],"start")==0)
   { x++;
     STARTLEVEL=atoi(argv[x]);
     printf("Start on level %d\n",STARTLEVEL);
   }
   if (strcmp(argv[x],"cycle")==0)
   { NOCYCLE=0;
     printf("Colour Cycling enabled\n");
   }
   if (strcmp(argv[x],"config")==0)
   { RUNCONFIG=1;
     printf("Run Config.\n");
   }
   if (strcmp(argv[x],"noplasma")==0)
   { PLASMA=0;
     printf("Plasma effect off.\n");
   }
   if (strcmp(argv[x],"fps")==0)
   { FPS=1;
     printf("Frame count display on.\n");
   }
   if (strcmp(argv[x],"plasmastep")==0)
   { x++;
     PLASMASTEP=atoi(argv[x]);
     printf("Plasma Step %d\n",PLASMASTEP);
   }
   if (strcmp(argv[x],"scar")==0)
   { PlayAsScar=1;
     printf("Play as Scar!\n");
   }
   if (strcmp(argv[x],"nala")==0)
   { PlayAsNala=1;
     printf("Play as Nala!\n");
   }
   if (strcmp(argv[x],"seeallend")==0)
   { SEEALLEND=1;
     printf("See ALL endings!\n");
   }
 }

 if (!nosound)
 { if (exists("patches.dat"))
     install_sound(DIGI_AUTODETECT,MIDI_DIGMID,NULL);
   else
     install_sound(DIGI_AUTODETECT,MIDI_AUTODETECT,NULL);
 }
 else
   install_sound(DIGI_NONE,MIDI_NONE,NULL);

 data=load_datafile("takatron.dat");
 if (data==NULL)
 { fail("Cannot load takatron.dat");
 }

 work=create_bitmap(640,960); /* extra room for stretched blits */
 if (work==NULL) fail("Can not reserve work memory.\n");
 work2=create_bitmap(640,480); /* static work screen */
 if (work2==NULL) fail("Not enough memory for workspace\n");

 set_palette(data[palette].dat);

 cycle_red=0;
 cycle_blue=SINESTEP;
 cycle_green=SINESTEP2;

 col.r=255;
 col.g=0;
 col.b=0;
 set_color(253, &col);
 if (NOCYCLE)
 {
	 col.g=255;
 }
 else
 {
	col.g=16;
	col.b=16;
 }
 set_color(254, &col);
 if (NOCYCLE)
 {
	 col.b=255;
 }
 else
 {
	col.g=32;
	col.b=32;
 }
 set_color(255, &col);

 /* find black */
 black=0;

 /* find white (as close as possible) */
 white=0;

 for (y=1; y<253; y++)
 { get_color(y,&col);
   if ((col.r>50)&&(col.g>50)&&(col.b>50))
     white=y;
 }

 /* setup for boss flashes */
 JAGCOL=17; /* I just *know* it is, okay? ;) Change the palette and die ;) */
 SCARCOL=43;
 get_color(white,&JAGWHITE);
 get_color(JAGCOL,&JAGNORM);
 get_color(SCARCOL,&SCARNORM);
 get_color(black,&BLACKCOL);

 if (!NOCYCLE)
 { if (install_int(cycle,20))
     fail("Can not install interrupt routine cycle()\n");
 }

 loadhi();

 for (y=0; y<MAXBULL; y++)
   bullet[y].x=-1;

 if (RUNCONFIG)
   do_config();
 else
   read_config();

 joy_type=JOY_TYPE_STANDARD;

 if (CONTROL==CONTROL_4BUTTON)
   joy_type=JOY_TYPE_4BUTTON;

 if (CONTROL!=CONTROL_KEYBOARD)
   initialise_joystick();

gameloop:
 clear_to_color(screen,black);
 if (do_title()) goto backtodos;
 if (do_game()) goto gameloop;

backtodos:
 if (!NOCYCLE)
	 remove_int(cycle);
 savehi();

}

int do_title()
{ /* draw title pic */
  int xt[8],yt[8],si[8],tot,i,x,y,fl,st;
  char buf[50];

t_start:

  st=5;

  tot=0;
  for (i=0; i<8; i++)
  { xt[i]=tot;
    tot=tot+((BITMAP*)(data[Title1+i].dat))->w + 1;
    yt[i]=((BITMAP*)(data[Title1+i].dat))->h;
  }
  x=320-(tot/2);
  for (i=0; i<8; i++)
  { xt[i]+=x;
    si[i]=(i+2)*(st*3);
  }

  fancy_clear();

  clear_to_color(work2,black);
  for (i=0; i<10; i++)
  { rect(work2,i,i,639-i,479-i,253);
    rect(work2,i+10,i+10,629-i,469-i,254);
    rect(work2,i+20,i+20,619-i,459-i,255);
  }

  blit(work2,screen,0,0,0,0,640,480);

  fl=1;
  play_sample(data[GO].dat,255,128,1000,0);

  retrace_count=0;
  while (retrace_count<35);

  while (fl)
  { retrace_count=0;
    blit(work2,work,0,0,0,SCREENOFFY,640,480);
    fl=0;
    for (i=0; i<8; i++)
    { if (si[i]>1)
      { fl=1;
        si[i]-=st;
        if (si[i]<st)
        { si[i]=1;
          play_sample(data[BOOM].dat,220,128,1000,0);
        }
      }

      center_mysprite(work,data[Title1+i].dat,xt[i],yt[i]+45,si[i]);
    }
    blit(work,screen,0,SCREENOFFY,0,0,640,480);
    while (retrace_count<4);
  }
  blit(work,work2,0,SCREENOFFY,0,0,640,480);
  x=320-(((BITMAP*)(data[TitleDate].dat))->w) / 2;
  fl=((BITMAP*)(data[TitleDate].dat))->h;
  for (i=19; i>=1; i=i-2)
  { retrace_count=0;
    blit(work2,work,0,0,0,SCREENOFFY,640,480);
    y=(150-((i-1)*fl)/2)+SCREENOFFY-20;
    if (y<0) y=0;
    my_sprite(work,data[TitleDate].dat,x,y,i);
    blit(work,screen,0,SCREENOFFY,0,0,640,480);
    while (retrace_count<4);
  }
  play_sample(data[GROWL].dat,255,128,1000,0);
  clear_to_color(work,black); 

  scrollfont=data[MyFont].dat;

  if (scroll_text("Top Players")) goto exit;
  play_sample(data[BraveLion].dat,255,128,1000,0);

  if (scroll_text("#  Score  Lvl  Name")) goto exit;
  for (i=9; i>=0; i--)
  { if (i==0)
      sprintf(buf,"~2%2d%8d%3d%10s",i+1,hi_score[i],hi_level[i],&hi_name[i][0]);
    else
      sprintf(buf,"%2d%8d%3d%10s",i+1,hi_score[i],hi_level[i],&hi_name[i][0]);
    if (scroll_text(buf)) goto exit;
  }

  for (i=0; i<10; i++)
    if (scroll_text(" ")) goto exit;

  scrollfont=data[Times].dat;

  if (GREETING==0)
  { if (scroll_text("~1Takatron: 2098")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Driven by his never-ending desire")) goto exit;
    if (scroll_text("to be King, in ~22098~1 Scar perfects")) goto exit;
    if (scroll_text("the ~3TAKATRON~1 - a robot species so")) goto exit;
    if (scroll_text("advanced, even Mufasa falls before them.")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Using their infallible logic, the")) goto exit;
    if (scroll_text("~3Takatrons~1 conclude:")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("~2Simba may yet become King,")) goto exit;
    if (scroll_text("~2and therefore must be eliminated.")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Simba is the last hope of the Pridelands.")) goto exit;
    if (scroll_text("Due to this bizarre storyline, he")) goto exit;
    if (scroll_text("possesses super powers. Simba's mission")) goto exit;
    if (scroll_text("is to rescue the lionesses, and stop the")) goto exit;
    if (scroll_text("~3Takatrons.")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("The forces of ~3HYENA~1 Takatrons")) goto exit;
    if (scroll_text("seek to destroy you.")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("The ~2Indestructible Rhino~1 Takatrons will")) goto exit;
    if (scroll_text("seek out and destroy the lionesses.")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("~2Tufe~1 Takatrons are programmed")) goto exit;
    if (scroll_text("to manufacture high speed")) goto exit;
    if (scroll_text("~2Buzzard~1 Takatrons.")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Beware the ingenious ~2CyberScar~1 Takatrons,")) goto exit;
    if (scroll_text("who possess the power to transform")) goto exit;
    if (scroll_text("lionesses into sinister ~2Growls~1.")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("As you struggle to reach adulthood,")) goto exit;
    if (scroll_text("beware of dangerous ~2obstacles~1")) goto exit;
    if (scroll_text("in your path.")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("~3Good Luck!")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("All names, characters, and voice clips are")) goto exit;
    if (scroll_text("Copyrighted by ~2Disney~1 or")) goto exit;
    if (scroll_text("~3Capcom~1, and")) goto exit;
    if (scroll_text("used in parody. This program")) goto exit;
    if (scroll_text("may ~3not~1 be sold.")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Based on Robotron, which is")) goto exit;
    if (scroll_text("Copyrighted by ~2Williams")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Many graphics from the Genesis")) goto exit;
    if (scroll_text("version of ~2Lion King~1, done by ~3Virgin.")) goto exit;
    if (scroll_text("(You can tell which ones ~3I~1 did. ;) )")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Scar boss (unintentionally ;) ) by")) goto exit;
    if (scroll_text("~2David Sauve~1")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("The bit that's left is")) goto exit;
    if (scroll_text("Copyrighted by ~3M.Brent~1, 1998")) goto exit;
    if (scroll_text("http://www.neteng.bc.ca/^tursi")) goto exit;
    if (scroll_text("tursi@neteng.bc.ca")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Written using ~2DJGPP~1 and ~3Allegro 3.0")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Press ~2Fire~1 to begin game...")) goto exit;
    if (scroll_text("Press ~3ESC~1 to quit")) goto exit;
  } else {
    if (scroll_text("Ooh, someone used -greet...")) goto exit;
    if (scroll_text("I wonder if I have any fans yet,")) goto exit;
    if (scroll_text("or if only people I remind")) goto exit;
    if (scroll_text("will see this. :) Thought I'd")) goto exit;
    if (scroll_text("do things a little different")) goto exit;
    if (scroll_text("for this one. :)")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Well.. this was mostly done in")) goto exit;
    if (scroll_text("the small town of Grand Forks,")) goto exit;
    if (scroll_text("while working here for my")) goto exit;
    if (scroll_text("company. I thought I'd babble")) goto exit;
    if (scroll_text("about how it came to be. :)")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("See, I used to read the")) goto exit;
    if (scroll_text("alt.fan.lionking newsgroup,")) goto exit;
    if (scroll_text("and one day when I looked after")) goto exit;
    if (scroll_text("a long absence, I saw there was")) goto exit;
    if (scroll_text("now a mailing list.")) goto exit;
    if (scroll_text("~2Cool!~1, says I, and I joined.")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Well, it was quite cool, and")) goto exit;
    if (scroll_text("very active despite the age.")) goto exit;
    if (scroll_text("A lot of people devised stories")) goto exit;
    if (scroll_text("and crossovers with Lion King")) goto exit;
    if (scroll_text("characters. I saw everything")) goto exit;
    if (scroll_text("from ~2Aliens~1 to ~3Beavis and")) goto exit;
    if (scroll_text("~3Butthead~1, to ~2SouthPark~1, to")) goto exit;
    if (scroll_text("an aborted ~3Titanic~1!")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("So, one day as I'm driving,")) goto exit;
    if (scroll_text("this little muse says to me")) goto exit;
    if (scroll_text("~2Hey! How about a Llamatron/")) goto exit;
    if (scroll_text("~2Lion King crossover?")) goto exit;
    if (scroll_text("(Llamatron, of course, is ~3Jeff")) goto exit;
    if (scroll_text("~3Minter~1's furry version of")) goto exit;
    if (scroll_text("the classic ~2Robotron~1. Tres")) goto exit;
    if (scroll_text("cool.)")) goto exit;
    if (scroll_text("I figured a quick, loose knock")) goto exit;
    if (scroll_text("off wouldn't take long, I just")) goto exit;
    if (scroll_text("want the muse to leave me alone so")) goto exit;
    if (scroll_text("I can do SSA. It's growing, though,")) goto exit;
    if (scroll_text("as I add little things it NEEDS")) goto exit;
    if (scroll_text("to have character.")) goto exit;
    if (scroll_text("Well, mebbe at least ONE person on")) goto exit;
    if (scroll_text("that list will try this and like it,")) goto exit;
    if (scroll_text("since I started it for them. When")) goto exit;
    if (scroll_text("I announced it, though, I got")) goto exit;
    if (scroll_text("zero response, and that was")) goto exit;
    if (scroll_text("discouraging.")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Well, this is getting long. There are")) goto exit;
    if (scroll_text("fewer options than planned, due to")) goto exit;
    if (scroll_text("the size and number of sprites,")) goto exit;
    if (scroll_text("and that I still can't draw. :)")) goto exit;
    if (scroll_text("Also, I just wanna FINISH this.")) goto exit;
    if (scroll_text("I want to put it on a CD my brother")) goto exit;
    if (scroll_text("keeps asking me to make as a surprise.")) goto exit;
    if (scroll_text("Note that the lionesses are actually Scar")) goto exit;
    if (scroll_text("with lighter colour and the mane removed.")) goto exit;
    if (scroll_text("Also, this font is the Windows Times")) goto exit;
    if (scroll_text("font, transcribed to Allegro by")) goto exit;
    if (scroll_text("hand, since someone commented that")) goto exit;
    if (scroll_text("Times was the font in the Lion King")) goto exit;
    if (scroll_text("title. :)")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Not to spoil the ending, but good")) goto exit;
    if (scroll_text("ol' ~2Dr. Wiley~1 was an inspirational")) goto exit;
    if (scroll_text("strike, mostly for my brother, who")) goto exit;
    if (scroll_text("introduced me to ~3Mega Man~1, but also")) goto exit;
    if (scroll_text("the way I keep thinking I should put")) goto exit;
    if (scroll_text("a comment that I realize that")) goto exit;
    if (scroll_text("building robots is more ~2Wile E.")) goto exit;
    if (scroll_text("~2Coyote~1's style than Scar's, then")) goto exit;
    if (scroll_text("realized that only Dr. Wiley makes")) goto exit;
    if (scroll_text("hundreds of the same kind of easily")) goto exit;
    if (scroll_text("defeated robots. ;)")) goto exit;
    if (scroll_text(" ")) goto exit;
    if (scroll_text("Alright, then, Hi! to everyone who")) goto exit;
    if (scroll_text("reads this far. :) And...")) goto exit;
    if (scroll_text("~2Captain C~1 - CD coming RSN ;)")) goto exit;
    if (scroll_text("Hope you like the cameo. ;)")) goto exit;
    if (scroll_text("~3Foxxfire~1 - so are you, to me :)")) goto exit;
    if (scroll_text("~2MY~1 Foxx! Mine! Mine! ;)")) goto exit;
    if (scroll_text("~2Binky~1 - cause you laughed at the")) goto exit;
    if (scroll_text("concept, and cause the pic you did")) goto exit;
    if (scroll_text("in my CF sketchbook was ~3CUTE")) goto exit;
    if (scroll_text("~2Orzel~1 - ~3Lazy~1 Tastyeagle. <tug!>")) goto exit;
    if (scroll_text("Try Yiff! already! :)")) goto exit;
    if (scroll_text("~2Jurrel~1 - See? You get your")) goto exit;
    if (scroll_text("mention. :) And your pause feature,")) goto exit;
    if (scroll_text("too. :) You don't, however, get")) goto exit;
    if (scroll_text("the special ending you requested. ;)")) goto exit;
    if (scroll_text(" ")) goto exit;

  }

  for (i=0; i<10; i++)
    if (scroll_text(" ")) goto exit;

  goto t_start;

exit:
if (key[KEY_ESC])
  return(1);
else
  return(0);

}

int scroll_text(char *s)
{ int i;

  text_mode(black);
  mytextout(s);
  for (i=0; i<40; i++)
  { retrace_count=0;
    blit(work,screen,35,100,35,200,570,250);
    blit(work,work,35,101,35,100,570,350);
    rect(work,35,449,605,451,black);
    call_joystick();
    if ((FIRE)||(key[KEY_ESC])) i=100;
    rand();
    while (retrace_count<2);
  }
  if (i>99)
    return(1);
  else
    return(0);
}

void mytextout(char *s)
{ int col,i,x,y;
  char out[80],*t;

col=253;
i=0;
t=s;
/* build temp string to determine centering */
while (*s)
{ if (*s=='~') s=s+2;
  if (*s)
    out[i++]=*(s++);
}
out[i]=0;
x=320-(text_length(scrollfont,out)/2);
y=351;
s=t;
i=0;
while (*s)
{ /* print string with ~ to indicate colour change 1,2,3 */
  if (*s=='~')
  { /* display current string, update data */
    out[i]=0;
    textout(work,scrollfont,out,x,y,col);
    x=x+text_length(scrollfont,out);
    i=0;
    s++;
    col=*s + 204;  /* (*s - '0' + 253) */
    s++;
  }
  out[i++]=*(s++);
}
out[i]=0;
textout(work,scrollfont,out,x,y,col);
}

void loadhi()
{ /* load hi scores from hi.dat */
FILE *fp;
int i;

fp=fopen("hi.dat","r");
if (fp)
{ for (i=0; i<10; i++)
    fscanf(fp,"%d,%d,%s",&hi_score[i],&hi_level[i],&hi_name[i][0]);
  fclose(fp);
}
else
{ for (i=0; i<10; i++)
  { hi_score[i]=(11-i)*1000;
    hi_level[i]=0;
    strcpy(&hi_name[i][0],"Tursi");
  }
}
}

void savehi()
{ /* save hi scores to hi.dat */
FILE *fp;
int i;

fp=fopen("hi.dat","w");
if (fp)
{ for (i=0; i<10; i++)
    fprintf(fp,"%d,%d,%s\n",hi_score[i],hi_level[i],&hi_name[i][0]);
  fclose(fp);
}
}

void fancy_clear()
{ /* do the Robotron clear */
int i,j,k,c;

play_sample(data[GO].dat,255,128,1000,0);

for (i=10; i<320; i=i+10)
{ retrace_count=0;
  clear(work2);
  j=i;
  c=253;
  while (j>0)
  { k=(j*3)/4;
    rectfill(work2,320-j,240-k,320+j,240+k,c);
    j=j-20;
    c++;
    if (c==256) c=253;
  }
  draw_sprite(screen,work2,0,0);
  while (retrace_count<1);
}

for (i=10; i<320; i=i+10)
{ retrace_count=0;
  k=(i*3)/4;
  rectfill(screen,320-i,240-k,320+i,240+k,black);
  while (retrace_count<1);
}

clear_to_color(work2,black);
for (i=0; i<10; i++)
{ rect(work2,i,i,639-i,479-i,253);
  rect(work2,i+10,i+10,629-i,469-i,254);
  rect(work2,i+20,i+20,619-i,459-i,255);
}

blit(work2,screen,0,0,0,0,640,480);

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
  FRAME++;
  switch (CONTROL)
  { case CONTROL_KEYBOARD: call_keyboard(); break;
    case CONTROL_1BUTTON : call_1button();  break;
    case CONTROL_4BUTTON : call_4button();  break;
    case CONTROL_KEY_JOY : call_keyjoy();   break;
    case CONTROL_JOY_KEY : call_joykey();   break;
  }
  FIRE=((FIREUP)||(FIREDOWN)||(FIRELEFT)||(FIRERIGHT));

  if ((CONTROL==CONTROL_KEYBOARD)||(CONTROL==CONTROL_1BUTTON))
  {  set_fire();
  }

  if (key[KEY_P])
  { textout_centre(screen,data[MyFont].dat,"Pause - R to resume",320,250,254);
    while (!key[KEY_R]);
  }
  if (key[KEY_F1])
  { save_pcx("Takatron.pcx",screen,data[palette].dat);
    while (key[KEY_F1]);
  }
  if ((key[KEY_F5])&&(CHEATING))
  { for (k=0; k<numenemy; k++)
      enemylife[k]=1;
  }
}

int do_game()
{ /* play the actual game, start at the title page */
int an,z,fl,old,zz;
int maxframe,up,down,left,right;
int index,bs;
int shoot_fl,tempyd,tempxd;
int firexd, fireyd;

play_sample(data[Challenge].dat,255,128,1000,0);
retrace_count=0;
while (retrace_count<200);
LOVE=0;
fancy_clear();

level=STARTLEVEL;
Lives=3;
Continues=0;
Score=0;
play_midi(NULL,0);  /* stop MIDI if it's playing */

set_clip(screen,30,0,609,479);

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

shoot_fl=0;

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

  if ((CONTROL==CONTROL_KEYBOARD)||(CONTROL==CONTROL_1BUTTON))
  { firexd=last_xd;
    fireyd=last_yd;
  } else
  { firexd=0;
    fireyd=0;
    if (FIREUP)
      fireyd=-15;
    if (FIREDOWN)
      fireyd=15;
    if (FIRELEFT)
      firexd=-15;
    if (FIRERIGHT)
      firexd=15;
  }

  if (PLASMA)
  { do_plasma(work);
  }
  else
  { blit(work2,work,0,0,0,SCREENOFFY,640,480);
  }

  shoot_fl=!(shoot_fl);
  if ((shoot_fl)&&((firexd)||(fireyd)))
  { for (index=0; index<MAXBULL; index++)
    { if (bullet[index].x==-1)
      { /* new bullet */
        bullet[index].x=px+15;
        bullet[index].y=py+4;
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
    center_mysprite(work,data[bs].dat,bullet[index].x,bullet[index].y,1);
    bullet[index].shape=bs;

  skipskip:
  }

  if (fl)
  { an++;
    if (an>=maxframe) an=1;
    center_mysprite(work,data[fl+an].dat,px,py,z);
    playershape=fl+an;
    old=fl;
  }
  else
  { center_mysprite(work,data[old].dat,px,py,z);
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
    textout_centre(work,data[MyFont].dat,buf,320,150+SCREENOFFY,254);

  blit(work2,work,0,0,0,SCREENOFFY,640,30);

  sprintf(buf2,"Score: %d",Score);
  textout(work,data[Small_Font].dat,buf2,32,2+SCREENOFFY,253);

  sprintf(buf2,"Level: %d",level);
  textout(work,data[Small_Font].dat,buf2,530,2+SCREENOFFY,253);

  buf2[0]=0;
  for (zz=0; zz<Lives; zz++)
    strcat(buf2,"~");
  textout_centre(work,data[Small_Font].dat,buf2,320,2+SCREENOFFY,253);

  set_clip(screen,30,0,609,449);
  blit(work,screen,0,SCREENOFFY,0,0,640,480);
  set_clip(screen,30,30,609,449);

  if (NOCYCLE) vsync();

  set_color(JAGCOL,&JAGNORM);
  set_color(SCARCOL,&SCARNORM);
  set_color(black,&BLACKCOL);

  if (FPS)
  { sprintf(buf,"Retrace: %d",retrace_count);
    if (FRAME>30) FRAME=1;
  }

  while (retrace_count<4);
}

/*** new level ***/

if (level!=50)
{ fancy_clear();
  level++;
  FRAME=0;
  EssCnt=0;
  HOTBULLETS=0;  /* turn off hot bullets */
  set_clip(screen,30,30,609,449);
  blit(screen,work2,30,30,30,30,610,450);
  px=320;
  py=240;
  an=1;
  for (index=0; index<MAXBULL; index++)
    bullet[index].x=-1;

  sprintf(buf,"Level %d",level);
  textout_centre(screen,data[MyFont].dat,buf,320,150,254);

  if (level==11)
  { retrace_count=0;
    play_sample(data[QuiverFear].dat,255,128,1000,0);
    while (retrace_count<130);
  }

  if ((level>=35)&&(left!=simaleft)&&(!PlayAsScar)&&(!PlayAsNala))
  { retrace_count=0;
    while (retrace_count<80);
    play_sample(data[CircleLife].dat,255,128,1000,0);
    retrace_count=0;
    while (retrace_count<150);
    maxframe=5;
    left=simaleft;
    right=simaright;
    up=simaup;
    down=simadown;
    old=left;
  }

  if (level==45)
    play_midi(data[BePrepared].dat,1);

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
      vsync();
    }

    stop_midi();

    retrace_count=0;
    play_sample(data[Surprised].dat,255,128,1000,0);
    while (retrace_count<350);

    set_volume(-1,MIDIvol*25);
    play_midi(data[FinalBoss].dat,1);

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
  blit(work2,work,0,0,0,SCREENOFFY,640,480);
  for (i=0; i<numenemy; i++)
  { if (enemytype[i]!=-1)
      center_mysprite(work,data[enemytype[i]].dat,enemyx[i],enemyy[i],j);
      enemyshape[i]=enemytype[i];
  }
  sprintf(buf,"Level %d",level);
  textout_centre(work,data[MyFont].dat,buf,320,150+SCREENOFFY,254);
  blit(work,screen,0,SCREENOFFY,0,0,640,480);
  while (retrace_count<4);
}
}

void center_mysprite(BITMAP *dest, BITMAP *spr, int x, int y, int scale)
{ /* y= y coordinate
     z= scale factor
     s= sprite bitmap
  */
  int y2;

  y2=(y-((scale*(spr->h))/2))+SCREENOFFY;
  if (y2<0) y2=0;
  my_sprite(dest,spr,x,y2,scale);
}

void draw_enemies()
{ int i,x,et,speed;

  speed=-10;
  for (i=numenemy-1; i>=0; i--)
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
    { center_mysprite(work,data[enemytype[i]+x].dat,enemyx[i],enemyy[i],enemysize[i]);
      enemyshape[i]=enemytype[i]+x;
      if ((LOVE>0)&&(et==type_Lioness))
        center_mysprite(work,data[Love_Heart].dat,enemyx[i],enemyy[i]-12,enemysize[i]);
    }
  }
}

void move_enemies()
{ int i,k,ax,ay,fl;
  int speed;

  speed=(level/10)+1;
  for (i=0; i<numenemy; i++)
  { if (enemysize[i]==1)
    { switch (type(enemytype[i]))
      { case type_Hyena: ax=sgn(px-enemyx[i])*speed;
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
                         break;
        case type_Buzzard: ax=sgn(px-enemyx[i])*(speed*2);
                           ay=sgn(py-enemyy[i])*speed;
                           enemyx[i]+=ax;
                           enemyy[i]+=ay;
                           if (past_target(px,enemyx[i]+ax,ax))
                             ax=0;
                           if (past_target(py,enemyy[i]+ay,ay))
                             ay=0;
                           if (ax<0) enemytype[i]=BuzzardR1;
                           if (ax>0) enemytype[i]=Buzzard1;
                           break;
        case type_Lioness: if (LOVE==0)
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
                           break;
        case type_Growl: ax=sgn(enemytargetx[i]-enemyx[i])*(speed*3);
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
                         break;
        case type_Kitu: ax=sgn(enemytargetx[i]-enemyx[i])*(speed+1);
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
                              play_sample(data[Start_Buzzard].dat,255,128,1000,0);
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
                        break;
        case type_PWR: ax=sgn(px-enemyx[i])*speed;
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
                       break;
        case type_Rhino: ax=sgn(enemytargetx[i]-enemyx[i])*(speed+2);
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
                                   play_sample(data[HIT].dat,255,128,1000,0);
                                   enemysize[fl]=2;
                                 }
                                 enemytargetx[i]=enemyx[fl];
                                 enemytargety[i]=enemyy[fl];
                               }
                             fl++;
                           }
                         }
                         break;
        case type_Scar: ax=sgn(enemytargetx[i]-enemyx[i])*(speed+2);
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
                                  play_sample(data[GROWL].dat,255,128,1000,0);
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
                        break;
        case type_Bullet: enemyx[i]+=enemytargetx[i];
                          enemyy[i]+=enemytargety[i];
                          if ((enemyx[i]<5)||(enemyx[i]>630)||(enemyy[i]<5)||(enemyy[i]>470))
                          { enemyx[i]=-1;
                            enemytype[i]=-1;
                            enemysize[i]=21;
                          }
                          break;
        case type_Jag: ax=sgn(enemytargetx[i]-enemyx[i])*(speed*2)+(speed/2);
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
                         switch (phase) /* this does the first attack */
                         {
                           case 4:
                           case 5:
                           case 6: if (enemytype[i]>=JagL1)
                                     enemytype[i]=JagL4;
                                   else
                                     enemytype[i]=Jag4;
                                   break;

                           case 7:
                           case 8:
                           case 9: if (enemytype[i]>=JagL1)
                                     enemytype[i]=JagL3;
                                   else
                                     enemytype[i]=Jag3;
                                   break;

                           case 10:
                           case 11:
                           case 12: if (enemytype[i]>=JagL1)
                                      enemytype[i]=JagL4;
                                    else
                                      enemytype[i]=Jag4;
                                    break;

                           case 13: if (rand()%3==0) phase=32;
                           case 14: 
                           case 15: if (enemytype[i]>=JagL1)
                                      enemytype[i]=JagL3;
                                    else
                                      enemytype[i]=Jag3;
                                     break;

                           case 16:
                           case 17:
                           case 18: if (enemytype[i]>=JagL1)
                                      enemytype[i]=JagL4;
                                    else
                                      enemytype[i]=Jag4;
                                    break;

                           case 19:
                           case 20:
                           case 21: if (enemytype[i]>=JagL1)
                                      enemytype[i]=JagL3;
                                    else
                                      enemytype[i]=Jag3;
                                     break;

                           case 22: start8way(enemyx[i],enemyy[i]);
                                    break;

                           case 25: start8way(enemyx[i],enemyy[i]);
                                    break;

                           case 28: start8way(enemyx[i],enemyy[i]);
                                    break;

                           case 81:
                           case 31: enemytargetx[i]=px;
                                    enemytargety[i]=py;
                                    phase=0;
                                    break;
                           case 32:
                           case 33:
                           case 34: if (enemytype[i]>=JagL1)
                                      enemytype[i]=JagL1;
                                    else
                                      enemytype[i]=Jag1;
                                     break;

                           case 35:
                           case 36:
                           case 37: if (enemytype[i]>=JagL1)
                                      enemytype[i]=JagL2;
                                    else
                                      enemytype[i]=Jag2;
                                    break;

                           case 38:
                           case 39:
                           case 40: if (enemytype[i]>=JagL1)
                                      enemytype[i]=JagL1;
                                    else
                                      enemytype[i]=Jag1;
                                    break;
                           case 41:
                           case 43:
                           case 45:
                           case 47:
                           case 49:
                           case 51:
                           case 53:
                           case 55:
                           case 57:
                           case 59:
                           case 61:
                           case 63:
                           case 65:
                           case 67:
                           case 69:
                           case 71:
                           case 73:
                           case 75:
                           case 77:
                           case 79: startaimedshot(enemyx[i],enemyy[i]);
                                    break;
                         }
                       }
                       break;
        case type_Wiley: if (phase<30) phase++;
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
                         break;
        case type_ScarAnim: /* phase starts at 31 */
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
                                       play_sample(data[HIT].dat,255,128,1000,0);
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
                            break;
        case type_SBoss:    phase++;
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
                                play_sample(data[Run].dat,255,128,1000,0);
                              } else
                              { Timer=20;
                                play_sample(data[RunAway].dat,255,128,1000,0);
                              }
                            }

                            if ((ax==0)&&(ay==0))
                            { /* reached target */
                              phase=0;
                              enemytargetx[i]=px;
                              enemytargety[i]=py;
                            }
                            break;
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
          { enemytype[k]=FScarBossB;
            enemysize[k]=2;
            enemyx[k]=enemyx[i]+((rand()%10)-5);
            enemyy[k]=enemyy[i]+((rand()%10)-5);
            play_sample(data[BOOM].dat,255,128,1000,0);
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

int myabs(int x)
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
            play_sample(data[HIT].dat,255,128,1000,0);
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
            play_sample(data[HIT].dat,255,128,2000,0);
            enemylife[i]--;
            if (enemylife[i]==0)
            { enemysize[i]=2;
              play_sample(data[BOOM].dat,255,128,1000,0);
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
            set_color(JAGCOL,&JAGWHITE);
          }
          if (t==type_SBoss)
          { bullet[j].x=-1;
            play_sample(data[HIT].dat,255,128,2000,0);
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
                vsync();
              }
              stop_midi();
              set_volume(-1,MIDIvol*25);
              play_sample(data[BOOM].dat,255,128,1000,0);
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
            set_color(SCARCOL,&JAGWHITE);
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
          { case PWR_1up : play_sample(data[Got_1up].dat,255,128,1000,0);
                           Lives++;
                           FRAME=20;
                           strcpy(buf,"Got extra Life!");
                           break;
            case PWR_Continue: play_sample(data[Got_Continue].dat,255,128,1000,0);
                           Continues++;
                           FRAME=20;
                           strcpy(buf,"Got extra Continue!");
                           break;
            case PWR_Love: if (LOVE<1)
                             play_sample(data[FEELLOVE].dat,255,128,1000,0);
                           LOVE=300;
                           FRAME=20;
                           strcpy(buf,"Lioness Love!");
                           break;
            case PWR_Roar: play_sample(data[ROAR].dat, 255, 128,1000,0);
                           for (j=0; j<numenemy; j++)
                             enemysize[j]=2;
                           retrace_count=0;
                           while (retrace_count<50);
                           Score+=500;
                           break;
            case PWR_Warp: play_sample(data[GetOut].dat,255,128,1000,0);
                           retrace_count=0;
                           while (retrace_count<75);
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
                             play_midi(data[BePrepared].dat,1);
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
        { play_sample(data[Get_Lioness].dat,255,128,1000,0);
          enemytype[i]=-1;
          enemysize[i]=21;
          k=1;
          for (j=0; j<numenemy; j++)
            if (type(enemytype[j])==type_Lioness)
              k=0;
          EssCnt++;
          Score+=100*EssCnt;
          if (k)
          { play_sample(data[WOW].dat,255,128,1000,0);
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

  play_sample(data[SIMBADIE].dat,255,128,1000,0);

  for (z=1; z<19; z=z+3)
  { /* scale Simba off */
    retrace_count=0;
    blit(work2,work,0,0,0,SCREENOFFY,640,480);
    center_mysprite(work,data[old].dat,px,py,z);
    draw_enemies();
    blit(work,screen,0,SCREENOFFY,0,0,640,480);
    while (retrace_count<4);
  }
  Lives--;

  /* clear up still-dying enemies and power ups and enemy shots */
  for (z=0; z<numenemy; z++)
  { if ((enemysize[z]!=1)||(type(enemytype[z])==type_PWR)||(type(enemytype[z])==type_Bullet))
    { enemysize[z]=21;
      enemytype[z]=-1;
    }
  }

  while (retrace_count<75);

  if (Lives==2)
  { play_sample(data[NotFair].dat,255,128,1000,0);
    while (retrace_count<150);
  }

  if ((Lives<0)&&(Continues))
  { sprintf(buf2,"Continue? (%d)",Continues);
    textout_centre(screen,data[MyFont].dat,buf2,320,150,254);
    play_sample(data[ComesBack].dat,255,128,1000,0);
    time=9;
    buf[1]=0;
    call_joystick();
    while (FIRE)
      call_joystick();
    while (time>0)
    { retrace_count=0;
      buf[0]=time+'0';
      textout_centre(screen,data[MyFont].dat,buf,320,180,253);
      while (retrace_count<50)
      { call_joystick();
        if (FIRE) time=-1;
      }
      time--;
    }
    if (time<0)
    { retrace_count=0;
      textout_centre(screen,data[MyFont].dat,"              ",320,150,black);
      textout_centre(screen,data[MyFont].dat," ",320,180,black);
      play_sample(data[HesAlive].dat,255,128,1000,0);
      while (retrace_count<250);
      Lives=3;
      Continues--;
    }
    textout_centre(screen,data[MyFont].dat,"              ",320,150,black);
    textout_centre(screen,data[MyFont].dat," ",320,180,black);
  }    
}

void game_over()
{ /* that's all, folks */
  if (level<10)
    play_sample(data[Humiliating].dat,255,128,1000,0);
  else
    play_sample(data[LiveKing].dat,255,128,1000,0);
  textout_centre(screen,data[MyFont].dat,"GAME OVER",320,150,254);
  call_joystick();
  while (FIRE)
    call_joystick();
  retrace_count=0;
  if (Score>hi_score[9]) retrace_count=250;
  while (retrace_count<500)
  { call_joystick();
    if (FIRE) retrace_count=510;
  }
  if (Score>hi_score[9]) new_high();
}

void greet()
{ /* changes the scroll text into a greet text */
GREETING=1;
}

int collide(int s1, int x1a, int y1a, int s2, int x2a, int y2a, int overlap)
{ /* return true if two sprites are touching more than overlap */
  /* y co-ordinates are all centered! */
int r,sx1,sy1,sx2,sy2,x1b,y1b,x2b,y2b;

if ((x1a==-1)||(x2a==-1)) return(0);

r=0;
sx1=((BITMAP*)(data[s1].dat))->w;
sy1=((BITMAP*)(data[s1].dat))->h;
y1a=y1a-(sy1/2);
x1b=x1a+sx1;
y1b=y1a+sy1;
x1a+=overlap;
x1b-=overlap;
if (x1b<=x1a) x1b=x1a+1;
y1a+=overlap;
y1b-=overlap;
if (y1b<=y1a) y1b=y1a+1;

sx2=((BITMAP*)(data[s2].dat))->w;
sy2=((BITMAP*)(data[s2].dat))->h;
y2a=y2a-(sy2/2);
x2b=x2a+sx2;
y2b=y2a+sy2;
x2a+=overlap;
x2b-=overlap;
if (x2b<x2a) x2b=x2a+1;
y2a+=overlap;
y2b-=overlap;
if (y2b<y2a) y2b=y2a+1;

if (debug)
{ rect(screen, x1a, y1a, x1b, y1b, 254);
  rect(screen, x2a, y2a, x2b, y2b, 254);
}

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

play_sample(data[NiceOne].dat, 255, 128,1000,0);
fancy_clear();
textout_centre(screen,data[Times].dat,"Use Keyboard to",320,50,253);
textout_centre(screen,data[Times].dat,"Enter your name",320,100,253);
fl=0;
i=0;
clear_keybuf();
while ((i<10)&&(fl==0))
{ while (!keypressed());
  k=readkey();
  if (k>>8==KEY_ENTER)
    fl=1;
  if (((k>>8==KEY_LEFT)||(k>>8==KEY_BACKSPACE))&&(i>0))
  { i--;
  } else
  k=k&0xff;
  if ((k>=' ')&&(k<='z'))
  { buf[i++]=k;
  }
  rectfill(screen,40,200,600,300,black);
  buf[i]=0;
  textout_centre(screen,data[MyFont].dat,buf,320,200,white);
}
i=9;
while ((Score>hi_score[i-1])&&(i>0))
  i--;
sprintf(buf2,"Number %d score!",i+1);
textout_centre(screen,data[Times].dat,buf2,320,400,254);
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
{ play_sample(data[MyKingdom].dat, 255, 128,1000,0);
}
retrace_count=0;
while (retrace_count<200);
}

void do_config()
{ char buf[10];

  fancy_clear();
  textout_centre(screen,data[MyFont].dat,"CONFIG",320,75,254);
  textout(screen,data[Times].dat,"(C)ontrol:",50,150,white);
  textout(screen,data[Times].dat,"(M)idi Volume:",50,200,white);
  textout(screen,data[Times].dat,"(D)igi Volume:",50,250,white);
  textout(screen,data[Times].dat,"(S)ave and Exit",50,350,253);
  set_volume(DIGIvol*25,MIDIvol*25);
  play_midi(data[LOVEMIDI].dat,1);

  while (!key[KEY_S])
  { switch (CONTROL)
    { case CONTROL_KEYBOARD:
              textout(screen,data[Times].dat,"Keyboard (arrows/space)",260,150,white); break;
      case CONTROL_1BUTTON:
              textout(screen,data[Times].dat,"1 button joystick           ",260,150,white); break;
      case CONTROL_4BUTTON:
              textout(screen,data[Times].dat,"4 button joypad           ",260,150,white); break;
      case CONTROL_KEY_JOY:
              textout(screen,data[Times].dat,"Key move/Joy fire         ",260,150,white); break;
      case CONTROL_JOY_KEY:
              textout(screen,data[Times].dat,"Joy move/Key fire         ",260,150,white); break;
    }
    sprintf(buf,"%d ",MIDIvol);
    textout(screen,data[Times].dat,buf,260,200,white);
    sprintf(buf,"%d ",DIGIvol);
    textout(screen,data[Times].dat,buf,260,250,white);

    if (key[KEY_C])
    { CONTROL++;
      if (CONTROL>CONTROL_JOY_KEY) CONTROL=CONTROL_KEYBOARD;
      while (key[KEY_C]);
    }
    if (key[KEY_M])
    { MIDIvol++;
      if (MIDIvol>10) MIDIvol=0;
      set_volume(-1,MIDIvol*25);
      while (key[KEY_M]);
    }
    if (key[KEY_D])
    { DIGIvol++;
      if (DIGIvol>10) DIGIvol=0;
      set_volume(DIGIvol*25,-1);
      play_sample(data[BOOM].dat,255,128,1000,0);
      while (key[KEY_D]);
    }
  }
  play_midi(NULL,0);
  fp=fopen("takatron.cfg","w");
  if (fp)
  { fprintf(fp,"# Automatically generated file - do not edit.\n");
    fprintf(fp,"Control= %d\n",CONTROL);
    fprintf(fp,"DIGI= %d\n",DIGIvol);
    fprintf(fp,"MIDI= %d\n",MIDIvol);
    fclose(fp);
  }
}

void read_config()
{ int *intp,i;
  char string[85];

  CONTROL=CONTROL_KEYBOARD;
  MIDIvol=10;
  DIGIvol=10;
  intp=NULL;

  fp=fopen("takatron.cfg","r");
  if (fp)
  { while (!feof(fp))
    { fgets(string,80,fp);
      i=0;
      while (string[i]==' ') i++;
      intp=NULL;
      if (toupper(string[i])=='M')
      { intp=&MIDIvol;
      }
      if (toupper(string[i])=='D')
      { intp=&DIGIvol;
      }
      if (toupper(string[i])=='C')
      { intp=&CONTROL;
      }

      if (intp)
      { while ((string[i]!='=')&&(string[i]!=0))
          i++;
        if (string[i])
        { i++;
          *intp=atoi(&string[i]);
        }
      }
    }
    fclose(fp);

    if ((CONTROL<CONTROL_KEYBOARD)||(CONTROL>CONTROL_JOY_KEY)) CONTROL=CONTROL_KEYBOARD;
    if ((MIDIvol<0)||(MIDIvol>10)) MIDIvol=10;
    if ((DIGIvol<0)||(DIGIvol>10)) DIGIvol=10;

    set_volume(DIGIvol*25,MIDIvol*25);
  }
}

void call_keyboard()
{ /* keyboard to move, space to lock firing angle */
if (key[KEY_UP])
  UP=1;
if (key[KEY_DOWN])
  DOWN=1;
if (key[KEY_LEFT])
  LEFT=1;
if (key[KEY_RIGHT])
  RIGHT=1;
if (key[KEY_SPACE])
  FIREUP=1;
}

void call_1button()
{
poll_joystick();
if (joy_up)
  UP=1;
if (joy_down)
  DOWN=1;
if (joy_left)
  LEFT=1;
if (joy_right)
  RIGHT=1;
if ((joy_b1)||(joy_b2))
  FIREUP=1;
}

void call_4button()
{
poll_joystick();
if (joy_up)
  UP=1;
if (joy_down)
  DOWN=1;
if (joy_left)
  LEFT=1;
if (joy_right)
  RIGHT=1;

if (joy_b1)
  FIRELEFT=1;
if (joy_b2)
  FIREUP=1;
if (joy_b3)
  FIREDOWN=1;
if (joy_b4)
  FIRERIGHT=1;
}

void call_keyjoy()
{
if (key[KEY_UP])
  UP=1;
if (key[KEY_DOWN])
  DOWN=1;
if (key[KEY_LEFT])
  LEFT=1;
if (key[KEY_RIGHT])
  RIGHT=1;

poll_joystick();
if (joy_up)
  FIREUP=1;
if (joy_down)
  FIREDOWN=1;
if (joy_left)
  FIRELEFT=1;
if (joy_right)
  FIRERIGHT=1;
}

void call_joykey()
{
poll_joystick();
if (joy_up)
  UP=1;
if (joy_down)
  DOWN=1;
if (joy_left)
  LEFT=1;
if (joy_right)
  RIGHT=1;

if (key[KEY_UP])
  FIREUP=1;
if (key[KEY_DOWN])
  FIREDOWN=1;
if (key[KEY_LEFT])
  FIRELEFT=1;
if (key[KEY_RIGHT])
  FIRERIGHT=1;
}

void set_fire()
{ if (FIREUP)
  { FIREUP=0;
    if (last_yd<0)
      FIREUP=1;
    if (last_yd>0)
      FIREDOWN=1;
    if (last_xd>0)
      FIRERIGHT=1;
    if (last_xd<0)
      FIRELEFT=1;
  }
}

void start8way(int x, int y)
{ /* start 8 enemy shots from x,y, offsetted */
int i,k;

for (i=0; i<8; i++)
{ k=0;
  while ((k<numenemy)&&(enemytype[k]!=-1)) k++;
  if (k<numenemy)
  { enemyx[k]=x+20;
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
  { enemyx[k]=x+20;
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

void do_plasma(BITMAP *bm)
{ /* do a plasma effect */
int xpixel,ypixel;
int pix1,pix2,pix3,pix4,pix5,pix6,pix7,pix8,temp;

for(ypixel = 28+SCREENOFFY+(FRAME%PLASMASTEP); ypixel < 452+SCREENOFFY; ypixel+=PLASMASTEP)
{
  for(xpixel = 29; xpixel < 612; xpixel++)
  {
    pix1=bm->line[ypixel-1][xpixel-1];
    pix2=bm->line[ypixel+1][xpixel-1];
    pix3=bm->line[ypixel-1][xpixel+1];
    pix4=bm->line[ypixel+1][xpixel+1];
    pix5=bm->line[ypixel][xpixel-1];
    pix6=bm->line[ypixel+1][xpixel];
    pix7=bm->line[ypixel-1][xpixel];
    pix8=bm->line[ypixel][xpixel+1];

    temp=(pix1+pix2+pix3+pix4+pix5+pix6+pix7+pix8)>>4; /* average /2 */

    bm->line[ypixel][xpixel]=temp;
  }
}
}

void start4wayA(int x, int y)
{ /* start 4 square enemy shots from x,y, offsetted */
int i,k;

for (i=0; i<4; i++)
{ k=0;
  while ((k<numenemy)&&(enemytype[k]!=-1)) k++;
  if (k<numenemy)
  { enemyx[k]=x+65;
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
  { enemyx[k]=x+65;
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
    if (PLASMA)
    { do_plasma(work);
    }
    else
    { blit(work2,work,0,0,0,SCREENOFFY,640,480);
    }
    if (PlayAsScar)
      center_mysprite(work,data[Scarright1].dat,px,py,1);
    else
      if (PlayAsNala)
        center_mysprite(work,data[essright1].dat,px,py,1);
      else
        center_mysprite(work,data[simaright].dat,px,py,1);
    blit(work,screen,0,SCREENOFFY,0,0,640,480);
    while (retrace_count<4);
  }
  retrace_count=0;
  play_sample(data[AllClear].dat,255,128,1000,0);
  while (retrace_count<350);

  fancy_clear();

  textout_centre(screen,data[MyFont].dat,"CONGRATULATIONS",320,125,254);

  play_midi(data[LOVEMIDI].dat,0);

  clear_to_color(work,black); 
  scrollfont=data[Times].dat;
  
  /* choose an ending ;) */

  index=rand()%4;
  if (PlayAsScar) index=rand()%2+5;
  if (PlayAsNala) index=7;
  if (SEEALLEND) index=0;

  switch (index)
  { case 0: scroll_text("(~2Cheesy ending 1 of 4~1)");
            scroll_text(" ");
            scroll_text("~3Rafiki~1 woke with a start..");
            scroll_text("~2'What a nightmare!'");
            scroll_text("He looked out towards");
            scroll_text("~3Pride Rock~1 where");
            scroll_text("the newborn ~3Simba~1");
            scroll_text("slept peacefully with");
            scroll_text("his parents.");
            scroll_text(" ");
            scroll_text("~2'Ah, well. It was");
            scroll_text("~2only a dream, nothing");
            scroll_text("~2to worry over.'");
            scroll_text(" ");
            if (!SEEALLEND)
              break;
    case 1: scroll_text("(~2Cheesy ending 2 of 4~1)");
            scroll_text(" ");
            scroll_text("~3Simba~1 stood tall over");
            scroll_text("the shattered remains of");
            scroll_text("~3Scar~1's evil devices.");
            scroll_text("He had defeated them all,");
            scroll_text("and once again the");
            scroll_text("~3Pridelands~1 were safe");
            scroll_text("from ~3Scar~1's plots.");
            scroll_text(" ");
            scroll_text("~2'If he'd only used his");
            scroll_text("~2genius for GOOD instead");
            scroll_text("~2of evil...'");
            scroll_text(" ");
            scroll_text("~3Simba~1 shook his head,");
            scroll_text("and turned back towards");
            scroll_text("~3Pride Rock~1, knowing");
            scroll_text("the next confrontation");
            scroll_text("was only a matter of time...");
            scroll_text(" ");
            if (!SEEALLEND)
              break;
    case 2: scroll_text("(~2Cheesy ending 3 of 4~1)");
            scroll_text(" ");
            scroll_text("~3Scar~1 leapt from the");
            scroll_text("exploding debris of his");
            scroll_text("giant ~3Takatron~1, landing");
            scroll_text("on a nearby ledge, leering");
            scroll_text("down at ~3Simba~1.");
            scroll_text(" ");
            scroll_text("~2'I'll get you next time,");
            scroll_text("~3Simba~2. NEXT TIIMEEE!!'");
            scroll_text(" ");
            scroll_text("~3Scar~1 turned, and vanished");
            scroll_text("into a small hole. ~3Simba");
            scroll_text("peered in that direction,");
            scroll_text("wondering.");
            scroll_text(" ");
            scroll_text("~2'That voice... where have I");
            scroll_text("~2heard that voice before?'");
            scroll_text(" ");
            if (!SEEALLEND)
              break;
    case 3: scroll_text("(~2Cheesy ending 4 of 4~1)");
            scroll_text(" ");
            scroll_text("~3MegaMan~1 teleported in as");
            scroll_text("the last echos of the massive");
            scroll_text("explosion faded.");
            scroll_text(" ");
            scroll_text("~2'Great work, Simba! You've");
            scroll_text("~2saved the ~3Pridelands~2!'");
            scroll_text(" ");
            scroll_text("~3Simba~1 nodded.");
            scroll_text(" ");
            scroll_text("~2'And it looks like ~3Scar");
            scroll_text("~2took care of Dr Wiley");
            scroll_text("~2for you.'");
            scroll_text(" ");
            scroll_text("~3Megaman~1 shook his head.");
            scroll_text(" ");
            scroll_text("~2'~3Dr Wiley~2 has survived");
            scroll_text("~2too many times for me");
            scroll_text("~2to believe that. I");
            scroll_text("~2must not let my guard");
            scroll_text("~2down. Neither should");
            scroll_text("~2you.'");
            scroll_text(" ");
            scroll_text("The two shook hand in paw,");
            scroll_text("and wished each other");
            scroll_text("well for the future.");
            scroll_text("With that, ~3Megaman");
            scroll_text("returned to ~3Dr.Light~1,");
            scroll_text("while ~3Simba~1 returned");
            scroll_text("to ~3Pride Rock~1,");
            scroll_text("wondering if he really");
            scroll_text("would meet ~3Scar~1 again...");
            scroll_text(" ");
            if (!SEEALLEND)
              break;
    case 5: scroll_text("(~2Scar's Cheesy Ending 1/2~1)");
            scroll_text(" ");
            scroll_text("~3Scar~1 smiled a feral grin");
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
    case 6: scroll_text("(~2Scar's Cheesy Ending 2/2~1)");
            scroll_text(" ");
            scroll_text("~3Scar~1 stared balefully at");
            scroll_text("the small man in the white frock,");
            scroll_text("as he finished his explanation.");
            scroll_text(" ");
            scroll_text("~2'You see, ~3Scar~2, with the");
            scroll_text("~2help of my robots, the two of");
            scroll_text("~2us can defeat Meg.. err, Mufasa,");
            scroll_text("~2and rule the Pridelands!'");
            scroll_text(" ");
            scroll_text("~3Scar~1's green eyes burned.");
            scroll_text(" ");
            scroll_text("~2'Do you take me for a fool,");
            scroll_text("~3Wiley~2? Machines, ~3Takatrons");
            scroll_text("~2indeed! Now.. your interruption");
            scroll_text("~2has made me lose my lunch...'");
            scroll_text(" ");
            scroll_text("~3Scar~1 advanced on ~3Dr Wiley");
            scroll_text("slowly, stalking him. An instant");
            scroll_text("later he pounced, as the small");
            scroll_text("man cried for help.");
            scroll_text(" ");
            scroll_text("~2'Scar!'~1 called an unfamiliar");
            scroll_text("voice. ~3Scar~1 turned. It was");
            scroll_text("a small blue man. ~2'Drop him.'");
            scroll_text(" ");
            scroll_text("~3Scar~1 shrugged and obliged.");
            scroll_text(" ");
            scroll_text("~2'Impeccable timing, Megaman,'");
            scroll_text("sighed ~3Dr Wiley~1.");
            scroll_text(" ");
            scroll_text("~3Scar~1 watched in amusement as");
            scroll_text("~3Megaman~1 carried ~3Dr Wiley");
            scroll_text("off through the air, till he");
            scroll_text("was no longer visible.");
            scroll_text(" ");
            if (!SEEALLEND)
              break;
    case 7: scroll_text("(~2Nala's Cheesy Ending~1)");
            scroll_text(" ");
            scroll_text("~3Nala~1 fell to the ground as");
            scroll_text("~3Scar~1 was finally defeated,");
            scroll_text("panting for breath. Exhausted,");
            scroll_text("bruised and beaten, but");
            scroll_text("triumphant.");
            scroll_text(" ");
            scroll_text("Sarabi came to her.");
            scroll_text(" ");
            scroll_text("~2'~3Nala~2, it is time for the hunt.'");
            scroll_text(" ");
            scroll_text("~3Nala~1 looked up at ~3Sarabi~1,");
            scroll_text("trying to understand how she");
            scroll_text("could be expected to hunt after");
            scroll_text("all her hard work. She realized");
            scroll_text("quickly, it was not ~3Sarabi~1,");
            scroll_text("it was the way things were, that");
            scroll_text("said she would do the hunting,");
            scroll_text("the birthing and the weaning,");
            scroll_text("the stalking and the killing,");
            scroll_text("the feeding, the guarding...");
            scroll_text(" ");
            scroll_text("~3Nala~1 sighed and got to her");
            scroll_text("feet. It seems a lioness'");
            scroll_text("work is never done...");
            scroll_text(" ");
            break;
  }

  scroll_text(" ");
  scroll_text(" ");
  scroll_text(" ");

  scrollfont=data[MyFont].dat;

  scroll_text("Credits");
  
  scrollfont=data[Times].dat;

  scroll_text(" ");
  if (scroll_text("Takatron: 2098")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("Conceived, designed and")) goto skipcreds;
  if (scroll_text("programmed by")) goto skipcreds;
  if (scroll_text("~2M.Brent~1 (~3Tursi~1)")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("Inspired by crossover")) goto skipcreds;
  if (scroll_text("discussions on")) goto skipcreds;
  if (scroll_text("~2The Lion King Mailing")) goto skipcreds;
  if (scroll_text("~2List~1, hosted at")) goto skipcreds;
  if (scroll_text("~3http://www.lionking.org~3")) goto skipcreds;
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
  if (scroll_text("clips are (C) ~2Disney~1,")) goto skipcreds;
  if (scroll_text("from the movie ~3The Lion")) goto skipcreds;
  if (scroll_text("~3King")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("Most of the artwork is")) goto skipcreds;
  if (scroll_text("from the game ~2The Lion")) goto skipcreds;
  if (scroll_text("~2King~1, published by")) goto skipcreds;
  if (scroll_text("~3Virgin Interactive~1,")) goto skipcreds;
  if (scroll_text("on the ~2Sega Genesis")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("~2Dr Wiley~1 and the")) goto skipcreds;
  if (scroll_text("~3MechaJaguar~1 are from")) goto skipcreds;
  if (scroll_text("~2MegaMan 2~1 by ~3Capcom")) goto skipcreds;
  if (scroll_text("(~2MegaMan~1 and ~2Dr.Light")) goto skipcreds;
  if (scroll_text("are also trademarks of")) goto skipcreds;
  if (scroll_text("~3Capcom~1)")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("Additional artwork by")) goto skipcreds;
  if (scroll_text("~3M.Brent~1, and")) goto skipcreds;
  if (scroll_text("~2David Sauve~1")) goto skipcreds;
  if (scroll_text("(final boss)")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("Sound effects not from")) goto skipcreds;
  if (scroll_text("the movie are believed")) goto skipcreds;
  if (scroll_text("to be ~3Public Domain")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("Be Prepared - Rock")) goto skipcreds;
  if (scroll_text("MIDI by unknown.")) goto skipcreds;
  if (scroll_text("Can You Feel the Love Tonight")) goto skipcreds;
  if (scroll_text("MIDI by unknown.")) goto skipcreds;
  if (scroll_text("AfterBurner - Take Off")) goto skipcreds;
  if (scroll_text("MIDI by unknown.")) goto skipcreds;
  if (scroll_text("Gyruss Bonus Theme")) goto skipcreds;
  if (scroll_text("Sampled from Game")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("The concept of the game")) goto skipcreds;
  if (scroll_text("is based on ~2Williams")) goto skipcreds;
  if (scroll_text("~3Robotron:2084~1, and")) goto skipcreds;
  if (scroll_text("~2Jeff Minter~1's update")) goto skipcreds;
  if (scroll_text("~3Llamatron~1.")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("Special Thanks to:")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("~2Shawn Hargreaves")) goto skipcreds;
  if (scroll_text("for ~3Allegro")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("~2DJ Delorie")) goto skipcreds;
  if (scroll_text("for ~3DJGPP")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("~2David Sauve")) goto skipcreds;
  if (scroll_text("for letting me use the face")) goto skipcreds;
  if (scroll_text("from his ~3Scar 007~1 pictures")) goto skipcreds;
  if (scroll_text("as the final boss."));
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("~2Michael Ponce'")) goto skipcreds;
  if (scroll_text("for his ~3Scar WWW Page")) goto skipcreds;
  if (scroll_text("(where I swiped some")) goto skipcreds;
  if (scroll_text("samples and lots of inspiration)")) goto skipcreds;
  if (scroll_text("~2http://www.cytag.nl/homes/scar/")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("~2Jurrel~1 and ~2Binky")) goto skipcreds;
  if (scroll_text("for ~3suggestions~1 and ~3testing")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("~2Foxxfire")) goto skipcreds;
  if (scroll_text("for ~3suggestions~1,")) goto skipcreds;
  if (scroll_text("~3patience~1 listening to the")) goto skipcreds;
  if (scroll_text("same sounds over and over and over,")) goto skipcreds;
  if (scroll_text("and being my ~3love~1.")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("~2Lawrence Wright")) goto skipcreds;
  if (scroll_text("for having ~3positive~1 things")) goto skipcreds;
  if (scroll_text("to say :)")) goto skipcreds;
  if (scroll_text(" ")) goto skipcreds;
  if (scroll_text("Commandline ~2scar~1 to")) goto skipcreds;
  if (scroll_text("play as ~3Scar")) goto skipcreds;
 
  for (index=10; index>0; index--)
  { if (scroll_text(" ")) goto skipcreds;
    if (index<MIDIvol)
      set_volume(-1,index*25);
  }

skipcreds:

  stop_midi();
  set_volume(-1,MIDIvol*25);

  if (Score>hi_score[9]) new_high();

}
