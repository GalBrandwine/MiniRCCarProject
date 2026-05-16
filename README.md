# MiniRCCarProject

## Basic Usage

```bash
# Build the image (takes a while — SDK download is large)
DOCKER_BUILDKIT=0 docker build -t my-zephyr-dev .  

# Run with your project mounted
docker run -it --rm -v $(pwd):/workspace my-zephyr-dev

# Inside the container, build a sample
cd /home/zephyr/zephyrproject/zephyr
west build -p always -b qemu_cortex_m3 samples/basic/blinky
```

## Starting from Zephyr Example Project

### Getting started from scratch

The [zephyr example application](https://github.com/zephyrproject-rtos/example-application) is the official template for out-of-tree Zephyr apps.  
It gives the correct `west.yml`, CMake structure, and CI setup from the start.

**On your host machine:**

```bash
cd ~/dev
# Clone the example app as your project (don't clone into RCCarProject, let it create the folder)
git clone https://github.com/zephyrproject-rtos/example-application RCCarProject
cd RCCarProject

# Point it at your own GitHub repo
git remote set-url origin https://github.com/<your-username>/RCCarProject.git
git push -u origin main
```

**Build the Docker Image (installing Zephyr, and other stuff):**

```bash
# Either manually build
DOCKER_BUILDKIT=0 docker build --no-cache -t rccarproject .

# OR use the docker-compose.yaml from scratch:
docker compose -f compose.debug.yaml build --no-cache
docker compose -f compose.debug.yaml up -d

# When developing use:
docker compose -f 'compose.debug.yaml' up -d --build 'rccarproject' 
```

**Then initialize the west workspace around it:**

Inside the container:

```bash
cd /home/zephyr
west init -l /workspace        # -l means "local manifest", uses the west.yml already in your repo
west update                    # pulls zephyr + modules declared in west.yml
west zephyr-export
```

The `-l` flag is the key difference from what the Dockerfile does — instead of pulling Zephyr first and your app second, it treats your repo as the manifest and pulls Zephyr as a dependency. That's the proper out-of-tree app model.

**What you'll get:**

```text
RCCarProject/        ← your git repo / west manifest
├── app/             ← your application source
├── boards/          ← custom board definitions
├── drivers/         ← custom drivers
├── dts/             ← custom devicetree bindings
├── west.yml         ← declares zephyr version dependency
└── CMakeLists.txt
```
