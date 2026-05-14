# ============================================================
# Zephyr RTOS Development Environment
# Base: Ubuntu 24.04 LTS
# Zephyr SDK: 1.0.1
# ============================================================

FROM ubuntu:24.04

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# ── System dependencies (per Zephyr Getting Started Guide) ──
RUN apt-get update && apt-get install -y --no-install-recommends \
    git \
    cmake \
    ninja-build \
    gperf \
    ccache \
    dfu-util \
    device-tree-compiler \
    wget \
    xz-utils \
    file \
    make \
    gcc \
    gcc-multilib \
    g++-multilib \
    libsdl2-dev \
    python3-dev \
    python3-pip \
    python3-setuptools \
    python3-tk \
    python3-venv \
    python3-wheel \
    # Git / SSH tooling useful for GitHub workflows
    openssh-client \
    ca-certificates \
    curl \
    && rm -rf /var/lib/apt/lists/*

# ── Create a non-root user for development ──
ARG USERNAME=zephyr
ARG USER_UID=1001
ARG USER_GID=1001

RUN groupadd --gid ${USER_GID} ${USERNAME} \
    && useradd --uid ${USER_UID} --gid ${USER_GID} -m ${USERNAME} \
    && apt-get update && apt-get install -y sudo \
    && echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers \
    && rm -rf /var/lib/apt/lists/*

USER ${USERNAME}
WORKDIR /home/${USERNAME}

# ── Python virtual environment + west ──
ENV VIRTUAL_ENV=/home/${USERNAME}/.venv
RUN python3 -m venv ${VIRTUAL_ENV}
ENV PATH="${VIRTUAL_ENV}/bin:${PATH}"

RUN pip install --upgrade pip wheel \
    && pip install west

# ── Zephyr workspace ──
# Initialise workspace from upstream Zephyr (tag v4.4.0).
# Change the --mr value to pin a different release.
ARG ZEPHYR_VERSION=v4.4.0
ENV ZEPHYR_BASE=/home/${USERNAME}/zephyrproject/zephyr

RUN west init -m https://github.com/zephyrproject-rtos/zephyr \
              --mr ${ZEPHYR_VERSION} /home/${USERNAME}/zephyrproject \
    && cd /home/${USERNAME}/zephyrproject \
    && west update \
    && west zephyr-export \
    && pip install -r ${ZEPHYR_BASE}/scripts/requirements.txt

# ── Zephyr SDK 1.0.1 ──
# Downloads the minimal bundle; the setup script then pulls only the
# toolchains you need.  Edit ZEPHYR_SDK_TOOLCHAINS below to add more
# (e.g. "arm xtensa riscv0" etc.).
ARG ZEPHYR_SDK_VERSION=1.0.1
ARG ZEPHYR_SDK_TOOLCHAINS="-t arm-zephyr-eabi"

ENV ZEPHYR_SDK_INSTALL_DIR=/home/${USERNAME}/zephyr-sdk-${ZEPHYR_SDK_VERSION}

RUN wget -q "https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${ZEPHYR_SDK_VERSION}/zephyr-sdk-${ZEPHYR_SDK_VERSION}_linux-x86_64_gnu.tar.xz" \
    && tar -xf "zephyr-sdk-${ZEPHYR_SDK_VERSION}_linux-x86_64_gnu.tar.xz" \
    && rm  "zephyr-sdk-${ZEPHYR_SDK_VERSION}_linux-x86_64_gnu.tar.xz" \
    && cd ${ZEPHYR_SDK_INSTALL_DIR} \
    && ./setup.sh -c ${ZEPHYR_SDK_TOOLCHAINS}

# ── Environment ──
ENV ZEPHYR_TOOLCHAIN_VARIANT=zephyr
ENV PATH="${VIRTUAL_ENV}/bin:${PATH}"

# Activate venv automatically for every shell session
RUN echo "source ${VIRTUAL_ENV}/bin/activate" >> /home/${USERNAME}/.bashrc \
    && echo "source ${ZEPHYR_BASE}/zephyr-env.sh 2>/dev/null || true" >> /home/${USERNAME}/.bashrc

# ── Working directory for your application code ──
# Mount your project here: docker run -v $(pwd):/workspace ...
WORKDIR /workspace

CMD ["/bin/bash"]