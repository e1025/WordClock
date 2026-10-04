#!/usr/bin/env bash
# Compile and flash the word clock firmware.
#
# Usage:
#   ./flash.sh          build + flash over USB serial (default)
#   ./flash.sh serial   same as above
#   ./flash.sh ota      build + flash over WiFi (ArduinoOTA), no cable needed
#
# OTA needs the device to be powered, connected to WiFi and reachable as
# WordClock.lan (the DHCP hostname the router resolves). The OTA password
# is read from include/secrets.h — the same file the firmware itself uses,
# so there is a single source of truth.

set -euo pipefail
cd "$(dirname "$0")"

MODE="${1:-serial}"
case "$MODE" in
  serial | ota) ;;
  *) echo "usage: $0 [serial|ota]" >&2; exit 1 ;;
esac

PIO="${PIO:-pio}"
if ! command -v "$PIO" >/dev/null 2>&1; then
  PIO="$HOME/.platformio/penv/bin/pio"
fi

if [ "$MODE" = "ota" ]; then
  OTA_PW="$(grep -oP 'OTA_PASSWORD\s*=\s*"\K[^"]*' include/secrets.h)"
  if [ -z "$OTA_PW" ]; then
    echo "error: OTA_PASSWORD not found in include/secrets.h" >&2
    exit 1
  fi
  export WORDCLOCK_OTA_PASSWORD="$OTA_PW"
  echo "== build + flash over WiFi (WordClock.lan) =="
  "$PIO" run -e d1_mini_lite_ota -t upload
else
  echo "== build + flash over USB serial =="
  "$PIO" run -e d1_mini_lite -t upload
fi
