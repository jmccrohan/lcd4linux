#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#include "lcd4linux.h"
#include "filter.h"

#define SLOTS 64

typedef struct {
  char *name;
  int slots;
  struct timeval *time;
  double *value;
} FILTER;

#define SECONDS(x) (x.tv_sec+x.tv_usec/1000000.0)

double smooth(char *name, int period, double value)
{
  static FILTER *Filter=NULL;
  static int nFilter=0;
  struct timeval now;
  double t, v;
  int i, j;
  
  gettimeofday (&now, NULL);
  
  for (i=0; i<nFilter; i++) {
    if (strcmp(name, Filter[i].name)==0) 
      break;
  }
  
  if (i==nFilter) {
    int slots=(period+tick-1)/tick;
    if (slots<2) 
      slots=2;
    else if (slots>SLOTS)
      slots=SLOTS;

    nFilter++;
    Filter=realloc(Filter, nFilter*sizeof(FILTER));
    Filter[i].name=strdup(name);
    Filter[i].slots=slots;
    Filter[i].time=malloc(slots*sizeof(Filter[i].time[0]));
    Filter[i].value=malloc(slots*sizeof(Filter[i].value[0]));
    for (j=0; j<slots; j++) {
      Filter[i].time[j]=now;
      Filter[i].value[j]=value;
    }
  }
  
  for (j=Filter[i].slots-1; j>0; j--) {
    Filter[i].time[j]=Filter[i].time[j-1];
    Filter[i].value[j]=Filter[i].value[j-1];
  }
  Filter[i].time[0]=now;
  Filter[i].value[0]=value;
  
  t = SECONDS(Filter[i].time[0]) - SECONDS(Filter[i].time[Filter[i].slots-1]);
  v = Filter[i].value[0]-Filter[i].value[Filter[i].slots-1];

  if (t==0.0 || v<0.0)
    return 0;
  else
    return v/t;
}

double damp(char *name, double value)
{
  static FILTER *Filter=NULL;
  static int nFilter=0;
  struct timeval now;
  double max;
  int i, j;
  
  gettimeofday (&now, NULL);
  
  for (i=0; i<nFilter; i++) {
    if (strcmp(name, Filter[i].name)==0) 
      break;
  }
  
  if (i==nFilter) {
    int slots=log(100)*tau/tick;
    if (slots<1) 
      slots=1;
    else if (slots>SLOTS)
      slots=SLOTS;

    nFilter++;
    Filter=realloc(Filter, nFilter*sizeof(FILTER));
    Filter[i].name=strdup(name);
    Filter[i].slots=slots;
    Filter[i].time=malloc(slots*sizeof(Filter[i].time));
    Filter[i].value=malloc(slots*sizeof(Filter[i].value));
    for (j=0; j<slots; j++) {
      Filter[i].time[j]=now;
      Filter[i].value[j]=0;
    }
  }
  
  max=value;
  for (j=Filter[i].slots-1; j>0; j--) {
    double t = SECONDS(Filter[i].time[j]) - SECONDS(Filter[i].time[j-1]);
    Filter[i].time[j]=Filter[i].time[j-1];
    Filter[i].value[j]=Filter[i].value[j-1]*exp(-t/tau);
    if (Filter[i].value[j]>max) max=Filter[i].value[j];
  }

  Filter[i].time[0]=now;
  Filter[i].value[0]=value;
  
  return max;
}
