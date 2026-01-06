//=============================================================================#
/** 
 * @brief: v- Read Voltage of capacitor using:
 *     + Resolution: 14 bit"
 *     + Pin AIN1
 * @note: Vcap =! Vbatt in this case due to using the boost
*/                                                  #
//=============================================================================#


#ifndef APPICATION_READ_VCAP_H_
#define APPICATION_READ_VCAP_H_

int8_t Vcap_init(void);
uint16_t read_Vcap_mv(void);

#endif /*APPICATION_READ_VCAP_H_*/