# Basic Usage

```bash
# Build the image (takes a while — SDK download is large)
DOCKER_BUILDKIT=0 docker build -t my-zephyr-dev .  

# Run with your project mounted
docker run -it --rm -v $(pwd):/workspace my-zephyr-dev

# Inside the container, build a sample
cd /home/zephyr/zephyrproject/zephyr
west build -p always -b qemu_cortex_m3 samples/basic/blinky
```
