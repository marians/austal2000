//=========================================================== IBJgk.c
//
// Gauss-Krueger Routines for IBJ-programs
// =======================================
//
// Copyright (C) Umweltbundesamt, Dessau-Roﬂlau, Germany, 2002
// Copyright (C) Janicke Consulting, 88662 ‹berlingen, Germany, 2002-2011
// email: info@austal2000.de
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of
// the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//-------------------------------------------------------------------
// The formulas used are from
//     Geodaetische Rechnungen und Abbildungen in der Landesvermessung
//     by Walter Grossmann
//     Verlag Konrad Wittwer, Stuttgart 1976
//-------------------------------------------------------------------
//
// last change 2011-06-17 uj
//
// 2011-06-17  source code in ANSI, deg separator "-" instead of circle
//
//===================================================================

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "IBJgk.h"

#define Nb  17

static int DEBUG = 0;

// static double BesselA = 6377397.155;
// static double BesselB = 6356078.963;
// static double FirstB0 = 47;
// static double StepB0 = 0.5;

static double Coeff4B[Nb][10] = {
    { 47.0, 5206717.123, 3238.62181, 0.25484, 27.18030, 0.00019, 0.84991, 0.01417, 0.00470, 0.00027 },
    { 47.5, 5262298.750, 3238.33873, 0.25446, 27.65713, 0.00023, 0.86595, 0.01469, 0.00482, 0.00028 },
    { 48.0, 5317885.232, 3238.05609, 0.25400, 28.14308, 0.00028, 0.88259, 0.01525, 0.00498, 0.00029 },
    { 48.5, 5373476.562, 3237.77398, 0.25347, 28.63850, 0.00032, 0.89986, 0.01582, 0.00514, 0.00030 },
    { 49.0, 5429072.731, 3237.49247, 0.25285, 29.14373, 0.00037, 0.91779, 0.01643, 0.00532, 0.00032 },
    { 49.5, 5484673.728, 3237.21167, 0.25217, 29.65919, 0.00042, 0.93641, 0.01707, 0.00550, 0.00033 },
    { 50.0, 5540279.542, 3236.93165, 0.25140, 30.18522, 0.00046, 0.95576, 0.01774, 0.00569, 0.00035 },
    { 50.5, 5595890.159, 3236.65250, 0.25056, 30.72228, 0.00051, 0.97588, 0.01844, 0.00588, 0.00036 },
    { 51.0, 5651505.564, 3236.37430, 0.24964, 31.27080, 0.00055, 0.99679, 0.01918, 0.00609, 0.00038 },
    { 51.5, 5707125.742, 3236.09714, 0.24865, 31.83124, 0.00060, 1.01855, 0.01995, 0.00630, 0.00040 },
    { 52.0, 5762750.674, 3235.82110, 0.24758, 32.40406, 0.00064, 1.04120, 0.02077, 0.00653, 0.00042 },
    { 52.5, 5818380.341, 3235.54627, 0.24644, 32.98980, 0.00069, 1.06479, 0.02163, 0.00677, 0.00044 },
    { 53.0, 5874014.722, 3235.27273, 0.24522, 33.58897, 0.00073, 1.08936, 0.02254, 0.00702, 0.00046 },
    { 53.5, 5929653.796, 3235.00056, 0.24393, 34.20215, 0.00078, 1.11498, 0.02350, 0.00729, 0.00049 },
    { 54.0, 5985297.539, 3234.72985, 0.24257, 34.82995, 0.00082, 1.14169, 0.02451, 0.00756, 0.00051 },
    { 54.5, 6040945.925, 3234.46067, 0.24113, 35.47298, 0.00087, 1.16956, 0.02558, 0.00786, 0.00054 },
    { 55.0, 6096598.929, 3234.19312, 0.23962, 36.13191, 0.00091, 1.19867, 0.02671, 0.00817, 0.00057 }
  };

