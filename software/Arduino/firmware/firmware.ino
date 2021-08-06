
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

// Max length in bytes of maximum SPI transaction. 
#define SPI_MAX_LEN 255

// Send comments over the command port that help in debugging problems
#define DEBUG 1

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


// This structure defines an IO port target. 

struct Io_target  { 

  private:
            
      uint8_t const _pin; 

     
  public:

      Io_target( uint8_t pin ) :  _pin{pin}  {};
        
      void init() const {

        pinMode( _pin , OUTPUT );

      }
      
      void set( boolean bit ) const {
        digitalWrite( _pin , bit );
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
    const std::map< char , Spi_target > *_spi_targets; 
    const std::map< char , Io_target > *_io_targets; 

    static const char COMMENT_CHAR = ';';

    static const char COMMAND_DELAY = 'D';
    static const char RESPONSE_DELAY = 'C';

    static const char COMMAND_TRANSFER = 'T';
    static const char RESPONSE_TRANSFER = 'S';
    
    static const char COMMAND_IO = 'I';
    static const char RESPONSE_IO = 'H';
    
    static const char RESPONSE_ERROR = 'E';

    static const unsigned LINE_MAX_LEN = (SPI_MAX_LEN*2) + 2; // The biggest command is SPI and has command byte+target byte+data in 2 byte ASCII.

    // Copied from https://stackoverflow.com/questions/6457551/hex-character-to-int-in-c
    // No error checking
    uint8_t hexdigit2value( const char c ) {
      uint8_t v = (c >= 'A') ? (c >= 'a') ? (c - 'a' + 10) : (c - 'A' + 10) : (c - '0');
      return v;
    }

    void send_response( const char c ) {
      _s.println( c );  
    }

    // Send the reponse code followed by the bytes in the buffer as ASCII hex digits
    void send_response_with_data( const char c , const uint8_t *b, unsigned len ) {
      _s.print( c );  

      while (len) {

        _s.printf( "%2.2x" , *b );      // Print as base 16 

        b++;
        len--;
      }

      _s.println();
      
    }

    void send_comment( const char *s ) {
        _s.print( COMMENT_CHAR );
        _s.println( s );        
    }
    
    void send_response_error( const char *s ) {

      if (DEBUG) {
        send_comment( s );
      }
      send_response( RESPONSE_ERROR );  
    }

    void processIoCommand( const char *s , unsigned len ) {

      unsigned pos =0;    // Where we are at parsing the input line

      // First look up the tag

      if (len<=pos) {
        // Line too short for tag
        send_response_error("No tag, IO request too short");
        return;        
      }

      const char tag = s[pos++];    // First byte is the SPI target tag

      auto const io_target_tupple =  _io_targets->find( tag );

      if (io_target_tupple == _io_targets->end()) {
        // Tag not found in map
        send_response_error("Bad tag in IO request");
        return;
      }

      // Get the spi_target from the map tupple (I feel like {key,value} struct would be better than a tupple here, C++?)
      Io_target io_target = io_target_tupple->second;

      // Next parse the output level 

      if (len<=pos) {
        // Line too short for level
        send_response_error("No level, IO request too short");
        return;        
      }
      
      const char level_char = s[pos++];

      if (len!=pos) {        
        send_response_error("IO request too long");
        return;        
      }

      switch (level_char) {

        case '0': 
          io_target.set( false ); 
          break;
          
        case '1': 
          io_target.set( true ); 
          break;

        default:
          send_response_error("Invalid level in IO request");
          return;        
        
      }

    }    
    

    void processTransferCommand( const char *s , unsigned len ) {

      unsigned pos =0;    // Where we are at parsing the input line

      // First look up the tag

      if (len<=pos) {
        // Line too short for tag
        send_response_error("No tag, SPI request too short");
        return;        
      }

      const char tag = s[pos++];    // First byte is the SPI target tag

      auto const spi_target_tupple =  _spi_targets->find( tag );

      if (spi_target_tupple == _spi_targets->end()) {
        // Tag not found in map
        send_response_error("Bad tag in SPI request");
        return;
      }

      // Get the spi_target from the map tupple (I feel like {key,value} struct would be better than a tupple here, C++?)
      Spi_target spi_target = spi_target_tupple->second;

      // Next parse the ASCII hex digits to a byte[]


      // A more responsible programmer would only allocate the actual number of bytes here, but this parser is already starting to feel bloated and we have plenty of RAM
      byte mosi_data_buffer[SPI_MAX_LEN];

      unsigned data_buffer_len = 0;

      while (pos<len) {

        if (data_buffer_len >=SPI_MAX_LEN) {
          send_response_error("More than SPI_MAX_LEN bytes in SPI request");
          return;
        }

        // Parse each pair of hex digits

        const char hdigit = s[pos++];

        if (pos>=len) {
          send_response_error("Incomplete ASCII hex byte in SPI request");
          return;        
        }

        if (!isxdigit(  hdigit )) {
          send_response_error("Invalid high ASCII hex digit in SPI request");
          return;        
        }

        const char ldigit = s[pos++];

        if (!isxdigit(  ldigit )) {
          send_response_error("Invalid low ASCII hex digit in SPI request");
          return;        
        }

        const uint8_t b = hexdigit2value( hdigit ) << 4 | hexdigit2value( ldigit ) ;

        Serial.print( b , 16 );
        

        mosi_data_buffer[ data_buffer_len++ ] = b;
                        
      }

      byte miso_data_buffer[data_buffer_len];   // Buffer for incoming SPI bytes
      Serial.println( data_buffer_len );
      

      spi_target.transferBuffer( mosi_data_buffer , miso_data_buffer , data_buffer_len );


      send_response_with_data( RESPONSE_TRANSFER , miso_data_buffer , data_buffer_len );
              
    }

    
    // Note that delay will overflow if greater than ULONG_MAX
    void processDelayCommand( const char *arg , const unsigned len ) {
      
      unsigned long delay_ms =0;

      unsigned pos =0;    

      if (pos==len) {
          send_response_error("Delay request missing millis");
          return;        
      }
      
      // Parse number of millis
      while (pos<len) {
        const char digit = arg[pos];
        if (!isdigit(digit)) {
          send_response_error("Bad digit in delay");
          return;
        }
        delay_ms*=10;
        delay_ms += digit-'0';

        pos++;
      }

      delay(delay_ms);

      send_response( RESPONSE_DELAY );
    }

    char inputLine[LINE_MAX_LEN]; // Leave room for terminating null
    unsigned inputLineLen=0;
    
    void processLine( char *l , unsigned len ) {

      switch (l[0]) {
        
        case COMMAND_DELAY:
            
            processDelayCommand( l+1 , len-1 );
            break;

        case COMMAND_TRANSFER:
        
            processTransferCommand( l+1 , len-1 );
            break;

        case COMMAND_IO:
        
            processIoCommand( l+1 , len-1 );
            break;
            

        case COMMENT_CHAR:

            // This page intionally left blank
            break;

        default:
            send_response_error( "Unknown command");
            break;
            
      }              
    }
    

  public:

    Command_port( Stream &s , const std::map< char , Spi_target > *spi_targets , const std::map< char , Io_target > *io_targets ) : _s{s} , _spi_targets{spi_targets} , _io_targets{io_targets}  {

      if (DEBUG) {
        send_comment( "Voyant-test-board firmware (c) 2021 josh.com");
      }
      
    }

    void service() {

      while (_s.available()) {
    
        char c = Serial.read(); 
    
        if (c=='\n' || c=='\r') {   // This lets us be \n \r agnostic and always ignore the 2nd one. 
    
          if (inputLineLen>0) {
          
            processLine( inputLine , inputLineLen );
            inputLineLen=0;
            
          }
          
        } else if (inputLineLen<LINE_MAX_LEN) {
          
          inputLine[inputLineLen++]=c;      
          
        }
        
      }    
    }
  
};



// Define our SPI targets in a map so we can access them by `tag`

static const std::map< char , Spi_target > spi_targets = {

  // key in the map is the `target` char that the API uses to specify which SPI target
  // Taken directly from schamtics. The numbers here refer to the pin numbers on the Teensy. 
  // The tags are taken from the labels on the M50 connector diagram except for `A` which is on the AUX header.
  
  // spi_target( uint8_t mosi, uint8_t miso, uint8_t clk , uint8_t cs , uint32_t bps ) 
  
  {'1', Spi_target( 33 , 32 , 31 , 30 , SPI_BPS ) }, 
  {'2', Spi_target( 22 , 21 , 20 , 19 , SPI_BPS ) }, 
  {'3', Spi_target( 26 , 12 , 13 ,  0 , SPI_BPS ) }, 
  {'4', Spi_target( 11 , 12 , 13 , 10 , SPI_BPS ) }, 
  {'5', Spi_target( 17 , 16 , 15 , 14 , SPI_BPS ) }, 
  
  {'A', Spi_target(  4 ,  6 ,  2 , 37 , SPI_BPS ) },        // On the aux header on the right side of the PCB
  
};


// Define our IO targets in a map so we can access them by `tag`

static const std::map< char , Io_target > io_targets = {

  // key in the map is the `target` char that the API uses to specify which IO target
  // io_target( uint8_t pin ) 
  
  {'A', Io_target( 24 ) },    // ENABLE_-2V0_BIAS
  
};



// Define any desired UART bridges here
// uart_bridge(  HardwareSerial  * const real_port,  usb_serial2_class * const virtual_port  )

Uart_bridge uart_bridges[] = {

  Uart_bridge( &Serial2 , &SerialUSB1 ),      // TEC_UART connection on M50 to secondary USB serial connection to host
  
};

// Define our command port here

Command_port command_port( Serial , &spi_targets , &io_targets );          // The default serial connection to USB host.

void init_spi_targets() {

  // Hey C++, where is my `std::for_all`?
  for( auto &st : spi_targets) {
    st.second.init(); 
  }
  
}

void init_io_targets() {

  // Hey C++, where is my `std::for_all`?
  for( auto &io : io_targets) {
    io.second.init(); 
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
  init_io_targets();

}



void loop() {

  // Maintain the uart bridges 
  service_uart_bridges();
  service_command_port();
  
}
