# ============================================================
# Zephyr RTOS Development Environment — Workspace Application
# Base:        Ubuntu 24.04 LTS
# Zephyr:      v4.4.0
# Zephyr SDK:  1.0.1
#
# West T2 topology:
#
#   /home/zephyr/workspace/          ← west workspace root
#   ├── .west/config
#   ├── zephyr/                      ← Zephyr kernel (fetched at build time)
#   ├── bootloader/
#   ├── modules/
#   └── app/                         ← YOUR application (bind-mount from host)
# ============================================================

FROM ubuntu:24.04

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# ── Locale (required for zsh + Powerline/unicode characters) ──
ENV LANG=en_US.UTF-8
ENV LANGUAGE=en_US:en
ENV LC_ALL=en_US.UTF-8

# ── System dependencies ──
RUN apt-get update && apt-get install -y --no-install-recommends \
    locales \
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
    sudo \
    zsh \
    fonts-powerline \
    vim \
    && locale-gen en_US.UTF-8 \
    && rm -rf /var/lib/apt/lists/*

# ── Create a non-root user for development ──
ARG USERNAME=zephyr
ARG USER_UID=1000
ARG USER_GID=1000

# Ubuntu 24.04 base image ships a built-in 'ubuntu' user at UID/GID 1000.
# Remove it first so we can claim UID/GID 1000 for our own user.
RUN userdel -r ubuntu 2>/dev/null || true \
    && groupdel ubuntu 2>/dev/null || true \
    && groupadd --gid ${USER_GID} ${USERNAME} \
    && useradd --uid ${USER_UID} --gid ${USER_GID} -m ${USERNAME} -s /bin/zsh \
    && echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

USER ${USERNAME}
WORKDIR /home/${USERNAME}

# ── Oh My Zsh + plugins ──
RUN sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)" "" --unattended \
    && git clone --depth=1 https://github.com/zsh-users/zsh-autosuggestions \
        ~/.oh-my-zsh/custom/plugins/zsh-autosuggestions \
    && git clone --depth=1 https://github.com/zsh-users/zsh-syntax-highlighting \
        ~/.oh-my-zsh/custom/plugins/zsh-syntax-highlighting \
    && sed -i 's/ZSH_THEME="robbyrussell"/ZSH_THEME="agnoster"/' ~/.zshrc \
    && sed -i 's/plugins=(git)/plugins=(git zsh-autosuggestions zsh-syntax-highlighting)/' ~/.zshrc

# ── Python virtual environment + west ──
ENV VIRTUAL_ENV=/home/${USERNAME}/.venv
RUN python3 -m venv ${VIRTUAL_ENV}
ENV PATH="${VIRTUAL_ENV}/bin:${PATH}"

RUN pip install --upgrade pip wheel \
    && pip install west

# ── Zephyr SDK ───────────────────────────────────────────────
ARG ZEPHYR_SDK_VERSION=1.0.1
ARG ZEPHYR_SDK_TOOLCHAINS="-t arm-zephyr-eabi"

ENV ZEPHYR_SDK_INSTALL_DIR=/home/${USERNAME}/zephyr-sdk-${ZEPHYR_SDK_VERSION}

RUN wget -q \
    "https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${ZEPHYR_SDK_VERSION}/zephyr-sdk-${ZEPHYR_SDK_VERSION}_linux-x86_64_gnu.tar.xz" \
    && tar -xf "zephyr-sdk-${ZEPHYR_SDK_VERSION}_linux-x86_64_gnu.tar.xz" \
    && rm   "zephyr-sdk-${ZEPHYR_SDK_VERSION}_linux-x86_64_gnu.tar.xz" \
    && cd ${ZEPHYR_SDK_INSTALL_DIR} \
    && ./setup.sh -c ${ZEPHYR_SDK_TOOLCHAINS}

# ── West workspace ───────────────────────────────────────────
# Bootstrap with upstream Zephyr manifest — no app mounted yet at build time.
# ZEPHYR_EXTRA_MODULES is intentionally NOT set here; it is set at runtime
# via .zshrc so it only takes effect when the app bind-mount is present.
ARG ZEPHYR_VERSION=v4.4.0
ENV ZEPHYR_BASE=/home/${USERNAME}/workspace/zephyr

RUN mkdir -p /home/${USERNAME}/workspace \
    && west init \
        --mr ${ZEPHYR_VERSION} \
        /home/${USERNAME}/workspace \
    && cd /home/${USERNAME}/workspace \
    && west update \
    && west zephyr-export \
    && pip install -r ${ZEPHYR_BASE}/scripts/requirements.txt

# ── Environment ──────────────────────────────────────────────
ENV ZEPHYR_TOOLCHAIN_VARIANT=zephyr
ENV SHELL=/bin/zsh

# ── Shell config (.zshrc) ────────────────────────────────────
RUN echo 'export VIRTUAL_ENV_DISABLE_PROMPT=1'                                        >> ~/.zshrc \
    && echo "source ${VIRTUAL_ENV}/bin/activate"                                       >> ~/.zshrc \
    && echo "source ${ZEPHYR_BASE}/zephyr-env.sh 2>/dev/null || true"                 >> ~/.zshrc \
    # Set ZEPHYR_EXTRA_MODULES at runtime only (app bind-mount is present then)
    && echo 'export ZEPHYR_EXTRA_MODULES=/home/zephyr/workspace/app'                  >> ~/.zshrc \
    && echo 'echo ""'                                                                  >> ~/.zshrc \
    && echo 'echo "  Zephyr workspace : /home/zephyr/workspace"'                      >> ~/.zshrc \
    && echo 'echo "  Your app         : /home/zephyr/workspace/app (bind-mount)"'     >> ~/.zshrc \
    && echo 'echo "  Build example    : west build -b <board> /home/zephyr/workspace/app/app"' >> ~/.zshrc \
    && echo 'echo ""'                                                                  >> ~/.zshrc

WORKDIR /home/${USERNAME}/workspace

CMD ["/bin/zsh"]