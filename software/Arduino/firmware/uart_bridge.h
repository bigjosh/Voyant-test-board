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
