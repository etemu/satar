/*
 * Read out the EEPROM config.
 *
 */

#include <EEPROM.h>
#include <avr/eeprom.h>

uint16_t value;

void setup()
{
  // initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  delay(64);
  read_config(); // read out the EEPROM and print it to serial
  read_wilssen_config();
}

void read_config()
{
  // read a byte from the current address of the EEPROM
  for (int i=0; i<=15;i++){ // start reading from the first byte (address 0) of the EEPROM
  value = EEPROM.read(i);
  Serial.print(i);
  Serial.print("\t");
  Serial.print(value, DEC);
  Serial.println();
  // advance to the next address of the EEPROM...
   } 
}

void read_wilssen_config()
{
  Serial.print("WiLSSEN configuration:");
  Serial.println();
  // read a byte from the current address of the EEPROM
  for (int i=42; i<=42;i++){ // start reading from the first byte (address 0) of the EEPROM
  value = eeprom_read_word((uint16_t*) i);
  Serial.print(i);
  Serial.print("\t");
  Serial.print(value, OCT);
  Serial.println();
  // advance to the next address of the EEPROM...
   } 
}

void loop(){
delay(420000);
}