static double Coeff4L[Nb][9] = {
    { 47.0, 4733.92902,  79.45949, 1.91545, 0.63848, 0.03862, 0.03862, 0.00163, 0.00016 },
    { 47.5, 4778.69235,  81.62513, 1.98139, 0.66046, 0.04049, 0.04049, 0.00173, 0.00017 },
    { 48.0, 4824.68415,  83.86604, 2.05054, 0.68351, 0.04247, 0.04247, 0.00184, 0.00018 },
    { 48.5, 4871.94748,  86.18590, 2.12311, 0.70770, 0.04457, 0.04457, 0.00195, 0.00019 },
    { 49.0, 4920.52752,  88.58865, 2.19930, 0.73310, 0.04681, 0.04681, 0.00207, 0.00021 },
    { 49.5, 4970.47175,  91.07844, 2.27934, 0.75978, 0.04918, 0.04918, 0.00220, 0.00022 },
    { 50.0, 5021.82999,  93.65970, 2.36346, 0.78782, 0.05170, 0.05170, 0.00234, 0.00023 },
    { 50.5, 5074.65466,  96.33718, 2.45194, 0.81731, 0.05439, 0.05439, 0.00249, 0.00025 },
    { 51.0, 5129.00084,  99.11582, 2.54505, 0.84835, 0.05725, 0.05725, 0.00265, 0.00026 },
    { 51.5, 5184.92656, 102.00101, 2.64309, 0.88103, 0.06030, 0.06030, 0.00283, 0.00028 },
    { 52.0, 5242.49287, 104.99841, 2.74640, 0.91547, 0.06355, 0.06355, 0.00302, 0.00030 },
    { 52.5, 5301.76414, 108.11409, 2.85533, 0.95178, 0.06703, 0.06703, 0.00323, 0.00032 },
    { 53.0, 5362.80823, 111.35456, 2.97027, 0.99009, 0.07074, 0.07074, 0.00345, 0.00034 },
    { 53.5, 5425.69674, 114.72673, 3.09163, 1.03054, 0.07471, 0.07471, 0.00369, 0.00037 },
    { 54.0, 5490.50527, 118.23801, 3.21985, 1.07328, 0.07897, 0.07897, 0.00395, 0.00039 },
    { 54.5, 5557.31373, 121.89635, 3.35545, 1.11848, 0.08352, 0.08352, 0.00424, 0.00042 },
    { 55.0, 5626.20658, 125.71029, 3.49892, 1.16631, 0.08841, 0.08841, 0.00455, 0.00046 }
  };

static double Coeff4X[Nb][9] = {
    { 47.0, 30877.3317, 0.7502, 37.449895, 0.00015, 0.024786, 0.001760, 0.0001323, 0.000002218 },
    { 47.5, 30880.0309, 0.7492, 37.399577, 0.00019, 0.031121, 0.001759, 0.0001283, 0.000002229 },
    { 48.0, 30882.7263, 0.7482, 37.337862, 0.00023, 0.037449, 0.001757, 0.0001242, 0.000002240 },
    { 48.5, 30885.4172, 0.7468, 37.264767, 0.00028, 0.043765, 0.001754, 0.0001202, 0.000002249 },
    { 49.0, 30888.1027, 0.7451, 37.180315, 0.00032, 0.050068, 0.001750, 0.0001161, 0.000002256 },
    { 49.5, 30890.7820, 0.7434, 37.084530, 0.00036, 0.056357, 0.001746, 0.0001120, 0.000002260 },
    { 50.0, 30893.4543, 0.7413, 36.977440, 0.00040, 0.062630, 0.001742, 0.0001079, 0.000002260 },
    { 50.5, 30896.1188, 0.7389, 36.859076, 0.00045, 0.068883, 0.001736, 0.0001038, 0.000002255 },
    { 51.0, 30898.7746, 0.7364, 36.729473, 0.00049, 0.075116, 0.001730, 0.0000997, 0.000002249 },
    { 51.5, 30901.4210, 0.7337, 36.588670, 0.00053, 0.081326, 0.001722, 0.0000956, 0.000002242 },
    { 52.0, 30904.0571, 0.7308, 36.436711, 0.00058, 0.087512, 0.001714, 0.0000916, 0.000002234 },
    { 52.5, 30906.6821, 0.7276, 36.273638, 0.00062, 0.093674, 0.001706, 0.0000875, 0.000002223 },
    { 53.0, 30909.2953, 0.7241, 36.099504, 0.00065, 0.099806, 0.001698, 0.0000835, 0.000002209 },
    { 53.5, 30911.8958, 0.7204, 35.914356, 0.00069, 0.105908, 0.001690, 0.0000795, 0.000002193 },
    { 54.0, 30914.4828, 0.7166, 35.718253, 0.00075, 0.111980, 0.001681, 0.0000756, 0.000002173 },
    { 54.5, 30917.0555, 0.7125, 35.511254, 0.00078, 0.118016, 0.001671, 0.0000717, 0.000002151 },
    { 55.0, 30919.6132, 0.7083, 35.293418, 0.00081, 0.124017, 0.001661, 0.0000678, 0.000002128 }
  };

