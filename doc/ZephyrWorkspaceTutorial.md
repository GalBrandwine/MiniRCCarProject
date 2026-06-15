# Zephyr Workspace Application with Docker — Setup Tutorial

A complete guide to setting up a reproducible Zephyr RTOS development environment
using Docker, structured as a proper West T2 workspace application.

---

## Concepts First

### The Three Application Types

Zephyr has three ways to structure an application. This tutorial uses the **workspace application** (T2 topology) — the recommended approach for real projects.

| Type | Where your app lives |
|---|---|
| Repository app | Inside the `zephyr/` repo itself |
| **Workspace app (T2)** | **Alongside `zephyr/` in the west workspace** |
| Freestanding app | Completely outside the workspace |

### The T2 Workspace Layout

```
workspace/                        ← west workspace root (lives inside the container)
├── .west/
│   └── config
├── zephyr/                       ← Zephyr kernel (fetched by west)
├── bootloader/
├── modules/
│   └── hal/
│       └── espressif/            ← target-specific HAL (fetched by west)
└── app/                          ← YOUR repository (bind-mounted from host)
    ├── .git/                     ← git-tracked on the host
    ├── west.yml                  ← manifest: declares what west fetches
    ├── app/                      ← the actual Zephyr application
    ├── boards/                   ← custom board definitions
    ├── drivers/                  ← out-of-tree drivers
    ├── Dockerfile
    └── compose.yaml
```

The key insight: **the container holds the build environment, your host holds the source.**
Zephyr, modules, and the SDK are baked into the image. Your code is bind-mounted at runtime.

---

## Step 1 — Start from the Example Application

Don't start from scratch. The official `example-application` repo is a battle-tested
skeleton for a T2 workspace application.

```bash
# On your host machine
git clone https://github.com/zephyrproject-rtos/example-application my-app
cd my-app

# Re-initialize as your own repo
rm -rf .git
git init
git add .
git commit -m "Initial commit from example-application skeleton"
```

Your repo now has the correct structure: `app/`, `boards/`, `drivers/`, `west.yml`, `Kconfig`, etc.

---

## Step 2 — The Dockerfile

The Dockerfile builds an image containing everything **except** your application code:
- Ubuntu 24.04 base
- All Zephyr system dependencies
- Python venv + `west`
- Zephyr SDK (with the toolchains you need)
- A pre-bootstrapped west workspace (Zephyr kernel + modules)

### Key decisions

<!-- **Toolchains** — declare every architecture you target. For ESP32 (Xtensa LX6) + ARM:
```dockerfile
ARG ZEPHYR_SDK_TOOLCHAINS="-t arm-zephyr-eabi -t xtensa-espressif_esp32_zephyr-elf"
``` -->

**Workspace bootstrap** — the image bootstraps using the upstream Zephyr manifest
so all base modules are pre-fetched and cached in the image layer:
```dockerfile
RUN west init --mr ${ZEPHYR_VERSION} /home/zephyr/workspace \
    && cd /home/zephyr/workspace \
    && west update \
    && west zephyr-export \
    && pip install -r ${ZEPHYR_BASE}/scripts/requirements.txt
```

**Working directory** — set to the workspace root, not your app:
```dockerfile
WORKDIR /home/zephyr/workspace
```

---

## Step 3 — compose.yaml

Mount your repo as `app/` inside the workspace, and set the working directory
to the workspace root so `west` commands work without any `cd`:

```yaml
services:
  rccarproject:
    image: rccarproject
    build:
      context: .
      dockerfile: ./Dockerfile
    stdin_open: true
    tty: true
    volumes:
      - .:/home/zephyr/workspace/app   # your repo → app/ in the workspace
    working_dir: /home/zephyr/workspace
```

For flashing over USB, add device passthrough:
```yaml
    # devices:
    #   - /dev/ttyUSB0:/dev/ttyUSB0      # adjust to ttyACM0 if needed
    privileged: true # gives access to all host devices
```

---

## Step 4 — devcontainer.json (VS Code)

