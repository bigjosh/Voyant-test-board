
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

#include <map>        // Used to store the list of SPI targets by index

#define SPI_BITS_PER_S 1000000       

// This structure defines an SPI port target. 

struct spi_target  { 

  private:
            
      uint8_t const _mosi;       // Master OUT, Slave IN pin
      uint8_t const _miso;       // Master IN, Slave OUT pin      
      uint8_t const _clk;        // CLock pin
      uint8_t const _cs;         // Chip Select pin

  public:

      spi_target( uint8_t mosi, uint8_t miso, uint8_t clk , uint8_t cs ) :  _mosi{mosi} , _miso{miso} , _clk{clk} , _cs{ cs } {};
        
      void init() const {

        digitalWrite( _mosi , LOW );
        pinMode( _mosi , OUTPUT );

        digitalWrite( _miso , LOW );  // Disable pull-up
        pinMode( _miso , INPUT );

        digitalWrite( _clk , LOW );   // Clock idle LOW
        pinMode( _clk , OUTPUT ); 
    
        digitalWrite( _cs , HIGH );   // ~CS idle HIGH
        pinMode( _cs  , OUTPUT );
      }

      void start() const {
        digitalWrite( _cs , LOW);
      }
     
      void end() const {
        digitalWrite( _cs , HIGH);
      }

      uint8_t transfer( const uint8_t b ) const {

        // Calculate delay bewteen clock transitions (all done at compile time)
        // Note that we will run slightly slower than this due to overhead.
        // Easy to compensate for if needed. 
        constexpr byte CLOCKS_PER_BIT = 2; 
        constexpr unsigned long CLOCKS_PER_S = SPI_BITS_PER_S * CLOCKS_PER_BIT;
        constexpr unsigned long NS_PER_S = 1000000000UL;
        constexpr unsigned long NS_PER_CLOCK = NS_PER_S / CLOCKS_PER_S;

        uint8_t in_val =0;
        uint8_t bit_mask = 0b10000000;      // MSB first is standard 

        do {

          digitalWrite( _mosi , b & bit_mask );
          delayNanoseconds( NS_PER_CLOCK );                    
          digitalWrite( _clk, HIGH );
          
          in_val <<=1;
          in_val |= digitalRead( _miso ); 
          
          delayNanoseconds( NS_PER_CLOCK );                    
          digitalWrite( _clk, LOW );

          bit_mask >>= 1;

        } while (bit_mask);
        
        return in_val;
      }
          
          
};

// Define our SPI targets in a map so we can access them by `tag`, which is a single char the API
// uses to refer to a target. 

static const std::map< char , spi_target > spi_targets = {

  // { tag ,  spi_port( mosi, miso, sck , cs ) }
  
  {'1', spi_target( 33 , 32 , 31 , 30 ) }, 
  {'A', spi_target(  4 ,  6 ,  2 , 37 ) },        // On the aux header on the right side of the PCB
  
};


void init_spi_targets() {

  // Hey C++, where is my `std::for_all`?
  for( auto &st : spi_targets) {
    st.second.init(); 
  }
  
}

// A UART bridge connects a virtual USB port to physical UART pins
// It automatically makes the baud rate on the pins follow the baud rate on the virtual port
// and bridges data back and forth between the two as long as `service()` is called frequently.

struct uart_bridge { 

  private:

      HardwareSerial  * const _real_port;
     
      usb_serial2_class  * const _virtual_port;

      boolean virtual_port_active_state = false;   // Is this port currently "active"
         

  public:
  
      uart_bridge(  HardwareSerial  * const real_port,  usb_serial2_class * const virtual_port  ): _real_port{ real_port} , _virtual_port{ virtual_port} {};

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

  uart_bridge( &Serial2 , &SerialUSB1 ),      // TEC_UART connection on M50
  
};


// Call this periodically to keep the uart bridges fresh

void service_uart_bridges() {

  // Hey C++, where is my `std::for_all`?
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


//spi_target spi_a = spi_target(  4 ,  6 ,  2 , 37 );

void setup()
{

  // Note that Teesy best practice is to not do Serial.begin()
  
  delay(100);    // Give host a moment to warm up USB 
  Serial.println(";Testboard command port. See https://github.com/bigjosh/Voyant-test-board");
  
  // Intialize outbound UART (`TEC_UART` on board). Does not matter what baud we pick, we will adjust the output baud to match whatever
  // the USB virtual port baud is before each send. 
  Serial2.begin(9600);

  init_spi_targets();
  
}



void loop() {

  // Maintain the uart bridges 
  service_uart_bridges();
  
  auto const i =  spi_targets.find('A');

  if (i != spi_targets.end()) {

    spi_target s = i->second;

    s.start();
    s.transfer( 'J' );
    s.transfer( 'L' );
    s.end();

    delay(100);
    
  } else {
    Serial.println("map not found.");
  }


/*    
    spi_a.start();
    spi_a.transfer( 'J' );
    spi_a.transfer( 'O' );
    spi_a.transfer( 'S' );
    
    spi_a.transfer( 'H' );    
    spi_a.end();

    delay(100);
*/
  
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
