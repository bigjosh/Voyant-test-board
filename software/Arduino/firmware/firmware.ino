
#include <SPI.h>  // include the SPI library:
#include "SoftSPIB.h"

struct spi_port : SoftSPIB {

  private:
        uint8_t _rst;
        uint8_t _cs;

  public:
        spi_port( uint8_t mosi, uint8_t miso, uint8_t sck , uint8_t cs , uint8_t rst ) 
          : SoftSPIB( mosi , miso , sck ) 
          {
            _cs    = cs;
            _rst = rst;
          
          }
          
        void begin() {
          SoftSPIB::begin();
          pinMode( _cs  , OUTPUT );
          digitalWrite( _cs , HIGH );
          
          pinMode( _rst , OUTPUT );
        }
    
        void set_reset( boolean b ) {
          digitalWrite( _rst , b );
        }
                
};


spi_port spi_ports[] = {
  spi_port( 1 , 2 , 3 , 4 , 5 )
  
};


SoftSPIB SPIS2(2, 3, 4);

const int slaveSelectPin = 10;
const int slaveSelectPin1 = 11;
const int slaveSelectPinS2 = 5;



void setup()
{

 Serial2.begin(2000000);           // Don't use Serial1 becuase pin conflicts with SPI1
 Serial2.println("Hello");
 pinMode(slaveSelectPin, OUTPUT);
 pinMode(slaveSelectPin1, OUTPUT);
 pinMode(slaveSelectPinS2, OUTPUT);

 // MsTimer2::set(20, flash); // 500ms period
 // MsTimer2::start();

  // initialize SPI:
  SPI.begin(); 
  SPI1.begin();   
  SPIS2.begin();
}



void loop() {
 Serial2.println("Loop");
 delay(100);

  digitalPotWrite( 0x01,  0b10101010 );
  digitalPotWrite1( 0x01, 0b10100000 );
  digitalPotWriteS2( 0x01, 0b10100000 );
  
  return;

  
  // go through the six channels of the digital pot:
  for (int channel = 0; channel < 6; channel++) { 
    
    // change the resistance on this channel from min to max:
    for (int level = 0; level < 255; level++) {
      digitalPotWrite(channel, level);
      delay(10);
    }
    // wait a second at the top:
    delay(100);
    // change the resistance on this channel from max to min:
    for (int level = 0; level < 255; level++) {
      digitalPotWrite(channel, 255 - level);
      delay(10);
    }
  }

}

void digitalPotWrite(int address, int value) {
  // take the SS pin low to select the chip:
  digitalWrite(slaveSelectPin,LOW);
  //  send in the address and value via SPI:
  SPI.transfer(address);
  SPI.transfer(value);
  // take the SS pin high to de-select the chip:
  digitalWrite(slaveSelectPin,HIGH); 
}

void digitalPotWrite1(int address, int value) {
  // take the SS pin low to select the chip:
  digitalWrite(slaveSelectPin1,LOW);
  //  send in the address and value via SPI:
  SPI1.transfer(address);
  SPI1.transfer(value);
  // take the SS pin high to de-select the chip:
  digitalWrite(slaveSelectPin1,HIGH); 
}


void digitalPotWriteS2(int address, int value) {
  // take the SS pin low to select the chip:
  digitalWrite(slaveSelectPinS2,LOW);
  //  send in the address and value via SPI:
  SPIS2.transfer(address);
  SPIS2.transfer(value);
  // take the SS pin high to de-select the chip:
  digitalWrite(slaveSelectPinS2,HIGH); 
}
