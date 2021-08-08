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
