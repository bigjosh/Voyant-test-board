
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
#define DEBUG true

#include "spi_target.h"
#include "uart_bridge.h"
#include "command_port.h"


// Define our SPI targets in a map so we can access them by `tag`

static const std::map< char , Spi_target > spi_targets = {

  // key in the map is the `target` char that the API uses to specify which SPI target
  // Taken directly from schamtics. The numbers here refer to the pin numbers on the Teensy. 
  // The tags are taken from the labels on the M50 connector diagram except for `A` which is on the AUX header.
  
  // spi_target( uint8_t mosi, uint8_t miso, uint8_t clk , uint8_t cs , uint32_t bps , uint8_t mode  ) 
  
  {'1', Spi_target( 33 , 32 , 31 , 30 , SPI_BPS , 2 ) },    // SPI1-AD7490, CLK idle HIGH, Sample on FALL, MSb first - https://www.analog.com/media/en/technical-documentation/data-sheets/AD7490.pdf
  {'2', Spi_target( 22 , 21 , 20 , 19 , SPI_BPS , 3 ) },    // SPI2-AD7124, CLK idle HIGH, Sample on RISE, MSb first - https://www.analog.com/media/en/technical-documentation/data-sheets/AD7124-4.pdf
  {'3', Spi_target( 26 ,  1 , 27 ,  0 , SPI_BPS , 2 ) },    // SPI3-AD5766, CLK idle HIGH, Sample on FALL, MSb first - https://www.analog.com/media/en/technical-documentation/data-sheets/ad5766-5767.pdf
  {'4', Spi_target( 11 , 12 , 13 , 10 , SPI_BPS , 2 ) },    // SPI4-AD5766, CLK idle HIGH, Sample on FALL, MSb first - https://www.analog.com/media/en/technical-documentation/data-sheets/ad5766-5767.pdf
  {'5', Spi_target( 17 , 16 , 15 , 14 , SPI_BPS , 1 ) },    // SPI5-AD5142, CLK idle HIGH, Sample on FALL, MSb first - https://www.analog.com/media/en/technical-documentation/data-sheets/AD5122_5142.pdf
  
  {'A', Spi_target(  4 ,  6 ,  2 , 37 , SPI_BPS , 1 ) },        // On the aux header on the right side of the PCB - For galvo. 
  
};


// Define our IO targets in a map so we can access them by `tag`

static const std::map< char , Io_target > io_targets = {

  // key in the map is the `target` char that the API uses to specify which IO target. 
  // The tags are arbitrary (but meant to hopefully be mnemonic)   
  // The pin refers to the Teensy pin number. 
  // io_target( uint8_t pin ) 

  // The comment is the label on the pin on the M50 connector
  
  {'N', Io_target( 24 ) },    // ENABLE_-2V0_BIAS
  {'P', Io_target( 25 ) },    // ENABLE_2V0_BIAS
  
  {'3', Io_target(  3 ) },    // AD5766_A_RST_N (SPI3)
  {'4', Io_target(  9 ) },    // AD5766_B_RST_N (SPI4)
  {'5', Io_target( 18 ) },    // POT_RST_N      (SPI5)

  {'T', Io_target(  5 ) },    // TEC_ENABLE_N   (TEC_UART)

  // The comment is the label on the Aux header  

  {'9', Io_target( 39 ) },    // D39
  {'8', Io_target( 38 ) },    // D38
  
};



// Define any desired UART bridges here
// uart_bridge(  HardwareSerial  * const real_port,  usb_serial2_class * const virtual_port  )

Uart_bridge uart_bridges[] = {

  Uart_bridge( &Serial2 , &SerialUSB1 ),      // TEC_UART connection on M50 to secondary USB serial connection to host
  
};

// Define our command port here

Command_port command_port( Serial , &spi_targets , &io_targets , DEBUG );          // The default serial connection to USB host.

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
