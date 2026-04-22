# RS-232 Magellan/SpaceMouse with modern software

![Magellan/SpaceMouse connected to a Raspberry Pi Pico](magellan.jpg)

This project lets you use an old serial Magellan/SpaceMouse controller with modern software on modern operating systems. It simulates a 3DConnexion SpaceMouse Pro, so it can be used with software like Fusion 360, 3ds Max, SolidWorks, Inventor, Maya and many others. No special software is required on the computer, apart from 3DxWare. From the computer's point of view, your old Magellan will look like a real SpaceMouse Pro connected over USB.

## Features
- **Auto-Initialization:** The firmware automatically sends wake-up commands to the Magellan upon power-up. No manual button combinations are needed to start the axes.
- **Robust Startup:** Includes a "line-clearing" phase to handle power-on noise from old serial hardware.
- **SpaceMouse Pro Emulation:** Works with official 3Dconnexion drivers.

## Build Targets & Pinouts

Depending on your hardware setup, you should choose the appropriate build target.

### 1. Standard USB Target (`rp2040usb`)
Use this for a standard Raspberry Pi Pico connected via USB.

| Pico Pin | Function | Serial Adapter Pin |
| :--- | :--- | :--- |
| **3V3** (pin 36) | VCC | VCC |
| **GND** (pin 23/38) | GND | GND |
| **GPIO 20** (pin 26) | UART1 TX | RX |
| **GPIO 21** (pin 27) | UART1 RX | TX |

### 2. RS232 Adapter Target (`rp2040rs232`)
Use this for dedicated RS232-to-Pico adapters (like RP2040-RS232) or when using GPIO 0/1.

| Pico Pin | Function | Serial Adapter Pin |
| :--- | :--- | :--- |
| **3V3** (pin 36) | VCC | VCC |
| **GND** (pin 23/38) | GND | GND |
| **GPIO 0** (pin 1) | UART0 TX | RX |
| **GPIO 1** (pin 2) | UART0 RX | TX |

## Installation
1. Flash the Pico with the appropriate `.uf2` file (e.g., `rp2040rs232.uf2`).
2. Hold the **BOOTSEL** button while connecting the Pico to the computer.
3. Copy the UF2 file to the USB drive that appears.
4. Install **3DxWare** on your computer.

## Button Mapping

| Magellan Button | SpaceMouse Pro Action |
| :--- | :--- |
| **1 - 4** | Buttons 1, 2, 3, 4 |
| **5** | Esc |
| **6** (incl. side button) | Ctrl |
| **7** (incl. side button) | Alt |
| **8** | Shift |
| **\*** | Menu |

*Note: On older models, the buttons to the left and right of the puck are hardware-wired to buttons 6 and 7 respectively.*

## Long Press Feature
If enabled, holding a button for > 1 second will send a `Win + Alt + <Number>` keyboard shortcut instead of the standard button action.

## How to compile the firmware

### Standard Build (USB)
```bash
git clone https://github.com/jfedor2/magellan-spacemouse.git
cd magellan-spacemouse
git submodule update --init
mkdir build
cd build
cmake -DMAGELLAN_TARGET=rp2040usb ..
make -j$(nproc)
```

### RS232 Adapter Build
```bash
mkdir build_rs232
cd build_rs232
cmake -DMAGELLAN_TARGET=rp2040rs232 ..
make -j$(nproc)
```

### Build with Long Press enabled
Set the `MAGELLAN_LONG_PRESS_ENABLE` environment variable:
```bash
MAGELLAN_LONG_PRESS_ENABLE=ON cmake ..
```

### ALL RIGHT 