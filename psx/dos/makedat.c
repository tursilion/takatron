#include <stdio.h>
#include <allegro.h>

#define NUM_SPRITE 128
/* ROTSTEP is a single 45-degree rotation value. 1 degree=4096 */
#define ROTSTEP 184320



struct {
        int tex, x, y, w,h, rotation,sx,sy;
       } sprite[NUM_SPRITE];

RGB *pal;
BITMAP *tt1a, *tt1b, *tt1c, *tt1d, *tt1e, *mousey, *subscreen;
int current,spnum,x1,y1,x2,y2,rot,xflip;
char buf[80];
FILE *fp;

void main(void);
void copyscreen(void);

void main()
{ allegro_init();
  install_keyboard();

  fp=fopen("sprite.dat","rb");
  if (fp)
  { fread((void*)sprite,sizeof(sprite),1,fp);
    fclose(fp);
  }

  sprite[36].w=128;
  sprite[38].w=128;
  sprite[55].sx=-6206;
  sprite[56].sx=-6206;
  sprite[56].sy=6206;

  set_color_depth(32);
  set_gfx_mode(GFX_AUTODETECT,640,480,640,480);

  install_mouse();

  tt1a=load_bmp("tt1a.bmp",pal);
  tt1b=load_bmp("tt1b.bmp",pal);
  tt1c=load_bmp("tt1c.bmp",pal);
  tt1d=load_bmp("tt1d.bmp",pal);
  tt1e=load_bmp("tt1e.bmp",pal);

  current=1;
  spnum=0;
  x1=y1=0;
  x2=y2=-1;
  rot=0;
  xflip=1;

  copyscreen();
  xor_mode(TRUE);
  show_mouse(screen);
  
  while (!key[KEY_ESC])
  { text_mode(0);
    sprintf(buf,"Sprite %d - Rot %d - Flip %d    ",spnum,rot,xflip);
    textout(screen, font, buf, 0, 390, 255);
    vsync();
    show_mouse(NULL);
    if (x1!=-1)
      putpixel(screen, x1, y1, 255);
    if (x2!=-1)
      rect(screen, x1, y1, x2, y2, 255);
    vsync();
    vsync();
    if (x1!=-1)
      putpixel(screen, x1, y1, 255);
    if (x2!=-1)
      rect(screen, x1, y1, x2, y2, 255);
    show_mouse(screen);

    if (key[KEY_A])
    { xflip=-1;
      while (key[KEY_A]);
    }

    if (key[KEY_S])
    { xflip=1;
      while (key[KEY_S]);
    }

    if (mouse_b&1)
    { x2=y2=-1;
      x1=mouse_x;
      y1=mouse_y;
      set_mouse_sprite(NULL);

      while (mouse_b&1);
    }
    if (mouse_b&2)
    { x2=mouse_x;
      y2=mouse_y;
      while (mouse_b&2);
    }

    if (key[KEY_C])
    { current=sprite[spnum].tex;
      x1=sprite[spnum].x;
      y1=sprite[spnum].y;
      x2=sprite[spnum].w+x1-1;
      y2=sprite[spnum].h+y1-1;
      rot=sprite[spnum].rotation/ROTSTEP;
      xflip=sprite[spnum].sx/4096;

      copyscreen();
      if (mousey) destroy_bitmap(mousey);
      if (subscreen) destroy_bitmap(subscreen);

      mousey=create_bitmap(sprite[spnum].w,sprite[spnum].h);
      subscreen=create_bitmap(sprite[spnum].w,sprite[spnum].h);
      blit(screen,mousey,x1,y1,0,0,sprite[spnum].w,sprite[spnum].h);
      if (xflip==1)
        rotate_sprite(subscreen, mousey, 0, 0, itofix(32*rot));
      else
        draw_sprite_h_flip(subscreen, mousey, 0, 0);

      set_mouse_sprite(subscreen);
    }

    if (key[KEY_ENTER])
    {
      sprite[spnum].tex=current;
      sprite[spnum].x=x1;
      sprite[spnum].y=y1;
      sprite[spnum].w=x2-x1+1;
      sprite[spnum].h=y2-y1+1;
      sprite[spnum].rotation=rot*ROTSTEP;
      sprite[spnum].sx=xflip*4096;
      sprite[spnum].sy=4096;
      if (mousey) destroy_bitmap(mousey);
      if (subscreen) destroy_bitmap(subscreen);

      mousey=create_bitmap(sprite[spnum].w,sprite[spnum].h);
      subscreen=create_bitmap(sprite[spnum].w,sprite[spnum].h);
      blit(screen,mousey,x1,y1,0,0,sprite[spnum].w,sprite[spnum].h);
      if (xflip==1)
        rotate_sprite(subscreen, mousey, 0, 0, itofix(32*rot));
      else
        draw_sprite_h_flip(subscreen, mousey, 0, 0);

      set_mouse_sprite(subscreen);
    }

    if (key[KEY_1])
    { current=1;
      copyscreen();
      while (key[KEY_1]);
    }
    if (key[KEY_2])
    { current=2;
      copyscreen();
      while (key[KEY_2]);
    }
    if (key[KEY_3])
    { current=3;
      copyscreen();
      while (key[KEY_3]);
    }
    if (key[KEY_4])
    { current=4;
      copyscreen();
      while (key[KEY_4]);
    }

    if (key[KEY_5])
    { current=5;
      copyscreen();
      while (key[KEY_5]);
    }

    if (key[KEY_LEFT])
    { if (spnum>0) spnum--;
      while (key[KEY_LSHIFT]);
    }

    if (key[KEY_RIGHT])
    { if (spnum<127) spnum++;
      while (key[KEY_LSHIFT]);
    }
    if (key[KEY_UP])
    { if (rot<7) rot++;
      while (key[KEY_LSHIFT]);
    }
    if (key[KEY_DOWN])
    { if (rot>0) rot--;
      while (key[KEY_LSHIFT]);
    }
  }

  fp=fopen("sprite.dat","wb");
  printf("Size: %d\n\n",sizeof(sprite));
  fwrite((void*)sprite,sizeof(sprite),1,fp);
  fclose(fp);
}

void copyscreen()
{ show_mouse(NULL);
  clear(screen);
  switch (current)
  { case 1: blit(tt1a,screen,0,0,0,0,255,255); break;
    case 2: blit(tt1b,screen,0,0,0,0,255,255); break;
    case 3: blit(tt1c,screen,0,0,0,0,255,255); break;
    case 4: blit(tt1d,screen,0,0,0,0,255,255); break;
    case 5: blit(tt1e,screen,0,0,0,0,255,255); break;
  }
  show_mouse(screen);
}