```json
{
    "name": "Zephyr RTOS Dev Environment",
    "dockerComposeFile": "../compose.yaml",
    "service": "rccarproject",
    "workspaceFolder": "/home/zephyr/workspace/app",
    "remoteUser": "zephyr",
    "customizations": {
        "vscode": {
            "extensions": [
                "ms-vscode.cpptools",
                "ms-vscode.cmake-tools",
                "twxs.cmake",
                "ms-vscode.vscode-serial-monitor",
                "marus25.cortex-debug",
                "trond-snekvik.gnu-mapfiles",
                "nordic-semiconductor.nrf-kconfig"
            ],
            "settings": {
                "cmake.configureOnOpen": false,
                "C_Cpp.default.compilerPath": "/home/zephyr/zephyr-sdk-1.0.1/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc",
                "C_Cpp.default.includePath": [
                    "${workspaceFolder}/**",
                    "/home/zephyr/workspace/zephyr/include/**"
                ],
                "terminal.integrated.defaultProfile.linux": "zsh"
            }
        }
    },
    "postStartCommand": "source /home/zephyr/.venv/bin/activate && source /home/zephyr/workspace/zephyr/zephyr-env.sh"
}
```

---

## Step 5 — west.yml (your manifest)

The `example-application` skeleton uses a `name-allowlist` to fetch only the modules
it needs. You must add any target-specific HAL to this list.

For ESP32, add `hal_espressif`. Also pin `zephyr` to the same version used in
your Docker image to avoid drift:

```yaml
manifest:
  self:
    west-commands: scripts/west-commands.yml
  remotes:
    - name: zephyrproject-rtos
      url-base: https://github.com/zephyrproject-rtos
  projects:
    - name: zephyr
      remote: zephyrproject-rtos
      revision: v4.4.0          # pin to match your Docker image
      import:
        name-allowlist:
          - cmsis_6              # ARM Cortex-M
          - hal_nordic           # Nordic boards
          - hal_stm32            # STM32 boards
          - hal_espressif        # ESP32 ← add this
```

> **Why the allowlist matters:** without it, `west update` fetches every HAL
> for every supported chip (~dozens of repos, gigabytes of data). The allowlist
> keeps the workspace lean and fast.

---

## Step 6 — First-Time Setup Inside the Container

Run once after starting the container for the first time (or after a fresh image build):

```bash
# 1. Tell west to use YOUR west.yml as the manifest
west config manifest.path app

# 2. Fetch the modules declared in your west.yml
west update

# 3. Fetch ESP32 binary blobs (bootloader, WiFi/BT firmware)
#    Required for ESP32 — west build will fail without this
west blobs fetch hal_espressif

west packages pip --install
```

---

## Step 7 — Build

```bash
# From /home/zephyr/workspace (the working_dir set in compose.yaml)
west build -b esp32_devkitc/esp32/procpu app
```

Note: Zephyr 4.4.x renamed `esp32_devkitc_wroom` → `esp32_devkitc`. Use the new name.
The `/esp32/procpu` qualifier selects the main CPU (required since Zephyr 3.5+).

---

## Step 8 — Flash

```bash
west flash --esp-device /dev/ttyUSB0
```

---

## Day-to-Day Workflow

```bash
# Start the container
docker compose run --rm rccarproject

# Edit code on your host — changes are immediately visible inside the container

# Build
west build -b esp32_devkitc/esp32/procpu app

# Rebuild after changes (west is incremental)
west build

# Clean build
west build -t pristine
west build -b esp32_devkitc/esp32/procpu app

# Flash
west flash --esp-device /dev/ttyUSB0
```

## Debugging within the container

### SEGGER JLINK's Configuringn.

I have J-Link at home. But it supports only ESP32-3C via JTAG. 
Ill keep the full setup here if needed next time:

**1. Wire J-Link to ESP32 DevKitC**

| J-Link Pin | ESP32 GPIO | Function |
|---|---|---|
| TDI | GPIO12 | JTAG TDI |
| TDO | GPIO15 | JTAG TDO |
| TCK | GPIO13 | JTAG TCK |
| TMS | GPIO14 | JTAG TMS |
| GND | GND | Ground |
| VTref | 3.3V | Reference voltage |

