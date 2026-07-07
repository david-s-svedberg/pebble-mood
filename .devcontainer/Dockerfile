FROM mcr.microsoft.com/devcontainers/base:ubuntu-24.04
LABEL MAINTAINER="David Svedberg"

# Pebble-emulatorns och SDK:ns systemberoenden (Core Devices pebble-tool).
# Notera SDL2 (inte SDL1 som gamla Rebble-SDK:n krävde).
# iptables/ipset/dnsutils/aggregate/jq behövs av remote-control-sandboxens firewall.
# openjdk-17 + unzip behövs av Android-toolchainen (companion-appen); själva
# Android-SDK:n installeras av post-create.sh till en named volume.
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        nodejs \
        npm \
        libsdl2-2.0-0 \
        libglib2.0-0 \
        libpixman-1-0 \
        libpng16-16t64 \
        zlib1g \
        libsndio7.0 \
        iptables \
        ipset \
        dnsutils \
        aggregate \
        jq \
        openjdk-17-jdk-headless \
        unzip \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*
ENV DEBIAN_FRONTEND=dialog

# uv hanterar Python-versionen och installerar pebble-tool. Läggs i /usr/local/bin
# så att den är på PATH för alla användare i imagen.
RUN curl -LsSf https://astral.sh/uv/install.sh | env UV_INSTALL_DIR=/usr/local/bin sh

# Remote-control-sandboxens firewall: kopiera skriptet och ge vscode passwordless sudo för ENBART
# det (least privilege), inte generell sudo.
COPY init-firewall.sh /usr/local/bin/init-firewall.sh
RUN chmod 0755 /usr/local/bin/init-firewall.sh \
    && echo "vscode ALL=(root) NOPASSWD: /usr/local/bin/init-firewall.sh" > /etc/sudoers.d/vscode-firewall \
    && chmod 0440 /etc/sudoers.d/vscode-firewall

# Skapa mountpunkterna ägda av vscode INNAN named volumes monteras: en tom named volume ärver
# katalogens ägare/rättigheter från imagen vid första monteringen, så vscode (uid 1000) kan skriva.
# android-sdk ligger i en named volume så att ~600 MB SDK överlever rebuilds.
RUN mkdir -p /home/vscode/.claude /home/vscode/.ssh /home/vscode/android-sdk \
    && chown -R vscode:vscode /home/vscode/.claude /home/vscode/.ssh /home/vscode/android-sdk \
    && chmod 700 /home/vscode/.ssh
