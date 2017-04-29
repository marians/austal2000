/*
 * verif.h
 *
 *  last change: 2014-01-21
 *  Author: ibj
 */

#ifndef VERIF_H_
#define VERIF_H_

#include "IBJmsg.h"                                               //-2008-09-30
#include "IBJnls.h"
#include "TalCfg.h"

static FILE *Out;
static char LogName[256];

static int init(int argc, char *argv[], char *v) {
  char *lan = "";
  char res[256];
  char *name = "result.txt";
  int nls = 0;
  int trns = -1;
  int i;
  strcpy(LogName, "austal2000.log");
  strcpy(res, "verif/");
  for (i=1; i<argc; i++) {
    if (!strncmp(argv[1], "--language=", 11)) {                   //-2008-09-30
      lan = argv[1] + 11;
    }
    else if (!strncmp(argv[i], "-X", 2)) {                        //-2008-09-30
      sscanf(argv[i]+2, "%d", &trns);
      MsgTranslate = trns;
    }
    else if (!strncmp(argv[i], "-r", 2)) {                        //-2014-01-21
    		name = argv[i]+2;
    }
    else if (!strncmp(argv[i], "-l", 2)) {                        //-2014-01-21
    		strcpy(LogName, argv[i]+2);
    } 
  }
  nls = NlsRead(argv[0], v, lan, NULL);
#ifndef __linux__                                                 //-2008-09-30
  if (!strcmp(NlsLanguage, "de@latin1") && (trns < 0))
    MsgTranslate = 1;
#endif
  //
  strcat(res, name);
  Out = fopen(res, "a");
  if (Out == NULL)  Out = stderr;
  CfgInit(0);
  return nls;
}

#endif /* VERIF_H_ */