**2. Install J-Link software in the container:**
```bash
# Check if JLinkGDBServer is already available
which JLinkGDBServer
```

If not:
```bash
wget -q https://www.segger.com/downloads/jlink/JLink_Linux_x86_64.tgz
tar -xf JLink_Linux_x86_64.tgz
sudo cp -r JLink_Linux_x86_64/* /usr/local/
```
**One caveat:** Cortex-Debug is designed for ARM. ESP32 is Xtensa, so you may need the **ESP-IDF extension** or **OpenOCD** instead of J-Link's GDB server for full support.


### ESP-PROG Configuration

![ESP32-prog](/home/zephyr/workspace/app/doc/ESP32-prog.png)

I've bought this [ESP32-programmer](https://he.aliexpress.com/item/4001296786022.html?spm=a2g0o.order_list.order_list_main.41.67b41802sdK85M&gatewayAdapt=glo2isr)

It Features:
 
1. ESP-Prog is a development and debugging tool with automatic download firmware, serial communication, JTAG online debugging and other functions. The automatic download firmware and serial communication functions are available for the ESP8266 and ESP32 platforms, and the JTAG in-circuit debugging function is available for the ESP32 platform.
2. ESP-Prog is easy to use and can be connected to a computer with only one USB cable. The computer can recognize the download function and the two ports corresponding to the JTAG function.
3. ESP-Prog can be connected to the user board using the cable. The connector is available in 2.54 mm and 1.27 mm pitch packages with a foolproof design. The user board is required to place the Program (6-Pin) and JTAG (10-Pin) connectors in the corresponding order.
4. Considering that the power supply voltage of different user boards may be different, the two interfaces of ESP-Prog can select 5V or 3.3V power supply through Pin Header, which has strong power compatibility.
 
Product description:
 
Size: 73.4mm * 25.1mm
Interface: Program; JTAG
 
Steps for usage:
 
1. Connect the ESP-Prog debug board and the USB port on the computer via a USB cable.
2. Install the FT2232HL chip driver on the computer side. The computer recognizes two ports, indicating that the driver has been successfully installed.
3. Use the Pin header to select the power supply output voltage on the Program/JTAG interface.
4. Connect the debug board and ESP product board with the gray cable.
5. Automatic download and JTAG debugging of the ESP32 product board can be achieved using official software tools or scripts.

Some good Good news — ESP-PROG is natively supported by OpenOCD with no extra setup needed. 
Here's what to do:

**Step 1 — Verify the container sees it:**
```bash
ls /dev/ttyUSB*
```
You should see two ports — `ttyUSB0` and `ttyUSB1` and `ttyUSB2`. ESP-PROG creates two:
- `ttyUSB0` — ESP32 (Maybe need to disconnect)
- `ttyUSB1` — JTAG
- `ttyUSB2` — UART (serial/flash)

**Step 2 — Update your OpenOCD config**  In `.vscode/esp32-esprog.cfg`:

```tcl
set ESP_RTOS none
set ESP_ONLYCPU 1
source [find interface/ftdi/esp32_devkitj_v1.cfg]
source [find target/esp32.cfg]
```
This is the config that was originally in the Zephyr board support — it was written exactly for ESP-PROG.

**Step 3 — Update `launch.json`:**
```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "ESP32 ESP-PROG Debug",
      "type": "cortex-debug",
      "request": "launch",
      "servertype": "openocd",
      "executable": "${workspaceFolder}/build/zephyr/zephyr.elf",
      "runToEntryPoint": "main",
      "toolchainPath": "/home/zephyr/zephyr-sdk-1.0.1/gnu/xtensa-espressif_esp32_zephyr-elf/bin",
      "toolchainPrefix": "xtensa-espressif_esp32_zephyr-elf",
      "gdbPath": "/home/zephyr/zephyr-sdk-1.0.1/gnu/xtensa-espressif_esp32_zephyr-elf/bin/xtensa-espressif_esp32_zephyr-elf-gdb",
      "openOCDPath": "/home/zephyr/openocd-esp32/bin/openocd",
      "configFiles": [
        "${workspaceFolder}/.vscode/esp32-esprog.cfg"
      ],
      "searchDir": [
        "/home/zephyr/openocd-esp32/share/openocd/scripts"
      ],
      "preLaunchTask": "west build debug"
    }
  ]
}
```

