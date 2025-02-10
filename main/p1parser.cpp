/*
 * p1parser.cpp
 *
 *  Created on: Oct 21, 2023
 *      Author: dig
 *
 *      1-3:0.2.8(50) // version

 0-0:1.0.0(231021110048S) // timestamp

 0-0:96.1.1(4530303533303038333837313934353231) // equipment ident

 1-0:1.8.1(000525.570*kWh) // deliverd energy  to client Tariff 1

 1-0:1.8.2(000530.217*kWh) // deliverd energy to client Tariff 2

 1-0:2.8.1(000001.552*kWh) // deliverd energy by client Tariff 2

 1-0:2.8.2(000000.000*kWh) // deliverd energy by client Tariff 2

 0-0:96.14.0(0001)	  // tariff indicator

 1-0:1.7.0(00.079*kW)      // actual power deleverd

 1-0:2.7.0(00.000*kW)	// actual received

 0-0:96.7.21(00012)	// nr power failures

 0-0:96.7.9(00004)	// nr long power failures

 1-0:99.97.0(2)(0-0:96.7.19)(210827121443S)(0000010761*s)(210827131420S)(0000000326*s) // Power Failure Event Log (long power failures)

 1-0:32.32.0(00008)	// Number of voltage sags in phase L1

 1-0:32.36.0(00001)	// Number of voltage swells in phase L1

 0-0:96.13.0()		// ext message max 1024 characters.

 1-0:32.7.0(227.5*V)	// Instantaneous voltage L1 in V resolution

 1-0:31.7.0(000*A)	// Instantaneous current L1 in A resolution.

 1-0:21.7.0(00.078*kW)	// instantaneous active power L1 (+P) in W resolution

 1-0:22.7.0(00.000*kW)	// Instantaneous active power L1 (-P) in W resolution

 *
 */

#include "p1parser.h"
#include <string.h>
#include <stdio.h>

#include "scripts.h"
#include "log.h"
#include "main.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

char p1OutData[P1OUTDATASIZE];
char p1OutBuffer[P1OUTDATASIZE];

log_t logValue;

volatile bool newP1Data;



#ifdef SIMULATE
	int simValue = 100;
	int m;
#endif

// @formatter:off
p1Var_t p1VarTable[] = {

#ifdef SOLARPANELS
#ifdef _3PHASE
	{ "1-0:21.7.0", "Actueel opgenomen vermogen L1",1},
	{ "1-0:41.7.0", "Actueel opgenomen vermogen L2",1},
	{ "1-0:61.7.0", "Actueel opgenomen vermogen L3",1},
	{ "1-0:22.7.0", "Actueel geleverd vermogen L1",2},
	{ "1-0:42.7.0", "Actueel geleverd vermogen L2",2},
	{ "1-0:62.7.0", "Actueel geleverd vermogen L3",2},
	{ "1-0:32.7.0", "Spanning L1",3},
	{ "1-0:52.7.0", "Spanning L2",3},
	{ "1-0:72.7.0", "Spanning L3",3},
	{ "1-0:1.8.1", 	"Opgenomen energie tarief 1",0 },
	{ "1-0:1.8.2", 	"Opgenomen energie tarief 2",0 },
	{ "1-0:2.8.1", 	"Geleverde energie tarief 1",0 },
	{ "1-0:2.8.2", 	"Geleverde energie tarief 2",0 },
	{ "0-0:96.7.21","Korte onderbrekingen",0},
	{ "0-0:96.7.9", "Lange onderbrekingen",0 },
//	{ "1-0:99.97.0","Onderbrekingslog",4 },
	{ "", "",0 },
#endif

#else  // no panels 1 phase
	{ "1-0:21.7.0", "Actueel opgenomen vermogen",1},
	{ "1-0:32.7.0", "Voltage",3},
	{ "1-0:1.8.1", 	"Geleverd tarief 1",0 },
	{ "1-0:1.8.2", 	"Geleverd tarief 2",0 },
	{ "0-0:96.7.21","Korte onderbrekingen",0},
	{ "0-0:96.7.9", "Lange onderbrekingen",0 },
	{ "1-0:99.97.0", "Onderbrekingslog",4 },
	{ "1-0:32.32.0","Spanningsdalen",0},
	{ "1-0:32.36.0","Spanningspieken" ,0},
	{ "", "",0 }
};

#endif
};
// @formatter:on


