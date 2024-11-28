#!/bin/bash

./build.sh

../SREC-UART-Loader/uart_flasher sources/build/kernel.srec /dev/tty.usbserial-006768F1

