# Word Clock — D1 mini (ESP8266)

An Upper Austrian German word clock on a Wemos D1 mini lite, driven by
[clock.md](clock.md) (Word Clock Spec v5). The device keeps time via NTP and
serves its 11×11 letter matrix as an HTML page.

## Hardware

- Wemos D1 mini lite (ESP8266)
- No peripherals required — the "display" is the web page served on port 80

## How it works

- The clock state is held in `char frame[3][11][11]`: the first dimension
  indexes the R, G and B planes, each `[row][column]` cell holding one byte.
  Active letters are `255,255,255` (white), inactive letters `64,64,64`
  (gray), cells without a letter stay dark.
- Every minute (on the minute change, checked in `loop()`), the frame is
  rebuilt from the current local time: the spec's time computation picks the
  active word instances (ES, IST, counter phrase, base phrase) and lights
  the union of their cells.
- The letter grid, word spans and sentence logic live in
  [`src/clock.h`](src/clock.h) as pure C++ with no Arduino dependencies, so
  they can be unit-tested on the host.
- A request to `/` renders the matrix as an HTML table (UTF-8, auto-refresh
  every 60 s). Before the first NTP sync all letters show dim gray.

Word layout (11×11, Upper Austrian German):

```text
E S . I S T . D R E I
Z W E I N S . V I E R
N A C H V O R F Ü N F
. . . Z E H N . V O R
N A C H . H A L B . .
D R E I V I E R T E L
Ü B E R . A C H T . .
D R E I N S Z W Ö L F
Z W E I V I E R E L F
S E C H S S I E B E N .
Z E H N E U N F Ü N F
```

Scanning the lit cells row by row reads the current time as a sentence in
order, e.g. 14:53 → „es ist drei nach zehn vor drei".

## Project layout

```text
platformio.ini      PlatformIO configuration (d1_mini_lite)
clock.md            Word clock specification (v5)
src/clock.h         letter grid, word instances, time-to-words logic
src/main.cpp        WiFi + NTP + HTTP server, per-minute frame update
test/test_clock.cpp host unit test (no hardware needed)
```

## Build, test, flash

```sh
./flash.sh          # build + flash over USB serial (default)
./flash.sh ota      # build + flash over WiFi (ArduinoOTA), no cable needed
```

`flash.sh` builds the firmware and uploads it, either over the USB serial
port or over WiFi. How to choose:

- **Serial** (`./flash.sh` or `./flash.sh serial`) — the safe default. Works
  no matter what state the device is in, as long as it is plugged in via
  USB (`/dev/ttyUSB0`). Use this for the first flash after a fresh chip,
  and to recover if an OTA-pushed firmware ever breaks WiFi.
- **OTA** (`./flash.sh ota`) — requires the device to be powered, connected
  to WiFi and reachable as `WordClock.lan` (the DHCP hostname it announces;
  falls back to `192.168.8.115`). The upload goes through `ArduinoOTA` on
  TCP port 8266 and is authenticated with the password from
  `include/secrets.h` — the script reads it from there, so the device and
  the uploader always match.

During an OTA transfer the clock pauses for about a minute (no web page,
frozen matrix), then reboots into the new firmware.

Plain PlatformIO commands still work:

```sh
pio run                                # build firmware
pio run -e d1_mini_lite -t upload      # flash over USB serial
pio run -e d1_mini_lite_ota -t upload  # flash over WiFi (needs
                                       # WORDCLOCK_OTA_PASSWORD set)
pio device monitor                     # serial console (115200 baud)
```

The logic is covered by a host test that checks all specification examples,
span/grid consistency, a full 24-hour sweep, and the row-major reading
property for every minute of the day:

```sh
g++ -std=c++11 -Wall -Wextra -I src -o /tmp/test_clock test/test_clock.cpp
/tmp/test_clock
```

Expected output: `3043 checks, 0 failures`.

## Configuration

WiFi credentials live in [`include/secrets.h`](include/secrets.h). This file
is deliberately **not tracked by git** (see `.gitignore`) — when cloning,
create it yourself:

```cpp
#pragma once
const char *WIFI_SSID = "your-network";
const char *WIFI_PASSWORD = "your-password";
const char *OTA_PASSWORD = "your-ota-password";  // for ./flash.sh ota
```

NTP servers and the time zone (CET/CEST with automatic DST) are constants at
the top of [`src/main.cpp`](src/main.cpp).
