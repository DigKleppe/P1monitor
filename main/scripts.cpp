/*
 * dummyscripts.cpp
 *
 *  Created on: Oct 27, 2023
 *      Author: dig
 */


// called from CGI
#include <stdio.h>
#include <string.h>

#include "settings.h"
#include "scripts.h"
#include "p1parser.h"
#include "log.h"

extern int scriptState;

int getSensorNameScript(char *pBuffer, int count) {
	if ( pBuffer == NULL)
		return 0;

	int len = 0;
	switch (scriptState) {
	case 0:
		scriptState++;
		len += sprintf(pBuffer + len, "Actueel,Nieuw\n");
		len += sprintf(pBuffer + len, "%s\n", userSettings.moduleName);
		return len;
		break;
	default:
		break;
	}
	return 0;
}

int getInfoValuesScript(char *pBuffer, int count) {
	switch (scriptState) {
	case 0:
		scriptState++;
		strcpy(pBuffer,"Onderdeel,Waarde\r");
		strcat (pBuffer,p1OutBuffer);
		return strlen(pBuffer);
		break;
	default:
		break;
	}
	return 0;
}



int saveSettingsScript(char *pBuffer, int count) {
	saveSettings();
	return 0;
}

int cancelSettingsScript(char *pBuffer, int count) {
	loadSettings();
	return 0;
}


// @formatter:off
char tempName[MAX_STRLEN];

const CGIdesc_t writeVarDescriptors[] = {
//		{ "Temperatuur", &calValues.temperature, FLT, NR_NTCS },
//		{ "moduleName",tempName, STR, 1 }
};

// @formatter:on

// reads all avaiable data from log
// issued as first request.

int getDayLogScript(char *pBuffer, int count) {
	static int oldTimeStamp = 0;
	static int logsToSend = 0;
	int left, len = 0;
	int n;
	if (scriptState == 0) { // find oldest value in cyclic logbuffer
		dayLogRxIdx = 0;
		if( dayLog[dayLogRxIdx].timeStamp == 0)
			return 0; // empty

		oldTimeStamp = 0;
		for (n = 0; n < MAXDAYLOGVALUES; n++) {
			if (dayLog[dayLogRxIdx].timeStamp < oldTimeStamp)
				break;
			else {
				oldTimeStamp = dayLog[dayLogRxIdx++].timeStamp;
			}
		}
		if (dayLog[dayLogRxIdx].timeStamp == 0) { // then log not full
			// not written yet?
			dayLogRxIdx = 0;
			logsToSend = n;
		} else
			logsToSend = MAXDAYLOGVALUES;
		scriptState++;
	}
	if (scriptState == 1) { // send complete buffer
		if (logsToSend) {
			do {
#ifdef SOLARPANELS
				len += sprintf(pBuffer + len, "%1.0f,%1.1f;", dayLog[dayLogRxIdx].power,dayLog[dayLogRxIdx].deliveredPower);
#else
				len += sprintf(pBuffer + len, "%1.0f,%1.1f;", dayLog[dayLogRxIdx].power,dayLog[dayLogRxIdx].voltage);
#endif
				dayLogRxIdx++;
				if (dayLogRxIdx >= MAXDAYLOGVALUES)
					dayLogRxIdx = 0;
				left = count - len;
				logsToSend--;

			} while (logsToSend && (left > 40));
		}
	}
	return len;
}

int getHourLogScript(char *pBuffer, int count) {
	static int oldTimeStamp = 0;
	static int logsToSend = 0;
	int left, len = 0;
	int n;
	if (scriptState == 0) { // find oldest value in cyclic logbuffer
		hourLogRxIdx = 0;
		if( hourLog[dayLogRxIdx].timeStamp == 0)
			return 0; // empty
		oldTimeStamp = 0;
		for (n = 0; n < MAXHOURLOGVALUES; n++) {
			if (hourLog[hourLogRxIdx].timeStamp < oldTimeStamp)
				break;
			else {
				oldTimeStamp = hourLog[hourLogRxIdx++].timeStamp;
			}
		}
		if (hourLog[hourLogRxIdx].timeStamp == 0) { // then log not full
			// not written yet?
			hourLogRxIdx = 0;
			logsToSend = n;
		} else
			logsToSend = MAXHOURLOGVALUES;
		scriptState++;
	}
	if (scriptState == 1) { // send complete buffer
		if (logsToSend) {
			do {
#ifdef SOLARPANELS
				len += sprintf(pBuffer + len, "%1.0f,%1.1f;", hourLog[hourLogRxIdx].power,hourLog[hourLogRxIdx].deliveredPower);
#else
				len += sprintf(pBuffer + len, "%1.0f,%1.1f;", hourLog[hourLogRxIdx].power,hourLog[hourLogRxIdx].voltage);
#endif
				hourLogRxIdx++;
				if (hourLogRxIdx >= MAXHOURLOGVALUES)
					hourLogRxIdx = 0;
				left = count - len;
				logsToSend--;

			} while (logsToSend && (left > 40));
		}
	}
	return len;
}


// values of setcal not used, calibrate ( offset only against reference TMP117
void parseCGIWriteData(char *buf, int received) {

}

