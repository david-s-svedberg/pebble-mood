# Companion-appen — plan

Android-app som tar emot Moods registreringar från klockan, lagrar historiken och gör
analysen som medvetet INTE bor på klockan: överblick, trender och korrelationer
(Joy vs sömn, Anxiety vs träning, lag-effekter). Klockan förblir "dum" insamlare.

## Dataväg: klocka → telefon → companion (nyckelbeslutet)

Exporten från klockan är klar och verifierad: `data_export.c` streamar alla registreringar
över AppMessage till pkjs (`src/pkjs/index.js`) med komplett kontrakt
(metric id/namn/typ/min/max, värde, timestamp, grupp-id/namn). Frågan är sista hoppet,
pkjs → companion-appen:

| Alternativ | Hur | För | Emot |
|---|---|---|---|
| **A. pkjs → localhost-POST (rekommenderad)** | pkjs XHR:ar batchen till `http://127.0.0.1:<port>/import` som companion-appen serverar (inbäddad HTTP-lyssnare i en foreground service) | Återanvänder den fungerande pkjs-mottagaren; ingen moln-beroende; datan lämnar aldrig telefonen | Companion-tjänsten måste lyssna när exporten sker; cleartext-localhost kräver networkSecurityConfig; **ovissa CORS/nätverksregler i Core Devices-appens JS-runtime → spike först** |
| B. PebbleKit Android direkt | Companion registrerar `PebbleDataReceiver` för appens UUID, klockan skickar direkt | Inget HTTP-hopp | Klassisk begränsning: watchapp MED pkjs-komponent får meddelanden routade till JS-runtimen, PebbleKit-broadcasts blir opålitliga; skulle kräva att pkjs droppas (och den gör redan jobbet) |
| C. Moln-endpoint | pkjs POST:ar till server | Funkar även utan companion igång | Overkill, privacy, drift — måendedata ska inte behöva lämna telefonen |

**Beslut: A, gated på en spike** (steg 1 nedan). Faller spiken: B (droppa pkjs, flytta
mottagningen till PebbleKit Android).

## Steg (MVP → korrelationer)

1. **Spike: localhost-hoppet.** Minimal companion-app = foreground service med inbäddad
   HTTP-lyssnare (Ktor embedded eller NanoHTTPD) + logg. pkjs utökas med POST av den
   färdiga exportlistan. Bevis: registreringar syns i companion-loggen. *Avgör A vs B.*
2. **Mottagning + lagring.** Room-databas; dedupe på `(metricId, groupId, timestamp)`
   (klockan skickar om HELA historiken varje export tills gallring finns — importen måste
   vara idempotent). Metric-metadata (namn/typ/min/max) versioneras per import.
3. **Historik-UI.** Jetpack Compose: dagvy (alla svar per dag, grupperade per pass) +
   metricvy (alla svar för en metric över tid).
4. **Trender.** Per-metric-graf (Vico), daglig aggregering: grupp-slots är redan "senaste
   svaret per pass"; spontana medelvärdas per dag. 7/30-dagarsfönster. Normalisera skalor
   via exporterade min/max.
5. **Ack + gallring** (kräver liten klock-ändring): companion svarar OK → pkjs skickar
   ACK-AppMessage → klockan gallrar exporterade registreringar äldre än ~7 dagar.
   Löser klockans ändliga persist-budget (se ROADMAP).
6. **Korrelationer.** Per-dag-serier per metric; parvis Spearman-korrelation inklusive
   lag ±1 dag (tränade igår → mående idag). "Insikter"-vy: starkaste sambanden med
   konfidens-brasklapp (få datapunkter första veckorna).

## Teknikval

- **Kotlin + Jetpack Compose**, minSdk 26, target/compile SDK 35.
- **Room** för lagring, **Vico** för grafer, **Ktor embedded** (eller NanoHTTPD) för
  import-lyssnaren.
- **Monorepo:** appen bor i `companion/` i det här repot (delar ROADMAP, review-skill och
  export-kontraktet; kontraktsändringar i `messageKeys`/pkjs/companion hålls i samma commit).
  Gradle-wrapper checkas in — ingen global gradle i containern.

## Dev-miljö (klar i devcontainern)

- JDK 17 + unzip i imagen; Android-SDK:n (cmdline-tools, platform-tools, android-35,
  build-tools 35) installeras av post-create till named volume `pebble-mood-android-sdk`.
  `ANDROID_HOME` + PATH satta i devcontainer.json. **Kräver container-rebuild.**
- **Installation på telefonen: wireless debugging.** Telefonen (192.168.1.161) är redan
  öppen i firewallen. Engångs: aktivera Wireless debugging i utvecklarinställningarna,
  `adb pair <ip>:<pair-port> <kod>`, sedan `adb connect <ip>:<port>` +
  `./gradlew :app:installDebug`. (Ingen emulator i containern — fysisk telefon är målet.)
- Verifiering i containern: `./gradlew build` + `adb logcat`-filter på appens tagg.

## Öppna frågor (avgörs under resans gång)

- Core Devices-appens JS-runtime: tillåts XHR mot localhost? (Spiken, steg 1.)
- Trigga export på begäran från companion (idag pushar pkjs vid 'ready') — räcker
  auto-push, eller behövs en pull-knapp (PebbleKit-intent / notifikation)?
- Grafen på klockans hemskärm (ROADMAP-idé) hämtar sin data lokalt på klockan — oberoende
  av companion, men bör dela aggregerings-semantiken (senaste per pass / medel per dag).
