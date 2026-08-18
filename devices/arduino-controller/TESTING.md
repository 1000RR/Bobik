# Controller verification

## Supported build target

The reproducible firmware build targets a classic Arduino Nano with an
ATmega328P running at 16 MHz (`nanoatmega328`). This definition uses the classic
Nano bootloader. If the physical board has the newer bootloader, use PlatformIO's
`nanoatmega328new` board definition before uploading; firmware compilation and
MCU memory limits are otherwise equivalent.

The build inputs are pinned:

- PlatformIO Core 6.1.19 (`requirements-build.txt`)
- PlatformIO Atmel AVR platform 5.3.0
- autowp-mcp2515 1.3.1
- ssd1306 1.8.8

The firmware target applies `-Wall -Wextra -Werror` to project sources. Host
tests are excluded from the embedded source tree by `build_src_filter`.

## Automated checks

Run the host regression suite with any C++11 compiler:

```sh
make -C controller/tests/host test
make -C controller/tests/host sanitize
```

The suite covers strict serial parsing, malformed/truncated/oversized input,
serial output backpressure, base-station command validation, distinct device
IDs, display rendering, CAN frame validation, receive bursts, transmit queue
saturation, busy retries, controller overflow/bus-off recovery, button bounce,
fresh transport initialization, sketch orchestration, and unsigned timer
rollover. The sanitizer target uses AddressSanitizer and UndefinedBehaviorSanitizer.

Build the real firmware without uploading it:

```sh
python3 -m venv .venv-platformio
.venv-platformio/bin/python -m pip install -r controller/requirements-build.txt
.venv-platformio/bin/platformio run --environment controller_nano
```

Verified on 2026-08-03 with the pinned inputs above:

| Resource | Used | Available | Utilization |
| --- | ---: | ---: | ---: |
| Flash | 13,928 bytes | 30,720 bytes | 45.3% |
| Static SRAM | 1,088 bytes | 2,048 bytes | 53.1% |

Treat an unexplained increase in static SRAM as release-blocking on this MCU.
The 960-byte remainder also has to absorb stack usage and library runtime state.

## Hardware acceptance gate

The following checks cannot be established by host mocks or a compiler. Run
them on the intended board, MCP2515 clock variant, OLED, serial host, and
terminated CAN network before rollout:

- Confirm whether the Nano uses the classic or newer bootloader and confirm the
  MCP2515 oscillator before uploading the `controller_nano` build.
- Cold-boot the controller repeatedly with the host and CAN nodes starting in
  different orders. The UI must leave the splash screen after three seconds,
  and valid traffic must resume without a reset.
- Inject CAN frames with DLC 0, 1, and 2, extended/RTR/error flags, and invalid
  standard IDs. They must be rejected while subsequent valid frames continue
  to flow.
- Burst traffic above three CAN frames per loop while serial output is both
  flowing and blocked. The controller must remain responsive and drain queued
  work once backpressure clears.
- Saturate all MCP2515 transmit buffers and the eight-entry software queue.
  Verify rate-limited `CAN_TX_BUSY`, `CAN_TX_FAILED`, or `CAN_TX_QUEUE_FULL`
  diagnostics and confirm that no command is duplicated.
- Force receive overflow and bus-off, then unplug/reconnect the CAN interface.
  Verify diagnostics and successful recovery attempts at five-second intervals.
- Exercise noisy/held button presses while serial and CAN are busy. Each
  deliberate press must produce exactly one arm-toggle request.
- Alarm device IDs `0x1` and `0x10` independently, clear them independently,
  and confirm both the OLED rows and host messages remain unambiguous.
- Run arm, alarm, clear, stop, and disarm flows across host/controller/node
  reboots. No stale partial serial line or stale alarm list may survive a
  controller reboot.

Record the board revision, MCP2515 oscillator, firmware commit, pass/fail result,
and any observed diagnostic counters with the deployment record.
