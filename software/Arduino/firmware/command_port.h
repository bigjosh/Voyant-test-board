

struct Command_port {

  private :

    Stream &_s;
    const std::map< char , Spi_target > *_spi_targets; 
    const std::map< char , Io_target > *_io_targets; 
    const boolean _debug_flag;
    
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

      if (_debug_flag) {
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

        mosi_data_buffer[ data_buffer_len++ ] = b;
                        
      }

      byte miso_data_buffer[data_buffer_len];   // Buffer for incoming SPI bytes
      

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

    Command_port( Stream &s , const std::map< char , Spi_target > *spi_targets , const std::map< char , Io_target > *io_targets , boolean debug_flag ) : _s{s} , _spi_targets{spi_targets} , _io_targets{io_targets} , _debug_flag{debug_flag} {

      if (debug_flag) {
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
