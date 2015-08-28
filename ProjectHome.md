Uses the STM32 built-in loader to load the executable code to the part. The hardware it uses is a FTDI's FT232 microcontroller. The RX and TX pins are connected to the UART, CBUS3 to RESET and CBUS2 to BOOT1. The software uses the Bootloader protocol to load the file and after that resets the part. It uses Qt GUI (qt.nokia.com).

![http://stm32-loader.googlecode.com/svn/wiki/stm32-loader.png](http://stm32-loader.googlecode.com/svn/wiki/stm32-loader.png)

https://docs.google.com/document/pub?id=1Dc6Yu-80Ew0XTu5y1XeW7yQ5tr80q_spqV4mFdXIHEc