
#include "asmdefs.inc"

#define S_BMP       ARG1
#define S_SPRITE    ARG2
#define S_X         ARG3
#define S_Y         ARG4
#define S_MYSKIP    ARG5

#define S_TGAP   -4(%ebp)
#define S_LGAP   -8(%ebp)
#define S_SGAP   -12(%ebp)
#define S_W      -16(%ebp)
#define S_H      -20(%ebp)
#define S_C      -24(%ebp)
#define S_MASK   -28(%ebp)

/* sets up a sprite draw operation and handles the clipping */
#define START_SPRITE_DRAW(name)                                              \
   pushl %ebp                                                              ; \
   movl %esp, %ebp                                                         ; \
   subl $28, %esp                         /* seven local variables */      ; \
									   ; \
   pushl %edi                                                              ; \
   pushl %esi                                                              ; \
   pushl %ebx                                                              ; \
   pushw %es                                                               ; \
									   ; \
   movl S_BMP, %edx                       /* edx = bitmap pointer */       ; \
   movl S_SPRITE, %esi                    /* esi = sprite pointer */       ; \
									   ; \
   movw BMP_SEG(%edx), %es                /* segment selector */           ; \
									   ; \
   cmpl $0, BMP_CLIP(%edx)                /* test bmp->clip */             ; \
   jz name##_no_clip                                                       ; \
									   ; \
   movl BMP_CT(%edx), %eax                /* bmp->ct */                    ; \
   subl S_Y, %eax                         /* eax -= y */                   ; \
   jge name##_tgap_ok                                                      ; \
   xorl %eax, %eax                                                         ; \
name##_tgap_ok:                                                            ; \
   movl %eax, S_TGAP                      /* set tgap */                   ; \
									   ; \
   movl BMP_H(%esi), %ebx                 /* get height */                 ; \
   imul S_MYSKIP, %ebx                    /* multiply, result in ebx */    ; \
   movl BMP_CB(%edx), %ecx                /* bmp->cb */                    ; \
   subl S_Y, %ecx                         /* ecx -= y */                   ; \
   cmpl %ebx, %ecx                        /* check bottom clipping */      ; \
   jg name##_height_ok                                                     ; \
   movl %ecx, %ebx                                                         ; \
name##_height_ok:                                                          ; \
   subl %eax, %ebx                        /* height -= tgap */             ; \
   jle name##_done                                                         ; \
   movl %ebx, S_H                         /* set h */                      ; \
									   ; \
   movl BMP_CL(%edx), %eax                /* bmp->cl */                    ; \
   subl S_X, %eax                         /* eax -= x */                   ; \
   jge name##_lgap_ok                                                      ; \
   xorl %eax, %eax                                                         ; \
name##_lgap_ok:                                                            ; \
   movl %eax, S_LGAP                      /* set lgap */                   ; \
									   ; \
   movl BMP_W(%esi), %ebx                 /* sprite->w */                  ; \
   movl BMP_CR(%edx), %ecx                /* bmp->cr */                    ; \
   subl S_X, %ecx                         /* ecx -= x */                   ; \
   cmpl %ebx, %ecx                        /* check left clipping */        ; \
   jg name##_width_ok                                                      ; \
   movl %ecx, %ebx                                                         ; \
name##_width_ok:                                                           ; \
   subl %eax, %ebx                        /* width -= lgap */              ; \
   jle name##_done                                                         ; \
   movl %ebx, S_W                         /* set w */                      ; \
									   ; \
   jmp name##_clip_done                                                    ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_no_clip:                                                            ; \
   movl $0, S_TGAP                                                         ; \
   movl $0, S_LGAP                                                         ; \
   movl BMP_W(%esi), %eax                                                  ; \
   movl %eax, S_W                         /* w = sprite->w */              ; \
   movl BMP_H(%esi), %eax                                                  ; \
   movl %eax, S_H                         /* h = sprite->h */              ; \
									   ; \
   .align 4, 0x90                                                          ; \
name##_clip_done:



/* cleans up the stack after a sprite draw operation */
#define END_SPRITE_DRAW()                                                    \
   popw %es                                                                ; \
   popl %ebx                                                               ; \
   popl %esi                                                               ; \
   popl %edi                                                               ; \
   movl %ebp, %esp                                                         ; \
   popl %ebp



