// header for native language support
//
extern int NlsCheck;
extern char NlsLanguage[256];                                     //-2008-08-27
extern char NlsLanOption[256];                                    //-2008-12-19
int NlsRead(char *name, char *pgm, char *lan, char *vrs);
char *NlsGetCset(void);