// p1Buffer points to '(' of value
int copyValue(char *src, char *dest) {
	int len = 0;
//	char *e;
	do {
		src++;
	} while ((*src == '0') && (*(src + 1) != '.') && (*(src + 1) != '*')); // skip leading zeros values


	do {
		len++;
		if( *src != '*') // do not show '*'
			*dest = *src;
		else
			*dest = ' ';
		dest++;
		src++;
	} while ( (*src != ')'));

//	e = strchr(src, ')');  // search ')'
//	strncpy(dest, src, e - src);  // add value

	return len ;//  e - src;  // return nr chars written in dest
}

int copyName(char *src, char *dest ) {
	return sprintf (dest, "%s=", src);
}
// reads p1Buffer, searches for IDs , if found add corresponding name and value to p1OutData;

bool parseP1data(char *p1Buffer, int nrCharsInBuffer) {
	char *p = p1OutData;
	char *b;  // begin value
	char *e;  // end value
	char *l;
	int n = 0;
	bool found;
	long var;
	int len;
	float f;
	logValue.power =0;
#ifdef SOLARPANELS
	logValue.deliveredPower =0;
#endif

	do {
		b = strstr(p1Buffer, p1VarTable[n].p1ID);  // find ID
		if (b != NULL) {
			b += strlen(p1VarTable[n].p1ID); // set pointer to first character of value
			e = strchr(b, ')');  // search ')'
			if (e != NULL) {
				p += copyName( p1VarTable[n].name, p);

				switch (p1VarTable[n].function) {
				case 0:
					p += copyValue(b, p);
					break;

				case 1: // Power used, set from kW to W
#ifdef SIMULATE
					simValue++;
					p += sprintf(p, "%d W", simValue);
					if ( simValue > 2000)
						simValue = 50;
					logValue.power += simValue; // add power of 3 phases ( if present)

#else
					len = sscanf(b + 1, "%f", &f);  // read kW
					if (len) {
						f *= 1000.0;
						logValue.power += f; // add power of 3 phases ( if present)
						p += sprintf(p, "%d W", (int) f);
					}
#endif

					break;

				case 2: // Power delivered, set from kW to W
					len = sscanf(b + 1, "%f", &f);  // read kW

#ifdef SIMULATE
					simValue++;
					p += sprintf(p, "%d W", simValue);
					logValue.deliveredPower += simValue; // add power of 3 phases ( if present)
#else
					if (len) {
						f *= 1000.0;
						logValue.deliveredPower += f; // add power of 3 phases ( if present)
						p += sprintf(p, "%d W", (int) f);
					}
#endif
					break;

				case 3:  // voltage
					len = sscanf(b + 1, "%f", &f);  // read Voltage
					if (len) {
						p += sprintf(p, "%1.1f V",  f);
						logValue.voltage = f;
					}
					break;


				case 4:   // power failures log eg 1-0:99.97.0(2)(0-0:96.7.19)(210827121443S)(0000010761*s)(210827131420S)(0000000326*s)
					p += copyValue(b, p);  //
					*p++ = ' ';

					b = strchr(e + 1, ')');  // search ')' after "(0-0:96.7.19)"
					l = strchr(e + 1, '\r'); // last character in log

					do {
						found = false;
						if ((b) && (l))
							if (b < l) {
								b = strchr(b + 1, '(');  // search '('
								if ((b) && (b < l)) {
									e = strchr(b + 1, ')'); // search ')'
									if (e) {
										strncpy(p, b + 1, e - b - 2); // add value without () and 'S'
										p += e - b - 2;
										*p++ = ' ';
										b = strchr(b + 1, '(');  // search  next '('
										sscanf(b + 1, "%ld", &var);  // read seconds without leading zeros
										len = sprintf(p, "%ld ", var);
										p += len;
										found = true;
										b++;
									}
								}
							}
					} while (found);
					break;
				default:
					break;
				}
				*p++ = '\r';
				*p = 0;
			}
		}
		n++;

	} while (strlen(p1VarTable[n].p1ID) != 0);

	memcpy (p1OutBuffer, p1OutData, sizeof(p1OutBuffer));
	newP1Data = true;
	addToLog(logValue);
	gpio_set_level(LED_PIN, true);
	gpio_set_level(LED_INV_PIN, false);
	vTaskDelay(20 / portTICK_PERIOD_MS);
	gpio_set_level(LED_PIN, false);
	gpio_set_level(LED_INV_PIN, true);
	return false;
}