/* sets up the inner sprite drawing loop, loads registers, etc */
#define SPRITE_LOOP(name)                                                    \
sprite_y_loop_##name:                                                      ; \
   movl S_Y, %eax                         /* load line */                  ; \
   WRITE_BANK()                           /* select bank */                ; \
   addl S_X, %eax                         /* add x offset */               ; \
   movl S_W, %ecx                         /* x loop counter */             ; \
									   ; \
   .align 4, 0x90                                                          ; \
sprite_x_loop_##name:



/* ends the inner (x) part of a sprite drawing loop */
#define SPRITE_END_X(name)                                                   \
   decl %ecx                                                               ; \
   jg sprite_x_loop_##name



/* ends the outer (y) part of a sprite drawing loop */
#define SPRITE_END_Y(name)                                                   \
   addl S_SGAP, %esi                      /* skip sprite bytes */          ; \
   movl S_MYSKIP, %eax                                                     ; \
   addl %eax, S_Y                         /* next line */                  ; \
   subl %eax, S_H                         /* loop counter */               ; \
   jg sprite_y_loop_##name

.text

/* void my_sprite(BITMAP *bmp, BITMAP *sprite, int x, y, skip);
 *  Draws a sprite onto a linear bitmap at the specified x, y position, 
 *  using a masked drawing mode where zero pixels are not output. Skips
 *  'skip' lines each scanline
 */
.globl _my_sprite

   /* inner loop that copies just the one byte */
   #define LOOP_ONLY_ONE_BYTE                                                \
      SPRITE_LOOP(only_one_byte)                                           ; \
      movb (%esi), %bl                       /* read pixel */              ; \
      testb %bl, %bl                         /* test */                    ; \
      jz only_one_byte_skip                                                ; \
      movb %bl, %es:(%eax)                   /* write */                   ; \
   only_one_byte_skip:                                                     ; \
      incl %esi                                                            ; \
      incl %eax                                                            ; \
      /* no x loop */                                                      ; \
      SPRITE_END_Y(only_one_byte)


   /* inner loop that copies just the one word */
   #define LOOP_ONLY_ONE_WORD                                                \
      SPRITE_LOOP(only_one_word)                                           ; \
      movw (%esi), %bx                       /* read two pixels */         ; \
      testb %bl, %bl                         /* test */                    ; \
      jz only_one_word_skip_1                                              ; \
      movb %bl, %es:(%eax)                   /* write */                   ; \
   only_one_word_skip_1:                                                   ; \
      testb %bh, %bh                         /* test */                    ; \
      jz only_one_word_skip_2                                              ; \
      movb %bh, %es:1(%eax)                  /* write */                   ; \
   only_one_word_skip_2:                                                   ; \
      addl $2, %esi                                                        ; \
      addl $2, %eax                                                        ; \
      /* no x loop */                                                      ; \
      SPRITE_END_Y(only_one_word)


   /* inner loop that copies a word at a time, plus a leftover byte */
   #define LOOP_WORDS_AND_BYTE                                               \
      SPRITE_LOOP(words_and_byte)                                          ; \
      movw (%esi), %bx                       /* read two pixels */         ; \
      testb %bl, %bl                         /* test */                    ; \
      jz words_and_byte_skip_1                                             ; \
      movb %bl, %es:(%eax)                   /* write */                   ; \
   words_and_byte_skip_1:                                                  ; \
      testb %bh, %bh                         /* test */                    ; \
      jz words_and_byte_skip_2                                             ; \
      movb %bh, %es:1(%eax)                  /* write */                   ; \
   words_and_byte_skip_2:                                                  ; \
      addl $2, %esi                                                        ; \
      addl $2, %eax                                                        ; \
      SPRITE_END_X(words_and_byte)           /* end of x loop */           ; \
      movb (%esi), %bl                       /* read pixel */              ; \
      testb %bl, %bl                         /* test */                    ; \
      jz words_and_byte_end_skip                                           ; \
      movb %bl, %es:(%eax)                   /* write */                   ; \
   words_and_byte_end_skip:                                                ; \
      incl %esi                                                            ; \
      incl %eax                                                            ; \
      SPRITE_END_Y(words_and_byte)


   /* inner loop that copies a long at a time */
   #define LOOP_LONGS_ONLY                                                   \
      SPRITE_LOOP(longs_only)                                              ; \
      movl (%esi), %ebx                      /* read four pixels */        ; \
      testb %bl, %bl                         /* test */                    ; \
      jz longs_only_skip_1                                                 ; \
      movb %bl, %es:(%eax)                   /* write */                   ; \
   longs_only_skip_1:                                                      ; \
      testb %bh, %bh                         /* test */                    ; \
      jz longs_only_skip_2                                                 ; \
      movb %bh, %es:1(%eax)                  /* write */                   ; \
   longs_only_skip_2:                                                      ; \
      shrl $16, %ebx                         /* access next two pixels */  ; \
      testb %bl, %bl                         /* test */                    ; \
      jz longs_only_skip_3                                                 ; \
      movb %bl, %es:2(%eax)                  /* write */                   ; \
   longs_only_skip_3:                                                      ; \
      testb %bh, %bh                         /* test */                    ; \
      jz longs_only_skip_4                                                 ; \
      movb %bh, %es:3(%eax)                  /* write */                   ; \
   longs_only_skip_4:                                                      ; \
      addl $4, %esi                                                        ; \
      addl $4, %eax                                                        ; \
      SPRITE_END_X(longs_only)                                             ; \
      /* no cleanup at end of line */                                      ; \
      SPRITE_END_Y(longs_only) 


   /* inner loop that copies a long at a time, plus a leftover word */
   #define LOOP_LONGS_AND_WORD                                               \
      SPRITE_LOOP(longs_and_word)                                          ; \
      movl (%esi), %ebx                      /* read four pixels */        ; \
      testb %bl, %bl                         /* test */                    ; \
      jz longs_and_word_skip_1                                             ; \
      movb %bl, %es:(%eax)                   /* write */                   ; \
   longs_and_word_skip_1:                                                  ; \
      testb %bh, %bh                         /* test */                    ; \
      jz longs_and_word_skip_2                                             ; \
      movb %bh, %es:1(%eax)                  /* write */                   ; \
   longs_and_word_skip_2:                                                  ; \
      shrl $16, %ebx                         /* access next two pixels */  ; \
      testb %bl, %bl                         /* test */                    ; \
      jz longs_and_word_skip_3                                             ; \
      movb %bl, %es:2(%eax)                  /* write */                   ; \
   longs_and_word_skip_3:                                                  ; \
      testb %bh, %bh                         /* test */                    ; \
      jz longs_and_word_skip_4                                             ; \
      movb %bh, %es:3(%eax)                  /* write */                   ; \
   longs_and_word_skip_4:                                                  ; \
      addl $4, %esi                                                        ; \
      addl $4, %eax                                                        ; \
      SPRITE_END_X(longs_and_word)           /* end of x loop */           ; \
      movw (%esi), %bx                       /* read two pixels */         ; \
      testb %bl, %bl                         /* test */                    ; \
      jz longs_and_word_end_skip_1                                         ; \
      movb %bl, %es:(%eax)                   /* write */                   ; \
   longs_and_word_end_skip_1:                                              ; \
      testb %bh, %bh                         /* test */                    ; \
      jz longs_and_word_end_skip_2                                         ; \
      movb %bh, %es:1(%eax)                  /* write */                   ; \
   longs_and_word_end_skip_2:                                              ; \
      addl $2, %esi                                                        ; \
      addl $2, %eax                                                        ; \
      SPRITE_END_Y(longs_and_word)


   /* the actual sprite drawing routine... */
   .align 4
