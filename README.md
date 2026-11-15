# Plant Waterer

Plant Waterer is a C application for a BeagleBone-style Linux board. It provides
HAL modules for a four-channel relay board, an SPI ADC, a joystick, and a
capacitive moisture sensor.

## Current application behavior

The current `main` initializes the four relay outputs and turns each relay on
for one second, then off, in sequence. It then releases the GPIO resources and
exits.

The joystick and moisture sensor HALs are implemented, but their sampling code
in `app/src/main.c` is currently commented out, so the application does not
currently read either sensor.

## Project layout

```text
app/
  src/main.c                 Application entry point
hal/
  include/hal/               Public HAL interfaces
  src/                       HAL implementations
CMakeLists.txt               Project build configuration
```

The HAL modules are:

- `relay`: controls four GPIO relay outputs using libgpiod.
- `adc`: opens/configures an SPI device and reads a 12-bit ADC channel.
- `joystick`: reads ADC channels 0 and 1 and computes joystick direction.
- `cap_moisture`: reads the capacitive moisture sensor from ADC channel 2.
- `lcd`: drives a 16x2 HD44780 LCD through a PCF8574 I2C backpack.

Joystick and moisture each own their own ADC/SPI state. They can be initialized,
read, and cleaned up independently.

## Hardware configuration 

The sensor examples use `/dev/spidev0.0` at 250 kHz. Update the device path and
speed in the application if your board uses a different SPI device or setup.

The relay GPIO chip paths and line offsets are configured in `app/src/main.c`.
Verify those values against your board's GPIO mapping and relay wiring before
running the application.

The LCD HAL defaults to `/dev/i2c-1` and address `0x3F`. On the BYAI, connect
SDA to physical pin 3 and SCL to physical pin 5. The LCD backpack must use the
PCF8574 mapping expected by the HAL: P0=RS, P1=RW, P2=EN, P3=backlight, and
P4-P7=D4-D7. Pass a different device path or address in `lcd_hal_config_t`
when the hardware differs.

The board must have SPI enabled, the ADC and sensors connected to the expected
channels, and GPIO/SPI device permissions that allow the application to access
the hardware. The current relay implementation links against libgpiod.

## Build

Install CMake, a C compiler, and the libgpiod development files for the build
environment. Configure and build with:

```sh
cmake -S . -B build
cmake --build build
```

The executable is produced at `build/app/plant_waterer`.

Build with a compiler/toolchain matching the BeagleBone's architecture and
system libraries when building off-device. A native host build may produce an
x86-64 executable that cannot run on the board. Check the executable before
copying it:

```sh
file build/app/plant_waterer
```

Transfer the target-compatible executable to the board using your normal
deployment method, then run it there. Hardware access may require elevated
permissions, for example:

```sh
sudo ./plant_waterer
```

## Adding or using HAL modules

HAL headers are included with the `hal/` prefix, for example:

```c
#include "hal/cap_moisture.h"
#include "hal/joystick.h"
#include "hal/relay.h"
```

The HAL library is assembled from the C files in `hal/src/`. Reconfigure CMake
after adding or removing source files so the build system picks them up.
