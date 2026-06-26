#!/bin/bash
set -euo pipefail
IFS=$'\n\t'

# Egress/ingress-firewall för Claude remote-control-sandboxen. Policy: den isolerade miljön får inte
# nå det interna företagsnätet. Metod: default-ALLOW, blocklista de interna RFC1918-ranges, med
# undantag bara för containerruntime-ns egen plumbing.
#
# Det här är ett personligt GitHub-repo över publika internet, så det finns INGET internt SCM-undantag
# (GitHub nås redan via default-allow). Kvar är policy-kärnan: blockera 10/8 och 192.168/16.
#
# Kräver NET_ADMIN/NET_RAW (sätts i devcontainer.json runArgs); körs som postStartCommand via sudo
# (sudoers-regel scopad till ENBART detta skript, se Dockerfile).

# Flusha ENDAST OUTPUT/INPUT. Kör ALDRIG `iptables -F`/`-X`: det raderar containerruntimes egna
# filter-kedjor (DOCKER, DOCKER-USER, DOCKER-ISOLATION) och bryter docker-in-docker. Docker routar
# container-trafik via FORWARD/nat, aldrig OUTPUT/INPUT, så att flusha just dessa två är säkert.
# Gör om-körningar idempotenta.
iptables -F OUTPUT
iptables -F INPUT

iptables -P INPUT ACCEPT
iptables -P FORWARD ACCEPT
iptables -P OUTPUT ACCEPT

# --- OUTPUT (egress) ---
# Undantag MÅSTE komma FÖRE LAN-blocken (first match wins i iptables).
# Docker Desktops interna tjänster (inbäddad DNS-resolver + host-gateway) ligger på 192.168.65.0/24.
# UTAN detta undantag dödar 192.168/16-blocket DNS och inget kan slås upp. Verifiera range på målet.
iptables -A OUTPUT -d 192.168.65.0/24 -j ACCEPT
# 172.16.0.0/12 lämnas ALLOWED med flit: Docker använder 172.17.x/172.18.x för sina bryggor och
# docker-in-docker behöver dem.
iptables -A OUTPUT -d 10.0.0.0/8 -j REJECT --reject-with icmp-admin-prohibited
iptables -A OUTPUT -d 192.168.0.0/16 -j REJECT --reject-with icmp-admin-prohibited

# --- INPUT (ingress) ---
iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
iptables -A INPUT -s 10.0.0.0/8 -j REJECT --reject-with icmp-admin-prohibited
iptables -A INPUT -s 192.168.0.0/16 -j REJECT --reject-with icmp-admin-prohibited

echo "Firewall configured (blocklist mode). Verifying..."
curl --connect-timeout 5 -s https://example.com >/dev/null 2>&1 \
    && echo "  ok: public Internet reachable" \
    || { echo "ERROR: public Internet unreachable - default-allow misconfigured"; exit 1; }
curl --connect-timeout 5 -s https://github.com >/dev/null 2>&1 \
    && echo "  ok: GitHub reachable" \
    || echo "WARN: GitHub unreachable - git push/pull will fail"
curl --connect-timeout 3 -s http://10.0.0.1 >/dev/null 2>&1 \
    && { echo "ERROR: 10.0.0.1 reachable - block rules not effective"; exit 1; } \
    || echo "  ok: internal IP blocked"
echo "Firewall verification done."
