
/*
 * 
 * **** Firmware for Voyant Test Board
 * **** https://github.com/bigjosh/Voyant-test-board
 * 
 * To use this skecth, you must... 
 * 
 * 1. Install Arduino IDE https://www.arduino.cc/en/software
 * 2. Install Teensiduino from https://www.pjrc.com/teensy/td_download.html 
 * 3. Set the following in the Arduino Tools menu...
 * 
 *    Board     = Teensy->Teensyduino->Teensy 4.1
 *    USB Type  = Tripple Serial
 *    CPU Speed = 600Mhz (default)
 *    Port      = (find the Teensy 4.1 board on this list)
 */


// For noww we will use a softSPI implementation for flexibility and simplicity. We can always switch to a faster hardware implementation
// on a per-port basisi if needed.
#include "SoftSPIB.h"

#include <limits.h>   // UCHAR_MAX

#include <map>
using namespace std;

std::map<int, char> m = {{1, 'a'}, {3, 'b'}, {5, 'c'}, {7, 'd'}};

// This structure defines an SPI port target. 

struct spi_target { 


  private:

      SoftSPIB soft_spi;         // Favor composition over inheritance. Makes it easier to have hertogenious port types in the future. 
          
      uint8_t _rst;             // Reset pin or NO_RESET_PIN if none supported
      uint8_t _cs;              // Chip select pin

  public:

      static const uint8_t NO_RESET_PIN= UCHAR_MAX;    // Special value indicates this port does not have a reset pin. 

      spi_port( uint8_t mosi, uint8_t miso, uint8_t sck , uint8_t cs , uint8_t rst ) {
              
        soft_spi =  SoftSPIB( mosi , miso , sck );
        _cs    = cs;
        _rst = rst;          
        
      }

      // No reset pin version
      spi_port( uint8_t mosi, uint8_t miso, uint8_t sck , uint8_t cs ) {

        spi_port(  mosi,  miso,  sck , cs , NO_RESET_PIN ); 
        
      }

      // Sorry for this value overload, but C++ doesn't have any elegant way to do a discriminated union. :/
      boolean reset_supported() {
        return _rst != NO_RESET_PIN; 
      }

        
      void begin() {
        soft_spi.begin();
        pinMode( _cs  , OUTPUT );
        digitalWrite( _cs , HIGH );

        if (reset_supported()) {
          pinMode( _rst , OUTPUT );
        }
      }


      void start() {
        digitalWrite( _cs , LOW);
      }
     
      void end() {
        digitalWrite( _cs , HIGH);
      }

       
      // Set the value output on the reset pin. 
      void set_reset(boolean b) {
        digitalWrite( _rst , b );
      }
      

                
};

// A UART bridge connects a virtual USB port to physical UART pins
// It automatically makes the baud rate on the pins follow the baud rate on the virtual port
// and bridges data back and forth between the two as long as `service()` is called frequently.

struct uart_bridge { 

  private:


      HardwareSerial  *_real_port;
     
      usb_serial2_class *_virtual_port;

      boolean virtual_port_active_state = false;   // Is this port currently "active"
         

  public:
  
      uart_bridge( const HardwareSerial  *real_port, const usb_serial2_class *virtual_port  ) {

        _real_port = real_port;
        _virtual_port = virtual_port;                 
        
      }  

      void service() {

        if ( !virtual_port_active_state )  {  // The usb_serial_class on Teensy returns a boolean indicating if there is a connection to the virtual serial port

          // There is a USB connection

          if (!virtual_port_active_state) {   // Just connected
            
            _real_port->begin( _virtual_port->baud() );    // Set the real port to match the baudrate of the new USB connection

            // We could also set the other protocol params here if needed

            virtual_port_active_state = true;
             
          }

          // We are connected, move any pending data in both directions
          // For now we take the simple byte-by-byte aproach, we can buffer if it becomes nessisary

          while ( _real_port->available() ) {
            _virtual_port->write( _real_port->read() );
          }

          while ( _virtual_port->available() ) {
            _real_port->write( _virtual_port->read() );
          }
          
        
        } else {

            virtual_port_active_state = false;
          
        }
        
      }

};

// Define any desired UART bridges here

uart_bridge uart_bridges[] = {

  uart_bridge( &Serial2 , &SerialUSB1 ),      // TEC_UART connection
  
};

// Call this periodically to keep the uart bridges fresh

void service_uart_bridges() {

  for( auto &ub : uart_bridges ) {
    ub.service(); 
  }
  
}




/*

spi_target spi_targets[] = {
  spi_port( 1 , 2 , 3 , 4 , 5 )
  
};


SoftSPIB SPIS2(2, 3, 4);

const int slaveSelectPin = 10;
const int slaveSelectPin1 = 11;
const int slaveSelectPinS2 = 5;
*/


void setup()
{

  // Note that Teesy best practice is to not do Serial.begin()
  
  delay(100);    // Give host a moment to warm up USB 
  Serial.println(";Testboard command port. See https://github.com/bigjosh/Voyant-test-board");
  
  // Intialize outbound UART (`TEC_UART` on board). Does not matter what baud we pick, we will adjust the output baud to match whatever
  // the USB virtual port baud is before each send. 
  Serial2.begin(9600);
 
}




void loop() {

  // Maintain the uart bridges 
  service_uart_bridges();
  
}

  /*
}
//  digitalPotWrite( 0x01,  0b10101010 );
//  digitalPotWrite1( 0x01, 0b10100000 );
//  digitalPotWriteS2( 0x01, 0b10100000 );
  
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

  */
