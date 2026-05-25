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

To debug you need a JTAG.

Good news — J-Link supports ESP32 via JTAG. Here's the full setup:

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

**One caveat:** Cortex-Debug is designed for ARM. ESP32 is Xtensa, so you may need the **ESP-IDF extension** or **OpenOCD** instead of J-Link's GDB server for full support. Want me to set up the OpenOCD path too?

---

## Troubleshooting

| Error | Cause | Fix |
|---|---|---|
| `ESP_IDF_PATH is not set` | `hal_espressif` blobs not fetched | `west blobs fetch hal_espressif` |
| `Unknown module(s): {'hal_espressif'}` | west still using upstream manifest | `west config manifest.path app && west update` |
| `hal_espressif` not fetched by `west update` | missing from `name-allowlist` in `west.yml` | Add `- hal_espressif` to the allowlist |
| Board name warning about `esp32_devkitc_wroom` | Renamed in Zephyr 4.x | Use `esp32_devkitc/esp32/procpu` |
| Toolchain not found for Xtensa | SDK built with ARM only | Add `-t xtensa-espressif_esp32_zephyr-elf` to `ZEPHYR_SDK_TOOLCHAINS` and rebuild image |