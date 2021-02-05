## SW-STM32-USB-CDC
Reinventing the Wheel, because why not

should be slightly better than the USB CDC provided by stm32cubemx in terms of speed and code size, didn't bother to actually get some data.

## How to use
create instances of the handles:

    SWHAL_USB_PCD_HandleTypeDef swpcd;
    SWHAL_USB_CDC_HandleTypeDef swcdc;
then call init function:

    SWHAL_USB_CDC_Init(&hpcd_USB_FS, &swpcd, &swcdc);
then use `SWHAL_USB_CDC_Transmit` and `SWHAL_USB_CDC_Receive` 

## Catches

 - receive buffer need to be 32-bit aligned
 - havent been thoroughly tested, might be buggy ¯\\\_(ツ)_/¯

