#include <libps.h>
#include "pad.h"

// low-level pad buffers: never need to touch
volatile u_char *bb0, *bb1;

// call once only in program initialisation
volatile u_char *PadInit() {
	GetPadBuf(&bb0, &bb1);
    return(bb0);
}

// call once per VSync(0)
// puts controller pad status into unsigned long integer
u_long PadRead(void) {
	return(~(*(bb0+3) | *(bb0+2) << 8 | *(bb1+3) << 16 | *(bb1+2) << 24));
}

