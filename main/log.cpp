/*
 * log.cpp
 *
 *  Created on: Nov 3, 2023
 *      Author: dig
 */
#include "log.h"
#include <stdint.h>

uint32_t  timeStamp;
int dayLogRxIdx;
int dayLogTxIdx;
int hourLogRxIdx;
int hourLogTxIdx;

int logPrescaler = LOGINTERVAL;

log_t accumulator;
log_t dayLog[ MAXDAYLOGVALUES];
log_t hourLog[ MAXHOURLOGVALUES];


void addToLog( log_t logValue)
{
	logValue.timeStamp = ++timeStamp;
	accumulator.power += logValue.power;
#ifdef SOLARPANELS
	accumulator.deliveredPower += logValue.deliveredPower;
#endif
	accumulator.voltage += logValue.voltage;

	if (--logPrescaler == 0) {
		logPrescaler = LOGINTERVAL;
		dayLog[dayLogTxIdx].power = accumulator.power / LOGINTERVAL; // average
		accumulator.power = 0;
#ifdef SOLARPANELS
		dayLog[dayLogTxIdx].deliveredPower = accumulator.deliveredPower / LOGINTERVAL; // average
		accumulator.deliveredPower = 0;
#endif
		dayLog[dayLogTxIdx].voltage = accumulator.voltage / LOGINTERVAL;
		dayLog[dayLogTxIdx].timeStamp = timeStamp;

		accumulator.voltage = 0;
		dayLogTxIdx++;
		if (dayLogTxIdx >= MAXDAYLOGVALUES)
			dayLogTxIdx = 0;
	}
	hourLog[hourLogTxIdx] = logValue;
	hourLogTxIdx++;
	if (hourLogTxIdx >= MAXHOURLOGVALUES)
		hourLogTxIdx = 0;
}
