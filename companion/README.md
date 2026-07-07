# Mood Companion (Android)

Companion-appen som tar emot Moods registreringar från klockan och (så småningom) gör
historik, trender och korrelationer. Plan: [../design/companion_app_plan.md](../design/companion_app_plan.md).

**Nuvarande läge: spike (steg 1).** Appen är en minimal import-lyssnare: en hand-rullad
HTTP-server på `127.0.0.1:9099/import` + en logg-vy. pkjs (i Pebble-appen) POST:ar hela
exporten dit när klockappen startar. Emulator-halvan av spiken är verifierad (pypkjs →
lyssnare i containern); den här appen verifierar telefon-halvan.

## Bygga & installera (i devcontainern, efter rebuild)

```bash
cd companion
./gradlew :app:assembleDebug          # bygga
./gradlew :app:installDebug           # installera (kräver adb-ansluten telefon)
```

Telefon via wireless debugging (telefonens IP är öppen i containerns firewall):

1. På telefonen: Inställningar → Utvecklaralternativ → Trådlös felsökning → aktivera.
2. "Koppla enhet med kopplingskod" → notera ip:port + kod.
3. I containern: `adb pair <ip>:<pair-port>` (ange koden), sen `adb connect <ip>:<port>`.

## Spike-testet på telefonen

1. Installera companion-appen och öppna den (lyssnaren är aktivitets-bunden — håll den öppen).
2. Öppna Pebble-appen → klockappens JS-sida startar om → exporten pushas.
3. Companion-appen ska visa "✅ Import #1: N registreringar".

Får du inget: kolla `adb logcat -s MoodCompanion MoodImportServer` och Pebble-appens
utvecklarlogg ("Companion delivery: HTTP 200" vs "failed"). Faller spiken (JS-runtimen
vägrar cleartext-localhost) är fallback PebbleKit Android — se planen.
