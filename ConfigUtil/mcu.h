#ifndef INC_MCU_H
#define INC_MCU_H

void initSpi();
void initMcu();
void resetMcu();
void initPtt();
float read_adc(unsigned char a, int scale);
void get_ptt_status(unsigned char p);

#endif