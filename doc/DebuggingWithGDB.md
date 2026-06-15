# Pure GDB Debugging

## ESP32 JTAG Debug Setup with Zephyr — What I Did

### Hardware

- **Board:** ESP32 NodeMCU (WROOM-32)
- **Debug probe:** ESP-PROG (FT2232H based, Espressif's official JTAG probe)
- **JTAG wiring:** GPIO12(TDI), GPIO13(TCK), GPIO14(TMS), GPIO15(TDO)
- **Key lesson:** GPIO12-15 conflict with SPI flash on WROOM-32 — the app must not use these pins for anything else

---

### Software Stack

- **OpenOCD:** Espressif's fork `openocd-esp32 v0.12.0-esp32-20251215` (baked into Dockerfile)
- **GDB:** `xtensa-espressif_esp32_zephyr-elf-gdb` from Zephyr SDK
- **VS Code:** `cppdbg` type with `MIMode: gdb` connecting to OpenOCD's GDB server on port 3333

---

### Brief into to ESP32 Firmware Image Format

The firmware consists of a `header` and `extended header` a `variable number of data segments` and a `footer`, more over - Multy-byte fields are little endian.

#### The Firmware Header

The image header is 8 bytes long:

| Byte(s) | Description                                                                                                                                                |
| ------: | ---------------------------------------------------------------------------------------------------------------------------------------------------------- |
|       0 | Magic number (always 0xE9)                                                                                                                                 |
|       1 | Number of segments                                                                                                                                         |
|       2 | SPI Flash Mode (0 = QIO, 1 = QOUT, 2 = DIO, 3 = DOUT)                                                                                                      |
|       3 | High four bits — Flash size (0 = 1MB, 1 = 2MB, 2 = 4MB, 3 = 8MB, 4 = 16MB); Low four bits — Flash frequency (0 = 40MHz, 1 = 26MHz, 2 = 20MHz, 0xF = 80MHz) |
|     4–7 | Entry point address                                                                                                                                        |

These bytes are only overridden if this is a bootloader image (an image written to a correct bootloader offset of 0x1000). In this case, the appended SHA256 digest, which is a cryptographic hash used to verify the integrity of the image, is also updated to reflect the header changes.

---

### Things I did to make it work

1. Solving `ESP-PROG not detected`

    - Missing udev rules → added `/etc/udev/rules.d/99-embedded-dev.rules`
    - USB permissions inside container → `sudo chmod 666 /dev/bus/usb/*/*`
    - Setting `privileged: true` in the compose.yaml, this gives access to all host devices

2. JTAG scan chain failed (`all ones`)

    - First NodeMCU had a hardware fault on JTAG pins
    - Switching to a second NodeMCU fixed it
    - `ESP32_FLASH_VOLTAGE 3.3` added to [OpenOCD](../.vscode/esp32-esprog.cfg) config to stabilize GPIO12 during reset

3. Target wouldn't halt (`timed out`)

    - Firmware was in a WDT crash loop because flash was empty
    - Fixed by flashing via onboard USB (JTAG wires must be disconnected during flash)
    - Added `reset halt` + `sleep 500` to OpenOCD launch sequence

4. Breakpoints hollow / not resolving

    - `appimage_offset` was wrong — OpenOCD couldn't find the app in flash
    - Scanned flash with `esptool read-flash` looking for `0xE9` magic byte
    - Found app at `0x1000` — set `esp appimage_offset 0x1000` in OpenOCD launch.
    Solved that using:
    
    ```bash
    # Note: this is image-info of an app buildt with --sysbuild, thus containg MCUBOOT
    $ esptool image-info ./build/mcuboot/zephyr/zephyr.bin 
    esptool v5.3.0
    Image size: 38432 bytes
    Detected image type: ESP32

    ESP32 Image Header
    ==================
    Image version: 1
    Entry point: 0x400a19f0
    Segments: 3
    Flash size: 4MB
    Flash freq: 40m
    Flash mode: DIO

    ESP32 Extended Image Header
    ===========================
    WP pin: 0xee (disabled)
    Flash pins drive settings: clk_drv: 0x0, q_drv: 0x0, d_drv: 0x0, cs0_drv: 0x0, hd_drv: 0x0, wp_drv: 0x0
    Chip ID: 0 (ESP32)
    Minimal chip revision: v0.0, (legacy min_rev = 0)
    Maximal chip revision: v655.35

    Segments Information
    ====================
    Segment   Length   Load addr   File offs  Memory types
    -------  -------  ----------  ----------  ------------
          0  0x01a74  0x3ffe8000  0x00000018  BYTE_ACCESSIBLE, DRAM, DIRAM_DRAM
          1  0x00b2c  0x40078000  0x00001a94  CACHE_APP
          2  0x07028  0x400a0000  0x000025c8  DIRAM_IRAM

    ESP32 Image Footer
    ==================
    Checksum: 0x07 (valid)
    Validation hash: 7ddac7bf91abb6c3cfeb44f88707af98a24c9c568736879f64856b57a8513503 (valid)
    ```


---

### Final Working OpenOCD Command

```bash
/home/zephyr/openocd-esp32/bin/openocd \
  -s /home/zephyr/openocd-esp32/share/openocd/scripts \
  -f .vscode/esp32-esprog.cfg \
  -c "init" \
  -c "reset halt" \
  -c "esp appimage_offset 0x1000"
```

### Final `.vscode/esp32-esprog.cfg`

```tcl
set ESP_RTOS none
set ESP_ONLYCPU 1
set ESP32_FLASH_VOLTAGE 3.3
source [find interface/ftdi/esp_ftdi.cfg]
adapter speed 4000
ftdi tdo_sample_edge falling
source [find target/esp32.cfg]
```

### Final `launch.json`

```json
{
    "name": "GDB ESP32",
    "type": "cppdbg",
    "request": "launch",
    "MIMode": "gdb",
    "miDebuggerPath": "/home/zephyr/zephyr-sdk-1.0.1/gnu/xtensa-espressif_esp32_zephyr-elf/bin/xtensa-espressif_esp32_zephyr-elf-gdb",
    "miDebuggerServerAddress": "localhost:3333",
    "program": "/home/zephyr/workspace/app/build/zephyr/zephyr.elf",
    "cwd": "${workspaceFolder}",
    "setupCommands": [
        { "text": "set remotetimeout 20" },
        { "text": "-enable-pretty-printing", "ignoreFailures": true }
    ],
    "initCommands": [
        { "text": "set remote hardware-watchpoint-limit 2" },
        { "text": "mon reset halt" },
        { "text": "maintenance flush register-cache" },
        { "text": "mon esp appimage_offset 0x1000" }
    ],
    "externalConsole": false
}
```

---

### Workflow

0. Build: `west build -p always -b esp32_devkitc/esp32/procpu /home/zephyr/workspace/app/app -- -DEXTRA_CONF_FILE=debug.conf`
1. Flash via onboard USB (JTAG disconnected): `west flash --esp-device /dev/ttyUSB0`
2. Reconnect JTAG wires
3. Start OpenOCD in a separate terminal:

    ```bash
       /home/zephyr/openocd-esp32/bin/openocd \
         -s /home/zephyr/openocd-esp32/share/openocd/scripts \
         -f /home/zephyr/workspace/app/.vscode/esp32-esprog.cfg \
         -c "init" \
         -c "reset halt" \
         -c "esp appimage_offset 0x1000"
    ```

4. Hit F5 in VS Code → full source-level debugging

## MCUboot and App Image Offset

### What is MCUboot

MCUboot is a secure bootloader for embedded systems. In a Zephyr project that uses it, the boot sequence is:

```text
ROM bootloader → MCUboot → my Zephyr app
```

MCUboot sits between the ESP32 ROM bootloader and my app. It provides:

- **OTA update support** — can swap between two app slots (slot0/slot1)
- **Image verification** — checks app signature/hash before booting
- **Rollback protection** — falls back to previous working app if new one fails

---

### Flash Layout — Without MCUboot (my current setup)

```text
0x0000  ROM bootloader (built into chip)
0x1000  ESP32 image header + my Zephyr app  ← appimage_offset 0x1000
0x8000  Partition table
```

West flashes directly to `0x1000`. The ESP32 ROM bootloader reads the image header at `0x1000` and jumps straight into my app. Simple and fast.

---

### Flash Layout — With MCUboot

First we need to tell Zephyr to build our image to contain both our app and MCUboot - simply add `--sysbuild` to the west build command.

```bash
west build --sysbuild -p always -b esp32_devkitc/esp32/procpu /home/zephyr/workspace/app/app -- -DEXTRA_CONF_FILE=debug.conf
```

Later on, MCUboot requires a signed image to properly to accept it and load it to memory.

So we create a pen key:
```bash
../bootloader/mcuboot/scripts/imgtool.py keygen -k mcu_root_key.pem -t ecdsa-p256
```

Then we need to modify our `prj.conf` to contain both the  `pem` key, and an BOOTLOADER_MCUBOOT config:

```text
# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
#
# This file contains selected Kconfig options for the application.

CONFIG_SENSOR=y
CONFIG_2CH_REMOTE_CONTROLL=y
CONFIG_BOOTLOADER_MCUBOOT=y
CONFIG_MCUBOOT_SIGNATURE_KEY_FILE="mcu_root_key.pem"
```

I got stuck here.
Flashing an image with MCUboot to the ESP is not straight forward, the image starts, selects the propper image.0 but then get stuck:

```bash
rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:2
load:0x3ffe8000,len:7296
load:0x40078000,len:2860
load:0x400a0000,len:29044
entry 0x400a1a38
I (soc_init): MCUboot 2nd stage bootloader
I (soc_init): compile time Jun 15 2026 21:58:16
W (soc_init): Unicore bootloader
I (soc_init): chip revision: v3.0
I (flash_init): SPI Speed      : 40MHz
I (flash_init): SPI Mode       : DIO
I (flash_init): SPI Flash Size : 4MB
I (boot): Loading image 0 - slot 0 from flash, area id: 2
I (boot): Application start=40089830h
I (boot): DRAM  : lma=00030254h vma=3ffb0000h size=016e4h (  5860) load
I (boot): IRAM  : lma=000200a8h vma=40080000h size=101ach ( 65964) load
```

For some reason - the `debug.conf` broke MCUBOOT - Ill investigate it later...

```text
0x0000  ROM bootloader (built into chip)
0x1000  MCUboot bootloader
0x8000  Partition table
0x10000 MCUboot scratch/status area
0x20000 slot0_partition — my Zephyr app  ← appimage_offset 0x20000
0x170000 slot1_partition — OTA update slot
```

With MCUboot, my app lives in `slot0` starting at `0x20000`. The image at `0x20000` starts with a **MCUboot image header** (`0x7c 0x84...`) not the ESP32 magic byte (`0xE9`). OpenOCD needs to know to skip the MCUboot header to find the real app — hence a different `appimage_offset`.

---

### Why This Matters for OpenOCD

OpenOCD reads the app image header to build flash memory maps, which it uses to set **software breakpoints** (by temporarily patching flash). If `appimage_offset` is wrong:

- OpenOCD can't find the image header → `Application image is invalid` warning
- Flash maps fail → only 2 hardware breakpoints available (ESP32 hardware limit)
- Software breakpoints don't work → breakpoints appear as hollow circles in VS Code

| Setup           | `appimage_offset`       | Magic byte at offset        |
| --------------- | ----------------------- | --------------------------- |
| Without MCUboot | `0x1000`                | `0xE9` ✅                    |
| With MCUboot    | `0x20000` + header size | `0xE9` after MCUboot header |

To find the correct offset when unsure:

```bash
esptool --port /dev/ttyUSB0 read-flash <suspected_offset> 4 /tmp/check.bin
od -A x -t x1z /tmp/check.bin
# In case of app image only. Look for first byte = 0xE9
```
