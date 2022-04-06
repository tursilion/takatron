#include <libps.h>

struct EXEC *myexec;

char r;

void main()
{ printf("reading..\n");
  myexec=CdReadExec("\\TAKATRON.EXE");
  printf("Myexec: %Lx\n",myexec);
  printf("Waiting..\n");
  if (CdReadSync(0,NULL)==-1) printf("Didn't load.");
  printf("Starting.\n");
  EnterCriticalSection();
  printf("%Lx\n",myexec->pc0);
  Exec(myexec,1,NULL);
}
