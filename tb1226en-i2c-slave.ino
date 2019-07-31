/* TB1226EN I2C Bridge
 * (2019) Martin Hejnfelt (martin@hejnfelt.com)
 *
 * For use with monitors that use the TB1226EN jungle IC.
 * This changes the I2C communication from monitor MCUs
 * that doesn't treat RGB like it should, to something
 * that does. The Arduino acts as the jungle, and thus
 * intercepts and changes the I2C communication from the
 * MCU. Basically it ensures that RGB Contrast is 
 * in sync with the "other" uni-color contrast value, 
 * plus ensures RGB is not muted.
 *
 * Connect the main I2C line of the monitors PCB to 
 * the hardware I2C (TWI) pins which is usually on 
 * pin A4 (SDA) and A5 (SCL) and then specify the 
 * desired secondary I2C bus pins for the communication 
 * to the jungle. These are by default D4 (SDA) and 
 * D5 (SDA) which works for ATmega328P based boards.
 * On the TB1226EN pin 9 is SCL and pin 10 is SDA.
 * The jungle will need to be isolated, so whatever
 * resistors are on the I2C line should be removed,
 * and the freed holes/pads can then be used to inject 
 * and intercept at.
 * 
 * A speedy Arduino is recommended, for now tested on:
 * - 20MHz atmega2560 clone
 * - 16MHz Nano v3.0 clone
 * - 16MHz UNO
 *
 * Requires the SoftI2CMaster library
 * https://github.com/felias-fogg/SoftI2CMaster
 *
 * Tested on:
 * JVC TM-A140PN (R291 for SCL, R292 for SDA)
 * JVC TM-A101G (R291 for SCL, R292 for SDA)
 */

#define I2C_TIMEOUT 10000
#define I2C_PULLUP 0
#define I2C_FASTMODE 1

// These work for a Mega2560
/*
#define SDA_PORT PORTA
#define SDA_PIN 0 // = 22
#define SCL_PORT PORTA
#define SCL_PIN 2 // = 24
*/

// These work for Nano 3.0
#define SDA_PORT PORTD
#define SDA_PIN 4 // = PD4
#define SCL_PORT PORTD
#define SCL_PIN 5 // = PD5

#include <SoftI2CMaster.h>
#include <Wire.h>
// Address in the datasheet is said to be 0x88 for write
// and 0x89 for read. That is somewhat of a "mistake" as
// i2c uses 7 bit addressing and the least significant bit
// is read (1) or write (0). Thus the address is shifted once
// to the right to get the "real" address which is then 44h/68
#define TB1226EN_ADDR (68)

#define REG_UNICOLOR (0x00)
#define REG_RGB_CONTRAST (0x06)
#define REG_MUTE_WIDE_VBLK (0x1B)

void setup() {
  Serial.begin(115200);
  Serial.print("TB1226EN I2C Bridge\n");
  while(!i2c_init()) {
    delay(100);
  }
  read();
  Wire.begin(TB1226EN_ADDR);
  Wire.onReceive(writeRequest);
  Wire.onRequest(readRequest);
}

void writeRegister(const uint8_t reg, const uint8_t val) {
  i2c_start((TB1226EN_ADDR<<1)|I2C_WRITE);
  bool b1 = i2c_write(reg);
  bool b2 = i2c_write(val);
  i2c_stop();
  if(!b1 || !b2) {
    Serial.print("Failed writing register ");
    Serial.print(reg,HEX);
    Serial.print("\n");
  }
}

void writeRequest(int byteCount) {
  uint8_t reg = Wire.read();
  uint8_t val = Wire.read();
  switch(reg) {
      case REG_UNICOLOR:
        writeRegister(reg,val);
        writeRegister(REG_RGB_CONTRAST,val);
      break;
      case REG_RGB_CONTRAST:
        // Dont kill RGB contrast
      break;
      case REG_MUTE_WIDE_VBLK:
        // Never mute RGB
        writeRegister(reg,val&0xBF);
      break;
      default:
        writeRegister(reg,val);
      break;
  }
}

uint8_t r[2] = {0, 0};

void readRequest() {
  Wire.write(r,2);
}

void read() {
  i2c_start((TB1226EN_ADDR<<1)|I2C_READ);
  r[0] = i2c_read(false); // read one byte
  r[1] = i2c_read(true); // read one byte and send NAK to terminate
  i2c_stop(); // send stop condition 
}

void loop() {
  noInterrupts();
  read();
  interrupts();
  delay(1000);
}
