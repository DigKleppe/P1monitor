/*
 * log.h
 *
 *  Created on: Nov 3, 2023
 *      Author: dig
 */

#ifndef MAIN_INCLUDE_LOG_H_
#define MAIN_INCLUDE_LOG_H_


#define MEASINTERVAL			 	1  //interval for sensor in seconds
#define LOGINTERVAL					(5*60)
#define MAXDAYLOGVALUES				((24*60*60)/LOGINTERVAL)  // for dayLog

#define MAXHOURLOGVALUES			3600

#include <stdint.h>
#include "main.h"

typedef struct {
	int32_t timeStamp;
	float power;
#ifdef SOLARPANELS
	float deliveredPower;
#endif
	float voltage;
} log_t;

extern int dayLogRxIdx;
extern int hourLogRxIdx;

extern log_t dayLog[ MAXDAYLOGVALUES];
extern log_t hourLog[ MAXHOURLOGVALUES];
void addToLog( log_t logValue);



#endif /* MAIN_INCLUDE_LOG_H_ */
