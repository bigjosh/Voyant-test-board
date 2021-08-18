## SPI1 - AD7490

CLK idle HIGH, sample on CS falling edge?

>The remaining
>address bits and data bits are then clocked out by subsequent
>SCLK falling edges, beginning with the second address bit,
>ADD2. Thus, the first SCLK falling edge on the serial clock has
>the ADD3 address bit provided and also clocks out address bit
>ADD2. The final bit in the data transfer is valid on the 16th
>falling edge, having being clocked out on the previous (15th)
>falling edge. 

https://www.analog.com/media/en/technical-documentation/data-sheets/AD7490.pdf

![image](https://user-images.githubusercontent.com/5520281/128620922-b7b4e311-b467-48be-b99b-3b708226ea6d.png)

We always write 16 bits and at the same time read the 16 bit result of the previous conversion. 

![image](https://user-images.githubusercontent.com/5520281/129519296-35390252-b687-4a90-93a0-1436824a5ece.png)

WRITE must be 1. PMx should both be 1 (power on). Shadow 0 for testing. WEAK is interesting. If 1 then the first bit of the address of previous convversion appears on DOUT when CS goes low - we want this so we always get the address back on evvery conversion. 

You initialize the interface by sending two transactions in a row of 0xFFFF each....
![image](https://user-images.githubusercontent.com/5520281/129949930-3144aa47-48ca-4dd5-a414-fa349dcbd214.png)

So for testing we will send 0b10aaaa110101. We will change aaaa and see if the aaaa changes in the next readout. 


## SPI2 - AD7124

Mode 3

https://www.analog.com/media/en/technical-documentation/data-sheets/AD7124-4.pdf
Page 38:
![image](https://user-images.githubusercontent.com/5520281/128595692-47a557a0-cf09-4c8f-a3a2-18ac19ede19b.png)

## SPI3 & SPI4 - AD5766

Looks like CLK idle HIGH, sampled on LOW - so Mode 2

24 bit(3 byte) commands. 

https://www.analog.com/media/en/technical-documentation/data-sheets/ad5766-5767.pdf

![image](https://user-images.githubusercontent.com/5520281/128617623-0770ec1c-e9e3-4c07-9d78-ab3628852f33.png)

![image](https://user-images.githubusercontent.com/5520281/128617543-efe30121-c968-4553-89db-e8ff2c0b3cd7.png)


Maximum SCLK frequency is 50 MHz for write mode and 10 MHz for readback mode.
Minimum time between a reset and the subsequent successful write is typically 25 ns.

100ns typ RESET2 pulse width low
100ns typ RESET2 pulse activation time

## SPI 5 - AD5142

Looks like CLK idle HIGH, sampled on LOW - so Mode 2

Chip is WRITE only. DOUT is only for daisy chain. 

https://www.analog.com/media/en/technical-documentation/data-sheets/AD5122_5142.pdf

"Data is loaded in at the SCLK falling edge transition, as
shown in Figure 3 and Figure 4. "

![image](https://user-images.githubusercontent.com/5520281/128621491-51e2cff3-53d7-482e-9348-e0ebac47a23f.png)





