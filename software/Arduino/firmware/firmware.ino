
/*
 * **** Firmware for Voyant Test Board (c)2021 josh.com
 * **** https://github.com/bigjosh/Voyant-test-board
 * 
 * To use this sketch, you must... 
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

#include <map>        // Used to store the list of SPI targets by index

// Start slow, we can always speed up later if needed. Global for now, but also possible to break this out per SPI target.
#define SPI_BPS 1000000    

// Length of maximum line on command port
#define COMMAND_MAX_LEN 255

// This structure defines an SPI port target. 
// For now all SPI is soft and bitbanged. This is easier since we don't need to worry about picking special pins, and
// almost certainly will be able to go as fast as that ribbon cable can take.

struct Spi_target  { 

  private:
            
      uint8_t const _mosi;       // Master OUT, Slave IN pin
      uint8_t const _miso;       // Master IN, Slave OUT pin      
      uint8_t const _clk;        // CLock pin
      uint8_t const _cs;         // Chip Select pin

      uint32_t const _ns_per_cphase;        // Nanoseconds per clock phase (sets max speed)

      // Compute the delay in ns between clock phases for a given SPI bits per second
      uint32_t bps_to_npc( uint32_t const bits_per_s ) {

        // Calculate delay bewteen clock phases (all done at compile time)
        // Note that we will run slightly slower than this due to overhead.
        // Easy to compensate for if needed. 
        const byte CLOCKPHASES_PER_BIT = 2; 
        const unsigned long clockphases_per_second = bits_per_s * CLOCKPHASES_PER_BIT;
        const unsigned long NS_PER_S = 1000000000UL;
        const unsigned long ns_per_clockphase= NS_PER_S / clockphases_per_second ;

        return ns_per_clockphase;
        
      }


      void start() const {
        digitalWrite( _cs , LOW);
      }
     
      void end() const {
        digitalWrite( _cs , HIGH);
      }

      uint8_t exchangeByte( const uint8_t b ) const {

        uint8_t in_val =0;
        uint8_t bit_mask = 0b10000000;      // MSB first is standard 

        do {

          digitalWrite( _mosi , b & bit_mask );
          delayNanoseconds( _ns_per_cphase );                    
          digitalWrite( _clk, HIGH );
          
          in_val <<=1;
          in_val |= digitalRead( _miso ); 
          
          delayNanoseconds( _ns_per_cphase );                    
          digitalWrite( _clk, LOW );

          bit_mask >>= 1;

        } while (bit_mask);
        
        return in_val;
      }
      

  public:

      Spi_target( uint8_t mosi, uint8_t miso, uint8_t clk , uint8_t cs , uint32_t bps ) :  _mosi{mosi} , _miso{miso} , _clk{clk} , _cs{ cs } , _ns_per_cphase{ bps_to_npc(bps) }  {};
        
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

      // Transfer

      void transferBuffer( const uint8_t *  tx_buffer , uint8_t * rx_buffer , uint8_t len ) const {

        start();

        while (len) {

          *rx_buffer = exchangeByte( *tx_buffer );

          rx_buffer++;
          tx_buffer++;
          len--;
          
        }

        end();
        
      }
          
          
};


// A UART bridge connects a virtual USB port to physical UART pins
// It automatically makes the baud rate on the pins follow the baud rate on the virtual port
// and bridges data back and forth between the two as long as `service()` is called frequently.

struct Uart_bridge { 

  private:

      HardwareSerial  * const _real_port;
     
      usb_serial2_class  * const _virtual_port;

      boolean virtual_port_active_state = false;   // Is this port currently "active"
         

  public:
  
      Uart_bridge(  HardwareSerial  * const real_port,  usb_serial2_class * const virtual_port  ): _real_port{ real_port} , _virtual_port{ virtual_port} {};

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


struct Command_port {

  private :

    Stream &_s;
    const std::map< char , Spi_target > &_spi_targets; 

    static const char COMMAND_DELAY = 'D';
    static const char RESPONSE_COMPLETED = 'C';

    static const char COMMAND_TRANSFER = 'T';
    static const char RESPONSE_SUCESS = 'S';
    static const char RESPONSE_UNKNOWNTARGET = 'U';
    
    static const char RESPONSE_ERROR = 'E';

  

    char inputLine[COMMAND_MAX_LEN+1]; // Leave room for terminating null
    uint8_t inputLineLen=0;

    void processTransfer( const char *s ) {

      const char tag = s[0];    // First byte is the SPI target tag

      auto const spi_target_touple =  _spi_targets.find( tag );

      if (spi_target_touple == _spi_targets.end()) {

        // tag not found in spi_targets

        _s.println( RESPONSE_UNKNOWNTARGET );
        return;
      }
    
      Spi_target spi_target = spi_target_touple->second;

      const char * spi_buffer = s+1;

      // convert from hex, transfer, convert back


      _s.print( RESPONSE_SUCESS );
      _s.println( "XX" );
                
    }


    void processDelay( const char *s ) {
      int d = atoi( s );
      delay(d);
      _s.println(RESPONSE_COMPLETED);   // Delay complete.
    }

    void processLine( char *l ) {

      switch (l[0]) {
        
        case COMMAND_DELAY:
            processDelay( l+1 );
            break;

        case COMMAND_TRANSFER:
            processTransfer( l+1 );
            break;

        default:
            _s.println( RESPONSE_ERROR );  // Unknown command
            
      }              
    }
    

  public:

    Command_port( Stream &s , const std::map< char , Spi_target > spi_targets ) : _s{s} , _spi_targets{spi_targets}  {}

    void service() {

      while (_s.available()) {
    
        char c = Serial.read(); 
    
        if (c=='\n' || c=='\r') {   // This lets us be \n \r agnostic and always ignore the 2nd one. 
    
          if (inputLineLen>0) {
          
            inputLine[inputLineLen]=0x00;

            processLine( inputLine );
            inputLineLen=0;
            
          }
          
        } else if (inputLineLen<COMMAND_MAX_LEN) {
          
          inputLine[inputLineLen++]=c;      
          
        }
        
      }    
    }
  
};



// Define our SPI targets in a map so we can access them by `tag`

static const std::map< char , Spi_target > spi_targets = {

  // key in the map is the `target` char that the API uses to specify which SPI target
  // spi_target( uint8_t mosi, uint8_t miso, uint8_t clk , uint8_t cs , ) 
  
  {'1', Spi_target( 33 , 32 , 31 , 30 , SPI_BPS ) }, 
  {'A', Spi_target(  4 ,  6 ,  2 , 37 , SPI_BPS ) },        // On the aux header on the right side of the PCB
  
};


// Define any desired UART bridges here
// uart_bridge(  HardwareSerial  * const real_port,  usb_serial2_class * const virtual_port  )

Uart_bridge uart_bridges[] = {

  Uart_bridge( &Serial2 , &SerialUSB1 ),      // TEC_UART connection on M50 to secondary USB serial connection to host
  
};

// Define our command port here

Command_port command_port( Serial , spi_targets );          // The default serial connection to USB host.



void init_spi_targets() {

  // Hey C++, where is my `std::for_all`?
  for( auto &st : spi_targets) {
    st.second.init(); 
  }
  
}

// Call this periodically to keep the uart bridges fresh

void service_uart_bridges() {

  // Hey C++, where is my `std::for_all`?
  for( auto &ub : uart_bridges ) {
    ub.service(); 
  }
  
}

// Process any commands recieved on the command port

void service_command_port() {

  command_port.service(); 
  
}


void setup() {

  init_spi_targets();
  
  // Note that Teesy best practice is to not do Serial.begin()
  
  delay(100);    // Give host a moment to warm up USB 
  
  
    
}



void loop() {

  // Maintain the uart bridges 
  service_uart_bridges();
  service_command_port();
  
}
