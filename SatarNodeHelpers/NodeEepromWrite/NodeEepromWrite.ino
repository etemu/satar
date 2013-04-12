/*
 * 
 * Stores the nodeID, ethernet MAC and IP to the EEPROM.
 * These values will stay in the EEPROM when the board is
 * turned off and may be retrieved later when booting the node.
 */

#include <EEPROM.h>

// Change these values according to your network setup:
static byte nodeID = 10; // range: 2-255
static byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, nodeID }; // choose a unique MAC
static byte ip[] = { 192, 168, 8, nodeID }; // your subnet IP scheme





byte val[] = {nodeID, mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],ip[0],ip[1],ip[2]};

int addr=0;
void setup()
{
  SaveData();
}

void loop()
{
  delay(1337);
  
}

void SaveData(){
  while (addr < 10){
  EEPROM.write(addr, val[addr]);
  // advance to the next address. 
  addr = addr + 1;
    }
}
