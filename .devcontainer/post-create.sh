#!/usr/bin/env bash
set -euo pipefail

# --- Pebble-toolchain (Core Devices) ---
# Core Devices rekommenderar Python 3.13 (3.14 stöds inte än). uv hämtar den vid behov.
uv tool install pebble-tool --python 3.13

export PATH="$HOME/.local/bin:$PATH"

# --- Git-identitet från host (utan dess Windows-safe.directory-brus) ---
# Host-configen är read-only monterad på en sido-sökväg. Plocka bara ut identiteten till en
# container-egen, skrivbar ~/.gitconfig så `git` slutar gnälla om icke-absoluta safe.directory-rader.
HOST_GITCONFIG="$HOME/.gitconfig-host"
if [ -f "$HOST_GITCONFIG" ]; then
    name=$(git config -f "$HOST_GITCONFIG" --get user.name || true)
    email=$(git config -f "$HOST_GITCONFIG" --get user.email || true)
    [ -n "$name" ] && git config --global user.name "$name"
    [ -n "$email" ] && git config --global user.email "$email"
    git config --global push.autosetupremote true
fi

# Hämtar senaste SDK:n. yes-pipen besvarar analytics-/licensfrågan icke-interaktivt. När pebble
# stänger sin stdin får `yes` SIGPIPE (exit 141), vilket med pipefail annars fäller hela skriptet
# fast installationen lyckats. Stäng av pipefail för raden, så blir statusen pebbles egen.
set +o pipefail
yes | pebble sdk install latest
set -o pipefail

# --- Android-SDK (companion-appen) ---
# Ligger i named volume (pebble-mood-android-sdk) så nedladdningen (~600 MB) överlever rebuilds.
# Körs vid postCreate = FÖRE firewallen, så dl.google.com är nåbar. Gradle-beroenden vid byggtid
# fungerar ändå: firewallen blockerar bara LAN, inte publika internet.
ANDROID_HOME="$HOME/android-sdk"
CMDLINE_TOOLS_ZIP="commandlinetools-linux-11076708_latest.zip"
if [ ! -d "$ANDROID_HOME/cmdline-tools/latest" ]; then
    echo "Installerar Android cmdline-tools..."
    mkdir -p "$ANDROID_HOME/cmdline-tools"
    curl -Lo "/tmp/$CMDLINE_TOOLS_ZIP" "https://dl.google.com/android/repository/$CMDLINE_TOOLS_ZIP"
    unzip -q "/tmp/$CMDLINE_TOOLS_ZIP" -d "$ANDROID_HOME/cmdline-tools"
    mv "$ANDROID_HOME/cmdline-tools/cmdline-tools" "$ANDROID_HOME/cmdline-tools/latest"
    rm "/tmp/$CMDLINE_TOOLS_ZIP"
fi

SDKMANAGER="$ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager"
# Samma SIGPIPE-dans som för pebble sdk ovan.
set +o pipefail
yes | "$SDKMANAGER" --licenses > /dev/null
set -o pipefail
"$SDKMANAGER" --install "platform-tools" "platforms;android-35" "build-tools;35.0.0" > /dev/null
echo "Android-SDK klar: $("$ANDROID_HOME/platform-tools/adb" --version | head -1)"

# --- Container-lokal deploy key (remote-control-sandbox, least credential blast radius) ---
# Ingen host-~/.ssh monteras in. Istället en dedikerad nyckel i en named volume (överlever rebuilds).
# Registrera dess publika halva som read/write deploy key på ENBART det här GitHub-repot.
DEPLOY_KEY="$HOME/.ssh/id_ed25519_pebble_mood"
if [ ! -f "$DEPLOY_KEY" ]; then
    ssh-keygen -t ed25519 -N "" -f "$DEPLOY_KEY" -C "pebble-mood-devcontainer"
fi

SSH_CONFIG="$HOME/.ssh/config"
if ! grep -q "id_ed25519_pebble_mood" "$SSH_CONFIG" 2>/dev/null; then
    cat >> "$SSH_CONFIG" <<EOF
Host github.com
    HostName github.com
    User git
    IdentityFile $DEPLOY_KEY
    IdentitiesOnly yes
EOF
    chmod 600 "$SSH_CONFIG"
fi

ssh-keyscan -t ed25519 github.com >> "$HOME/.ssh/known_hosts" 2>/dev/null || true
sort -u "$HOME/.ssh/known_hosts" -o "$HOME/.ssh/known_hosts" 2>/dev/null || true

echo ""
echo "================================================================"
echo "Deploy key (lägg till som read/write deploy key på GitHub-repot:"
echo "  https://github.com/david-s-svedberg/pebble-mood/settings/keys ):"
echo ""
cat "${DEPLOY_KEY}.pub"
echo "================================================================"