static double Coeff4Y[Nb][9] = {
    { 47.0, 21124.1021, 109.48178, 0.250142, 0.005652, 0.000422, 0.0007735, 0.0000045, 0.000000303 },
    { 47.5, 20926.2268, 110.37825, 0.247856, 0.007030, 0.000425, 0.0007572, 0.0000046, 0.000000301 },
    { 48.0, 20726.7454, 111.26639, 0.245550, 0.008378, 0.000429, 0.0007403, 0.0000047, 0.000000300 },
    { 48.5, 20525.6728, 112.14618, 0.243224, 0.009695, 0.000433, 0.0007232, 0.0000048, 0.000000298 },
    { 49.0, 20323.0242, 113.01758, 0.240878, 0.010981, 0.000437, 0.0007056, 0.0000049, 0.000000296 },
    { 49.5, 20118.8147, 113.88048, 0.238513, 0.012235, 0.000440, 0.0006877, 0.0000050, 0.000000293 },
    { 50.0, 19913.0596, 114.73486, 0.236128, 0.013457, 0.000444, 0.0006694, 0.0000051, 0.000000290 },
    { 50.5, 19705.7744, 115.58059, 0.233723, 0.014645, 0.000448, 0.0006509, 0.0000051, 0.000000287 },
    { 51.0, 19496.9748, 116.41764, 0.231299, 0.015799, 0.000451, 0.0006320, 0.0000052, 0.000000283 },
    { 51.5, 19286.6763, 117.24591, 0.228856, 0.016920, 0.000454, 0.0006128, 0.0000053, 0.000000279 },
    { 52.0, 19074.8948, 118.06536, 0.226395, 0.018005, 0.000458, 0.0005934, 0.0000054, 0.000000275 },
    { 52.5, 18861.6463, 118.87595, 0.223914, 0.019055, 0.000461, 0.0005737, 0.0000055, 0.000000270 },
    { 53.0, 18646.9468, 119.67752, 0.221414, 0.020070, 0.000465, 0.0005537, 0.0000055, 0.000000265 },
    { 53.5, 18430.8126, 120.47008, 0.218897, 0.021049, 0.000469, 0.0005335, 0.0000056, 0.000000260 },
    { 54.0, 18213.2600, 121.25356, 0.216362, 0.021991, 0.000472, 0.0005131, 0.0000056, 0.000000255 },
    { 54.5, 17994.3053, 122.02786, 0.213806, 0.022896, 0.000475, 0.0004925, 0.0000057, 0.000000250 },
    { 55.0, 17773.9652, 122.79297, 0.211237, 0.023763, 0.000479, 0.0004716, 0.0000058, 0.000000244 }
  };

static int Init;
static double G  [Nb];
static double B0 [Nb];
static double b10[Nb];
static double b20[Nb];
static double b02[Nb];
static double b30[Nb];
static double b12[Nb];
static double b22[Nb];
static double b04[Nb];
static double b14[Nb];
static double b01[Nb];
static double b11[Nb];
static double b21[Nb];
static double b03[Nb];
static double b31[Nb];
static double b13[Nb];
static double b23[Nb];
static double b05[Nb];
static double a10[Nb];
static double a20[Nb];
static double a02[Nb];
static double a30[Nb];
static double a12[Nb];
static double a22[Nb];
static double a04[Nb];
static double a14[Nb];
static double a01[Nb];
static double a11[Nb];
static double a21[Nb];
static double a03[Nb];
static double a31[Nb];
static double a13[Nb];
static double a23[Nb];
static double a05[Nb];

