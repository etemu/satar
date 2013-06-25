/*
 * Stores the nodeID, ethernet MAC and IP to the EEPROM.
 * These values will stay in the EEPROM when the board is
 * turned off and may be retrieved later when booting the node.
 */

#include <EEPROM.h>
#include <avr/eeprom.h>
// Change these values according to your network setup:
static byte nodeID = 7; // range: 2-254 // addr0
static byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, nodeID }; // choose a unique MAC // addr1-6
static byte ip[] = { 192, 168, 8, nodeID }; // your subnet IP scheme // addr7-10
static byte gw[] = {192, 168, 8, 1}; // your gateway // addr11-14

// The leading 0 (zero) is a descriptor for the complier to interpret the following digits as an octal number. Do not leave out the leading zeros.
static uint16_t wilssen_node = 00012; // 00000=master, 05555=highest value.

byte val[] = {nodeID, mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],ip[0],ip[1],ip[2],ip[3],gw[0],gw[1],gw[2],gw[3]};
uint16_t value;
int addr=0;
int led = 13;
void setup()
{
    // initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
 
  SaveData();
  WilConf();
  Serial.println("EEPROM config saved.");
  delay(128);
  read_config(); // read out the EEPROM and print it to serial
  delay(128);
  read_wilssen_config();
  pinMode(led, OUTPUT); 
}

void loop()
{
for (int _i=0; _i<=3; _i++){
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(66);               
  digitalWrite(led, LOW);
  delay(200);
}  
  delay(2000);              
  }

void SaveData(){
  while (addr < 15){
  EEPROM.write(addr, val[addr]);
  // advance to the next address. 
  addr++;
    }
}

void WilConf(){
eeprom_write_word ((uint16_t*) 42, wilssen_node) ;
// eeprom_write_word(wilssen_addr, wilssen_node);
//eeprom_write_block(static_cast<void *>(&wilssen_node),WilConfPointer,sizeof(wilssen_node));
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
  Serial.print("WiLSSEN config:");
  Serial.println();
  // read a byte from the current address of the EEPROM
  for (int i=42; i<=42;i++){ // start reading from the first byte (address 0) of the EEPROM
  value = eeprom_read_word((uint16_t*) i);
  Serial.print(i);
  Serial.print("\t");
  Serial.print(value, DEC);
  Serial.println();
  Serial.print("Wnode");
  Serial.print("\t");
  Serial.print(value, OCT);
  Serial.println();
  // advance to the next address of the EEPROM...
   } 
}
