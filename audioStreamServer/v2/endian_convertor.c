#include "endian_convertor.h"

void inttolittle(unsigned int x, uint8_t* y){
  y[0] = (uint8_t)(x>>0);
  y[1] = (uint8_t)(x>>8);
  y[2] = (uint8_t)(x>>16);
  y[3] = (uint8_t)(x>>24);
}

unsigned int littletoint(uint8_t* y) {
  return (y[0]<<0) | (y[1]<<8) | (y[2]<<16) | (y[3]<<24);
}

void shorttolittle(unsigned short x, uint8_t* y){
  y[0] = (uint8_t)(x>>0);
  y[1] = (uint8_t)(x>>8);
}

unsigned short littletoshort(uint8_t* y) {
  return (y[0]<<0) | (y[1]<<8);
}

void uinttobyte(int x, char* y){
  y[0] = (x & 0xFF);
}

unsigned int bytetouint(char* y) {
  return (y[0]<<0);
}