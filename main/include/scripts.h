/*
 * scripts.h
 *
 *  Created on: Oct 27, 2023
 *      Author: dig
 */

#ifndef MAIN_INCLUDE_SCRIPTS_H_
#define MAIN_INCLUDE_SCRIPTS_H_

int getRTMeasValuesScript(char *pBuffer, int count) ;
int getNewMeasValuesScript(char *pBuffer, int count);
int getDayLogScript(char *pBuffer, int count);
int getHourLogScript(char *pBuffer, int count);
int getInfoValuesScript (char *pBuffer, int count);
int getCalValuesScript (char *pBuffer, int count);
int saveSettingsScript (char *pBuffer, int count);
int cancelSettingsScript (char *pBuffer, int count);
int calibrateRespScript(char *pBuffer, int count);
int getSensorNameScript (char *pBuffer, int count);
void parseCGIWriteData(char *buf, int received);

#endif /* MAIN_INCLUDE_SCRIPTS_H_ */
