// configuration data
//
// last change: 2014-01-21 uj

#include <stddef.h>
#include <string.h>

#include "TalCfg.h"
#include "TalCfg.nls"

char *CfgLanguage;
char *CfgDepString;                                               //-2011-11-23
char *CfgDryString;                                               //-2011-11-23
char *CfgWetString;                                               //-2011-11-23
char *CfgDayString;
char *CfgIndString;
char *CfgAddString;
char *CfgDevString;
char *CfgYearString;
char *CfgHourString;
char *CfgMntAddString;
char *CfgMntBckString;
char *CfgMntSctString;
char *CfgSeriesString;
int  CfgWet=0;                                                    //-2014-01-21

static char string[256];

char *CfgString() {
  strcpy(string, "/");
  strcat(string, _language_);
  strcat(string, "/");
  strcat(string, _add_string_);
  strcat(string, "/");
  strcat(string, _dev_string_);
  strcat(string, "/");
  strcat(string, _hour_string_);
  strcat(string, "/");
  strcat(string, _day_string_);
  strcat(string, "/");
  strcat(string, _ind_string_);
  strcat(string, "/");
  strcat(string, _year_string_);
  strcat(string, "/");
  strcat(string, _mnt_add_string_);
  strcat(string, "/");
  strcat(string, _mnt_sct_string_);
  strcat(string, "/");
  strcat(string, _mnt_bck_string_);
  strcat(string, "/");
  strcat(string, _series_);
  strcat(string, "/");
  strcat(string, (CfgWet) ? "1" : "0");                           //-2014-01-21
  return string;
}

void CfgInit(int wet) {
  CfgLanguage=_language_;
  CfgDepString = _dep_string_;                                    //-2011-12-13
  CfgDryString = _dry_string_;                                    //-2011-11-23
  CfgWetString = _wet_string_;                                    //-2011-11-23
  CfgDayString = _day_string_;
  CfgIndString = _ind_string_;
  CfgAddString = _add_string_;
  CfgDevString = _dev_string_;
  CfgYearString = _year_string_;
  CfgHourString = _hour_string_;
  CfgMntAddString = _mnt_add_string_;
  CfgMntBckString = _mnt_bck_string_;
  CfgMntSctString = _mnt_sct_string_;
  CfgSeriesString = _series_;
  CfgWet = wet;                                                   //-2014-01-21
}