static void initialize() {
  int i;
  for (i=0; i<Nb; i++) {
    B0[i]  =  Coeff4B[i][0];
    G[i]   =  Coeff4B[i][1];
    b10[i] =  Coeff4B[i][2];
    b20[i] = -Coeff4B[i][3];
    b02[i] = -Coeff4B[i][4];
    b30[i] =  Coeff4B[i][5];
    b12[i] = -Coeff4B[i][6];
    b22[i] = -Coeff4B[i][7];
    b04[i] =  Coeff4B[i][8];
    b14[i] =  Coeff4B[i][9];
    b01[i] =  Coeff4L[i][1];
    b11[i] =  Coeff4L[i][2];
    b21[i] =  Coeff4L[i][3];
    b03[i] = -Coeff4L[i][4];
    b31[i] =  Coeff4L[i][5];
    b13[i] = -Coeff4L[i][6];
    b23[i] = -Coeff4L[i][7];
    b05[i] =  Coeff4L[i][8];
    a10[i] =  Coeff4X[i][1];
    a20[i] =  Coeff4X[i][2];
    a02[i] =  Coeff4X[i][3];
    a30[i] = -Coeff4X[i][4];
    a12[i] = -Coeff4X[i][5];
    a22[i] = -Coeff4X[i][6];
    a04[i] =  Coeff4X[i][7];
    a14[i] = -Coeff4X[i][8];
    a01[i] =  Coeff4Y[i][1];
    a11[i] = -Coeff4Y[i][2];
    a21[i] = -Coeff4Y[i][3];
    a03[i] = -Coeff4Y[i][4];
    a31[i] =  Coeff4Y[i][5];
    a13[i] = -Coeff4Y[i][6];
    a23[i] =  Coeff4Y[i][7];
    a05[i] = -Coeff4Y[i][8];
  }
  Init = 1;
}

//===================================================================== GKgetLatitude
//
double GKgetLatitude(   // get geographical latitude from Gauss-Krueger
  double rechts,        // Gauss-Krueger Rechts-value
  double hoch )         // Gauss-Krueger Hoch-value
{
  double dx, dy, dB, lat, zone;
  int i;
  if (!Init)  initialize();
  zone = floor(rechts*1.e-6);
  if (zone<1 || zone>5)  return 0;
  for (i=1; i<Nb; i++)  if (hoch < 0.5*(G[i-1]+G[i]))  break;
  i--;
  if (DEBUG) printf("i=%d\n", i);
  dy = rechts - 1.e6*zone - 0.5e6;
  dx = hoch - G[i];
  if (DEBUG) printf("dx=%1.5lf, dy=%1.5lf\n", dx, dy);
  dy *= 1.e-5;
  dx *= 1.e-5;
  dB = b10[i]*dx + b20[i]*dx*dx + b02[i]*dy*dy + b30[i]*dx*dx*dx
    + b12[i]*dx*dy*dy + b22[i]*dx*dx*dy*dy + b04[i]*dy*dy*dy*dy;
  if (DEBUG) printf("dB=%1.5lf\n", dB);
  lat = B0[i] + dB/3600;
  return lat;
}

//======================================================================== GKgetLongitude
//
double GKgetLongitude(  // get geographical longitude from Gauss-Krueger 
  double rechts,        // Gauss-Krueger Rechts-value 
  double hoch )         // Gauss-Krueger Hoch-value 
{
  double dx, dy, l, lon, zone;
  int i;
  if (!Init)  initialize();
  zone = floor(rechts*1.e-6);
  if (zone<1 || zone>5)  return 0;
  dy = rechts - 1.e6*zone - 0.5e6;
  for (i=1; i<Nb; i++)  if (hoch < 0.5*(G[i-1]+G[i]))  break;
  i--;
  if (DEBUG) printf("i=%d\n", i);
  dx = hoch - G[i];
  if (DEBUG) printf("dx=%1.5lf, dy=%1.5lf\n", dx, dy);
  dy *= 1.e-5;
  dx *= 1.e-5;
  l = b01[i]*dy + b11[i]*dx*dy + b21[i]*dx*dx*dy + b03[i]*dy*dy*dy
    + b31[i]*dx*dx*dx*dy + b13[i]*dx*dy*dy*dy + b23[i]*dx*dx*dy*dy*dy + b05[i]*dy*dy*dy*dy*dy;
  lon = 3*zone + l/3600;
  return lon;
}

