/*
 * Copyright (c) 2009 Artur Bergman <sky@crucially.net>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "TSDB.h"
#include <sys/time.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <string.h>

#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

extern char *optarg;

int agg_usage () {
  WARN("Usage: tsdb agg [options]\n");
  WARN(" -f filename  db to get info about\n");
  WARN(" -s start aggregating from this timestamp (unix timestamp)\n");
  WARN(" -e aggregate until this timestamp (unix timestamp)\n");
  WARN(" -i interval to aggregate down to (seconds)\n");
  WARN(" -h this help\n\n");
  WARN("Aggregate a 24 hour period into 12 buckets each with the aggregate of an hour\n");
  WARN("\ttsdb agg -f data.tsdb -s 1258096151 -e 1258182558 -i 3600\n");
  return 1;
}

int agg_main(int argc, char * const *argv) {
  int o, start, end, interval, column, difference, unit, unit_set;
  char *filename = 0;
  char *unit_name = 0;

  difference = column = start = end = interval = unit = unit_set = 0;

  while ((o = getopt(argc, argv,
		     "f:s:e:i:h:c:du:")) != -1) {

    switch(o) {
    case 'd':
      difference = 1;
      break;
    case 'c':
      column = atoi(optarg);
      break;
    case 'f':
      filename = malloc(strlen(optarg)+1);
      memcpy(filename, optarg, strlen(optarg)+1);
      break;
    case 'u':
      unit_name = malloc(strlen(optarg)+1);
      memcpy(unit_name, optarg, strlen(optarg)+1);
      unit_set++;
      break;
    case 's':
      start = atoi(optarg);
      break;      
    case 'e':
      end = atoi(optarg);
      break;      
    case 'i':
      interval = atoi(optarg);
      break;      
    }
  }

  if(!filename || !interval) {
    return agg_usage();
  }

  if(unit_set) {
    if(!memcmp(unit_name, "Mibit", 4)) {
      unit = 1024*1024/8;
    } else if (!memcmp(unit_name, "Kibit", 4)) {
      unit = 1024/8;
    } else {
      printf("Unknown unit name\n");
    }
  } else {
    unit = 1;
  }
  

  int flags;
  if (difference)
    flags = TSDB_COUNTER;
  else
    flags = TSDB_GAUGE;

  struct TSDB_metric_t *metric    = TSDB_open(filename);

  if(!start)
    start = metric->created;

  if(!end) 
    end = time(NULL);

  struct TSDB_range_t  *range     = TSDB_timespan(metric, start, end);
  struct TSDB_range_t  *aggregate = TSDB_aggregate(range, interval, flags, unit);

  int rows = aggregate->rows;
  int i = 0;
  while(rows--) {
    int offset = ((i * aggregate->columns) + column);
    printf("AVG: %.4f      MAX: %llu      MIN: %llu      STDDEV: %f\n", aggregate->agg[offset].avg, aggregate->agg[offset].max, aggregate->agg[offset].min, aggregate->agg[offset].stddev);
    i++;
  }
  return 0;
}
