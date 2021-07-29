# Programming the PLA ALT controller

This guide will show you how to program the PLA ALT controller with new firmware. This includes burning the PLA ALT bootloader and uploading the latest code.

To burn the bootloader, two pieces of hardware are required:

1. A USBasp programmer (available from many places, like [Amazon](https://www.amazon.com/Geekstory-Microcontroller-Programmer-Downloader-Adapter/dp/B07NZ59VK2/)).
2. A 6-pin Pogo adapter ([on Amazon](https://www.amazon.com/pin-AVR-ICSP-Pogo-Adapter/dp/B075Q25BK3/)).

This guide assumes you are running Windows 10.

## 1. Prepare the USB programmer

Plug the USB programmer into the computer. Then, download and run the program Zadig from [here](https://zadig.akeo.ie/). This program installs drivers for the programmer.

Once open, the drop-down box should contain an entry (like "USBasp"). Click "Install Driver"; once finished, exit Zadig.

## 2. Prepare the Arduino IDE

The Arduino IDE can be downloaded [here](https://www.arduino.cc/en/software). Once installed, run the Arduino IDE program. This will create a folder that we will be copying files into. Just close the IDE once it is open.

## 3. Download the PLA ALT source code

Download a .zip of the latest source code [here](https://github.com/tcsullivan/pla-device/archive/refs/heads/master.zip). Once downloaded, unzip the archive. If this creates a folder named `pla-device-master`, rename the folder to `pla-device`.

Inside the `pla-device` folder is a `hardware` folder. Copy and paste the `hardware` folder into `Documents\Arduino`.

## 4. Programming the PLA ALT controller

Now, open the Arduino IDE again. From the menu bar, select **Tools > Board: > PLA: > PLA ALT**. Also select **Tools > Programmer: > USBasp**.

### a. Burning the bootloader

Plug the PLA controller into the computer, and connect the programmer to it by pressing it into the programming port on the bottom of the PLA controller (check the pin labels and make sure they line up and match):

![Programmer connection](https://raw.githubusercontent.com/tcsullivan/pla-device/master/doc/usbasp_connection.jpg)

Keep the programmer connected, and choose **Tools > Burn Bootloader** from the menu bar. Once the bootloader burning is finished, you may remove the programmer from the PLA controller.

### b. Uploading the code

Under **Tools > Port**, we need to select the COM port for the PLA controller. If there are multiple COM ports, open Device Manager (can be found by pressing Windows key + X) and look under "Ports (COM & LPT)". The PLA controller will appear as a Sparkfun Pro Micro.

Once the port is selected, simply press **Sketch > Upload**. When finished, reconnect the controller and the blue/white LEDs should light up.

## Notes

* Once the bootloader has been burned to a device, it does not need to be burned again (unless there is a new bootloader version).
* To program the PLA controller after the bootloader has been uploaded, plug the contoller into the computer **while** pressing down the right joystick. The controller's lights will not turn on if you successfully entered the programming mode.
* If an error occurs while working with the Arduino IDE, go to **File > Prefences** and next to "Show verbose output during:" check both the compilation and upload boxes. Repeat the step you were stuck on, and detailed information on what went wrong should be given.

