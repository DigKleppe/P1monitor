/*
 * p1parser.h
 *
 *  Created on: Oct 21, 2023
 *      Author: dig
 */

#ifndef MAIN_INCLUDE_P1PARSER_H_
#define MAIN_INCLUDE_P1PARSER_H_

#define P1OUTDATASIZE 	1024


typedef struct {
	char p1ID[16]; // id from P1 specification
	char name[32]; // name
	int function; // special function
}p1Var_t;

//#define SIMULATE
extern char p1OutBuffer[P1OUTDATASIZE];

bool parseP1data(char * p1Buffer,int nrCharsInBuffer);

#endif /* MAIN_INCLUDE_P1PARSER_H_ */