//======================================================================= GKgetGEOpoint
//
GEOpoint GKgetGEOpoint( // get geographical point from Gauss-Krueger
  double rechts,        // Gauss-Krueger Rechts-value 
  double hoch )         // Gauss-Krueger Hoch-value 
{
  GEOpoint geo;
  geo.lon = GKgetLongitude(rechts, hoch);
  geo.lat = GKgetLatitude(rechts, hoch);
  return geo;
}

//======================================================================= GKgetRechts
//
double GKgetRechts(     // get Gauss-Krueger Rechts from geographical coord.
  double lon,           // geographical longitude (eastern)
  double lat )          // geographical latitude
{
  double db, dl, dy, rechts, zone;
  int i;
  if (!Init)  initialize();
  zone = floor(lon/3 + 0.5);
  if (zone<1 || zone>5)  return 0;
  for (i=1; i<Nb; i++)  if (lat < 0.5*(B0[i-1]+B0[i]))  break;
  i--;
  db = lat - B0[i];
  dl = lon - 3*zone;
  db *= 3.6;
  dl *= 3.6;
  dy = a01[i]*dl + a11[i]*db*dl + a21[i]*db*db*dl + a03[i]*dl*dl*dl
    + a31[i]*db*db*db*dl + a13[i]*db*dl*dl*dl + a05[i]*dl*dl*dl*dl*dl;
  rechts = 1.e6*zone + 0.5e6 + dy;
  return rechts;
}

//========================================================================= GKgetHoch
//
double GKgetHoch(       // get Gauss-Krueger Hoch from geographical coord. 
  double lon,           // geographical longitude (eastern) 
  double lat )          // geographical latitude
{
  double db, dl, dx, hoch, zone;
  int i;
  if (!Init)  initialize();
  zone = floor(lon/3 + 0.5);
  if (zone<1 || zone>5)  return 0;
  for (i=1; i<Nb; i++)  if (lat < 0.5*(B0[i-1]+B0[i]))  break;
  i--;
  db = lat - B0[i];
  dl = lon - 3*zone;
  db *= 3.6;
  dl *= 3.6;
  dx = a10[i]*db + a20[i]*db*db + a02[i]*dl*dl + a30[i]*db*db*db
    + a12[i]*db*dl*dl + a22[i]*db*db*dl*dl + a04[i]*dl*dl*dl*dl;
  hoch = G[i] + dx;
  return hoch;
}

//======================================================================== GKgetGKpoint
//
GKpoint GKgetGKpoint(   // get Gauss-Krueger point from geographical coord.  
  double lon,           // geographical longitude (eastern) 
  double lat )          // geographical latitude
{
  GKpoint point;
  point.rechts = GKgetRechts(lon, lat);
  point.hoch   = GKgetHoch(lon, lat);
  return point;
}

//========================================================================== GKxGK
//
GKpoint GKxGK(          // transform Gauss-Krueger between zones
  int newz,             // new zone index
  double rechts,        // Gauss-Krueger Rechts-value  
  double hoch )         // Gauss-Krueger Hoch-value 
{
  double db, dl, dx, dy, zone;
  GEOpoint geo;
  GKpoint gk = { 0, 0 };
  int i;
  if (newz<1 || newz>5)  return gk;
  zone = floor(rechts*1.e-6);
  if (zone<1 || zone>5)  return gk;
  if (zone == newz) {
    gk.rechts = rechts;
    gk.hoch = hoch;
    return gk;
  }
  geo = GKgetGEOpoint(rechts, hoch);
  for (i=1; i<Nb; i++)  if (geo.lat < 0.5*(B0[i-1]+B0[i]))  break;
  i--;
  db = geo.lat - B0[i];
  dl = geo.lon - 3*newz;
  db *= 3.6;
  dl *= 3.6;
  dy = a01[i]*dl + a11[i]*db*dl + a21[i]*db*db*dl + a03[i]*dl*dl*dl
    + a31[i]*db*db*db*dl + a13[i]*db*dl*dl*dl + a05[i]*dl*dl*dl*dl*dl;
  gk.rechts = 1.e6*newz + 0.5e6 + dy;
  dx = a10[i]*db + a20[i]*db*db + a02[i]*dl*dl + a30[i]*db*db*db
    + a12[i]*db*dl*dl + a22[i]*db*db*dl*dl + a04[i]*dl*dl*dl*dl;
  gk.hoch = G[i] + dx;
  return gk;
}

