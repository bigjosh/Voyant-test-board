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

      uint8_t const _mode;       // SPI mode 0-3. Info here: https://www.analog.com/en/analog-dialogue/articles/introduction-to-spi-interface.html
      

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


          // I know this is verbose, but it is clear and I get easily confused looking at these silly modes
          // Note that the Analog Devices "Intro to SPI" is just wrong...
          // https://electronics.stackexchange.com/questions/580610/is-this-diagram-for-spi-mode-2-wrong
          // ..so I used this reference instead...
          // http://dlnware.com/theory/SPI-Transfer-Modes
          
          switch (_mode) {
                           
            case 0: {
  
                // CLK idle low, Data sampled on rising edge 
  
                digitalWrite( _mosi , b & bit_mask );                
                delayNanoseconds( _ns_per_cphase );  

                if (digitalRead( _miso )) {
                  in_val |= bit_mask;  
                }
                                
                digitalWrite( _clk, HIGH );
                delayNanoseconds( _ns_per_cphase );                    
                digitalWrite( _clk, LOW );                                      
              
              };
              break;

            case 1: {
  
                // CLK idle low, Data sampled on falling edge 

                digitalWrite( _clk, HIGH );              
                digitalWrite( _mosi , b & bit_mask );                
                delayNanoseconds( _ns_per_cphase );  

                if (digitalRead( _miso )) {
                  in_val |= bit_mask;  
                }
                                
                digitalWrite( _clk, LOW );                                      
                delayNanoseconds( _ns_per_cphase );                    
              
              };
              break;
              
  
            case 2: {
  
                // CLK idle HIGH, Data sampled on falling edge
  
                digitalWrite( _mosi , b & bit_mask );
                delayNanoseconds( _ns_per_cphase );  

                if (digitalRead( _miso )) {
                  in_val |= bit_mask;  
                }
                
                digitalWrite( _clk, LOW );
                delayNanoseconds( _ns_per_cphase );                    
                digitalWrite( _clk, HIGH );                                      
              
              };
              break;
  
            case 3: {
  
                // CLK idle HIGH, Data sampled on rising edge 

                delayNanoseconds( _ns_per_cphase );                    
                digitalWrite( _clk, LOW );
                
                digitalWrite( _mosi , b & bit_mask );                                                                
                delayNanoseconds( _ns_per_cphase ); 
                
                if (digitalRead( _miso )) {
                  in_val |= bit_mask;  
                }
                
                digitalWrite( _clk, HIGH );                                      
              
              };
              break;

            case 5: {
  
                // CLK idle HIGH, I made this up so that MOSI is valid on both edges. You need a logic analyzer then to see whats going on. 

                digitalWrite( _mosi , b & bit_mask );                                                
                delayNanoseconds( _ns_per_cphase );                 
                digitalWrite( _clk, LOW );                
                delayNanoseconds( _ns_per_cphase ); 
                in_val |= digitalRead( _miso ); 
                digitalWrite( _clk, HIGH );                                      
                delayNanoseconds( _ns_per_cphase );                    
              
              };
              break;
              
            
          }

          bit_mask >>= 1;

        } while (bit_mask);
        
        return in_val;
      }
      

  public:

      Spi_target( uint8_t mosi, uint8_t miso, uint8_t clk , uint8_t cs , uint32_t bps , uint8_t mode ) :  _mosi{mosi} , _miso{miso} , _clk{clk} , _cs{ cs } , _ns_per_cphase{ bps_to_npc(bps)} , _mode{mode}   {};
        
      void init() const {

        pinMode( _mosi , OUTPUT );
        digitalWrite( _mosi , LOW );

        pinMode( _miso , INPUT );
    
        pinMode( _cs  , OUTPUT );
        digitalWrite( _cs , HIGH );   // ~CS idle HIGH
        

        pinMode( _clk , OUTPUT ); 

        if (  _mode == 0 || _mode == 1 ) {

          digitalWrite( _clk , LOW );   // Clock idle LOW
          
        } else {

          digitalWrite( _clk , HIGH );   // Clock idle HIGH
          
        }

          
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
