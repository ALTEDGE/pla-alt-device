# pla-alt-device

This repository contains the source code and hardware design files for the PLA ALT Avatar Motion Controller.

To compile the source code, use the [Arduino IDE](https://www.arduino.cc/en/software). After running the Arduino IDE, an `Arduino` folder is created in your Documents folder. Copy the `hardware` folder in this repo into that `Arudino` folder; then, the Arduino IDE should show "PLA ALT" as a board option.

The controller's hardware was designed using [EAGLE](https://www.autodesk.com/products/eagle/overview). Schematic and board files are located in the `eagle` folder.

## Brief user guide

The controller appears to the computer as a regular gaming joystick/controller. The [PLA ALT GUI](https://github.com/ALTEDGE/pla-alt-gui) must be running in order to use the controller to its full potential.

To reprogram the ALT controller, put it into programming mode by holding down the right joystick button while plugging into the computer; the LEDs should not turn on. After uploading new code through the IDE, reconnect the controller.

The ALT controller self-calibrates when it is first powered on. If recalibration is ever necessary, hold down the left joystick button while plugging into the computer. Release the button once the LEDs light up white. Calibration is finished once the LEDs return to blue.