//=============================================================== GKgetString
//
char *GKgetString(      // convert degree into string representation
  double d )            // degree value
{
  double deg, min, sec;
  double frc;
  static char s[80], t[8];
  if (d <= 0)  return "0-00\'00.00\"";                            //-2011-06-17
  deg = floor(d);
  d -= deg;
  d *= 60;
  min = floor(d);
  d -= min;
  d *= 60;
  sec = floor(d);
  frc = d - sec;
  sprintf(t, "%4.2lf", frc);
  if (*t == '1') {
    strcpy(t, "0.00");
    sec++;
    if (sec >= 60) {
      min++;
      sec = 0;
    }
    if (min >= 60) {
      deg++;
      min = 0;
    }
  }
  sprintf(s, "%d-%02d\'%02d%s\"", (int)deg, (int)min, (int)sec, t+1); //-2011-06-17
  return s;
}

//================================================================= GKgetDegree
//
double GKgetDegree(     // get degree value from string representation
  char *s )             // string representation (-'")
{
  double d, deg=0, min=0, sec=0;
  char ss[80], *pc;
  strcpy(ss, s);
  for (pc=ss; (*pc); pc++)  if (*pc == ',')  *pc = '.';
  sscanf(ss, "%lf-%lf\'%lf", &deg, &min, &sec);                   //-2011-06-17
  d = deg + (min + sec/60)/60;
  return d;
}

#ifdef MAIN //#############################################################
int main( int argc, char *argv ) {
  char s[80], slon[80], slat[80];
  GEOpoint geo = { 0, 0 };
  GKpoint gk = { 0, 0 };
  double r, h, lon, lat;
  int zone, n;
  *s = 0;
  printf("Test Gauss-Krueger\n");
  while (1) {
    printf(">");  fflush(stdout);
    gets(s);
    if (!*s)  break;
    switch (*s) {
      case 'K': // read Gauss-Krueger point
                n = sscanf(s+1, "%lf %lf", &r, &h);
                if (n != 2)  continue;
                gk.rechts = r;
                gk.hoch = h;
                printf("GK:  %lf  %lf\n", gk.rechts, gk.hoch);
                break;
      case 'G': // read geographical point
                n = sscanf(s+1, "%s %s", slon, slat);
                if (n != 2)  continue;
                geo.lon = GKgetDegree(slon);
                geo.lat = GKgetDegree(slat);
                strcpy(slon, GKgetString(geo.lon));
                strcpy(slat, GKgetString(geo.lat));
                printf("GEO:  %s  %s\n", slon, slat);
                break;
      case 'k': // convert geographic to Gauss-Krueger
                gk = GKgetGKpoint(geo.lon, geo.lat);
                printf("GK:  %lf  %lf\n", gk.rechts, gk.hoch);
                break;
      case 'g': // convert Gauss-Krueger to geographic
                geo = GKgetGEOpoint(gk.rechts, gk.hoch);
                strcpy(slon, GKgetString(geo.lon));
                strcpy(slat, GKgetString(geo.lat));
                printf("GEO:  %s  %s\n", slon, slat);
                break;
      case 'x': // transform Gauss-Krueger to specified zone
                n = sscanf(s+1, "%d", &zone);
                if (n != 1)  continue;
                gk = GKxGK(zone, gk.rechts, gk.hoch);
                printf("GK:  %lf  %lf\n", gk.rechts, gk.hoch);
                break;
      case 't': // list of forward and backward transforms
                n = sscanf(s+1, "%lf", &lon);
                if (n != 1)  continue;
                for (lat=47; lat<=55.05; lat+=0.1) {
                  gk = GKgetGKpoint(lon, lat);
                  geo = GKgetGEOpoint(gk.rechts, gk.hoch);
                  strcpy(slon, GKgetString(geo.lon));
                  strcpy(slat, GKgetString(geo.lat));
                  printf("%5.1lf %5.1lf  %s  %s\n", lon, lat, slon, slat);
                }
                break;
      default:  printf("?\n");
    }
  }
  return 0;
}
#endif //#################################################################
