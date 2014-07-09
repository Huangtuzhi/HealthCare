#!/bin/ash
cd ADCDriver
insmod myadc.ko
cd ../ControlDriver
insmod GPIOs.ko
echo "Insmoding All the Modules" 