_my_sprite:
   START_SPRITE_DRAW(sprite)

   movl BMP_W(%esi), %eax        /* sprite->w */
   subl S_W, %eax                /* - w */
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   shrl $1, S_W                  /* halve counter for word copies */
   jz sprite_only_one_byte
   jnc sprite_even_bytes

   .align 4, 0x90
   LOOP_WORDS_AND_BYTE           /* word at a time, plus leftover byte */
   jmp sprite_done

   .align 4, 0x90
sprite_even_bytes: 
   shrl $1, S_W                  /* halve counter again, for long copies */
   jz sprite_only_one_word
   jnc sprite_even_words

   .align 4, 0x90
   LOOP_LONGS_AND_WORD           /* long at a time, plus leftover word */
   jmp sprite_done

   .align 4, 0x90
sprite_even_words: 
   LOOP_LONGS_ONLY               /* copy a long at a time */
   jmp sprite_done

   .align 4, 0x90
sprite_only_one_byte: 
   LOOP_ONLY_ONE_BYTE            /* copy just the one byte */
   jmp sprite_done

   .align 4, 0x90
sprite_only_one_word: 
   LOOP_ONLY_ONE_WORD            /* copy just the one word */

   .align 4, 0x90
sprite_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_sprite8() */

.globl _my_sprite_end
   .align 4
_my_sprite_end:
   ret