**Step 4 — Wire ESP-PROG to NodeMCU:**

| ESP-PROG JTAG | NodeMCU |
|---|---|
| VDD (3.3V) | 3.3V |
| GND | GND |
| TDI | GPIO12 |
| TDO | GPIO15 |
| TCK | GPIO13 |
| TMS | GPIO14 |
| EN | EN (optional) |

**Step 5 — Test OpenOCD manually first:**
```bash
/home/zephyr/openocd-esp32/bin/openocd \
  -s /home/zephyr/openocd-esp32/share/openocd/scripts \
  -f /home/zephyr/workspace/app/.vscode/esp32-esprog.cfg
```

You should see:
```
Info : esp32.cpu0: hardware has 2 breakpoints, 2 watchpoints
```

If no openocd, install manually:

```bash
cd /home/zephyr
wget https://github.com/espressif/openocd-esp32/releases/download/v0.12.0-esp32-20240318/openocd-esp32-linux-amd64-0.12.0-esp32-20240318.tar.gz
tar -xf openocd-esp32-linux-amd64-0.12.0-esp32-20240318.tar.gz
rm openocd-esp32-linux-amd64-0.12.0-esp32-20240318.tar.gz
echo 'export PATH=$PATH:/home/zephyr/openocd-esp32/bin' >> ~/.zshrc
source ~/.zshrc
```

Note: Illl add it to the Dockerfile.

That means it's connected. Then hit F5 in VS Code.
 
Package includes:
 
1 x ESP-Prog Development Board




**3. Add to `prj.conf` or `debug.conf`:**
```conf
CONFIG_DEBUG_OPTIMIZATIONS=y
CONFIG_LOG=y
CONFIG_APP_LOG_LEVEL_DBG=y
```

**4. Build with debug config:**
```bash
west build -p always -b esp32_devkitc/esp32/procpu /home/zephyr/workspace/app/app \
  -- -DEXTRA_CONF_FILE=debug.conf
```

**5. Create `.vscode/launch.json` in your project:**
```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "ESP32 J-Link Debug",
      "type": "cortex-debug",
      "request": "launch",
      "servertype": "jlink",
      "device": "ESP32",
      "interface": "jtag",
      "speed": 4000,
      "executable": "${workspaceFolder}/build/zephyr/zephyr.elf",
      "runToEntryPoint": "main",
      "jlinkscript": "",
      "svdFile": "",
      "preLaunchTask": "west build"
    }
  ]
}
```

**6. Add `.vscode/tasks.json` for the build task:**
```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "west build",
      "type": "shell",
      "command": "west build -p always -b esp32_devkitc/esp32/procpu /home/zephyr/workspace/app/app -- -DEXTRA_CONF_FILE=debug.conf",
      "group": {
        "kind": "build",
        "isDefault": true
      },
      "problemMatcher": ["$gcc"]
    }
  ]
}
```

Then in VS Code hit `F5` to build and start debugging.

---

## Troubleshooting

| Error | Cause | Fix |
|---|---|---|
| `ESP_IDF_PATH is not set` | `hal_espressif` blobs not fetched | `west blobs fetch hal_espressif` |
| `Unknown module(s): {'hal_espressif'}` | west still using upstream manifest | `west config manifest.path app && west update` |
| `hal_espressif` not fetched by `west update` | missing from `name-allowlist` in `west.yml` | Add `- hal_espressif` to the allowlist |
| Board name warning about `esp32_devkitc_wroom` | Renamed in Zephyr 4.x | Use `esp32_devkitc/esp32/procpu` |
| Toolchain not found for Xtensa | SDK built with ARM only | Add `-t xtensa-espressif_esp32_zephyr-elf` to `ZEPHYR_SDK_TOOLCHAINS` and rebuild image |