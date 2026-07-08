# Companion-appen — plan

Android-app som är en **fullvärdig tvilling** till klockappen: tar emot registreringar,
lagrar historiken, kan själv mata in metrics (telefonläge med egna notiser), hanterar
konfigurationen (grupper/metrics/ikoner, synkad till klockan) och gör analysen som
medvetet INTE bor på klockan: grafer, korrelationer (matematiskt + AI-analys som tillval)
och hälsodata via Health Connect.

**Faserna och besluten ägs av [../ROADMAP.md](../ROADMAP.md) (companion-avsnittet)** —
det här dokumentet håller de tekniska detaljerna.

**Status: spike (steg 1) VERIFIERAD på riktig hårdvara 2026-07-07** — hela kedjan
klocka → AppMessage → pkjs (Core Devices 1.5.0.2) → localhost-POST → companion (Pixel 6).
Gotcha: XHR-bryggan kräver hostnamnet `localhost`; råa IP:n (`127.0.0.1`) ger onerror.
Dataväg A gäller; PebbleKit-fallbacken behövdes aldrig.

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

**Utfall: A verifierad.** PebbleKit-fallbacken behövdes aldrig.

## Tekniska detaljer per fas (fasordning + beslut: se ROADMAP)

- **Fas 2 (mottagning):** foreground service äger `ImportServer`-lyssnaren (spike-versionen
  är aktivitets-bunden — flyttas). Room med dedupe-nyckel `(metricId, groupId, timestamp)`;
  metric-metadata (namn/typ/min/max/ikon) upsertas per import. **Ack + gallring:**
  companion svarar `{status:"ok", ackedThrough:<timestamp>}` → pkjs skickar
  ACK-AppMessage → klockan gallrar synkade registreringar äldre än ~7 dagar — MEN
  favorit-metricsens trendcache (klockgrafen, ROADMAP §2) fylls FÖRE gallring.
- **Fas 3 (graf):** Vico. Dag-aggregerad översikt + detaljzoom med råa tidsstämplar
  (spontana = flera punkter/dag). Bool = event-markörer på egen rad, aldrig linjer.
  Normalisering via min/max.
- **Fas 4 (config-synk):** nytt AppMessage-kontrakt telefon→klocka (config-entiteter:
  grupp {id, namn, tid, aktiv}, metric {id, namn, typ, min/max, ikoner, texter},
  medlemskap). Klockan applicerar via repositories. `IconChoice`-enumen blir delad
  kontraktskonstant. Konfliktmodell: last-write-wins per entitet (revisionsräknare).
- **Fas 5 (telefonläge) — KLAR:** global setting (`PhoneMode`, SharedPreferences). Av-läge
  synkar en global `alarms_suspended`-flagga (AppConfig, `SET_ALARMS_SUSPENDED`) i stället
  för att klottra varje grupps `alarm.active` — `ensure_all_alarms_scheduled` avarmerar allt
  vid suspend och återställer exakt schemat vid resume. Lokala notiser via AlarmManager
  (`setAndAllowWhileIdle`, inexakt → ingen `SCHEDULE_EXACT_ALARM`-behörighet), återarmeras per
  träff (ReminderReceiver) + efter omstart (BootReceiver). Inmatnings-UI (Telefon-flik,
  `PhoneScreen`) återanvänder registreringsflödets semantik (en metric i taget, knapp=värde;
  bool→Nej/Ja, three_option→3 knappar, interval→min..max). Telefonsvar lagras lokalt direkt
  och köas som `kind:"registration"` → klockan (`SET_REG_*`) uppdaterar dagens slot in-place
  eller appendar. Kön töms via /pending-ack; ingen dubbelapplicering (idempotent per dag).
  **Klockgrafens trendcache är ännu inte byggd** — sync-back uppdaterar registrerings-storen,
  cachen kopplas in när den finns (§2).
- **Fas 6 (Health Connect):** läsbehörigheter för sömn/steg/puls; daglig aggregering till
  auto-metrics. Steg 0: `adb shell` / HC-appen — verifiera vilka källor som skriver.
- **Fas 7 (korrelation):** Spearman + lag ±1d på per-dag-serier, generisk över alla
  metrics (även användarskapade). AI-tillval: JSON-dump av vald period → Claude API
  (nyckel i settings, privacy-notis) → resonemang renderas/sparas.

## Teknikval

- **Kotlin + Jetpack Compose**, minSdk 26, target/compile SDK 35.
- **Room** för lagring, **Vico** för grafer; import-lyssnaren är en hand-rullad
  loopback-`ServerSocket` (noll beroenden, redan skriven i spiken).
- **Monorepo:** appen bor i `companion/` i det här repot (delar ROADMAP, review-skill och
  export-kontraktet; kontraktsändringar i `messageKeys`/pkjs/companion hålls i samma commit).
  Gradle-wrapper incheckad — ingen global gradle i containern.

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

- ~~Core Devices-appens JS-runtime: tillåts XHR mot localhost?~~ **Ja** — men bara med
  hostnamnet `localhost`, inte råa IP:n (verifierat på coreapp 1.5.0.2).
- Skriver Core Devices-appen Pebble-hälsodata till Health Connect? (Fas 6, steg 0.)
- Trigga export på begäran från companion (idag pushar pkjs vid 'ready') — räcker
  auto-push, eller behövs en pull-knapp?
- Config-synkens transport: AppMessage-kontraktet växer rejält i fas 4 — utvärdera om
  pkjs bör chunka config-payloads likt klockans persist_blob.
- Grafen på klockans hemskärm delar aggregerings-semantik med companion (senaste per
  pass / medel per dag) — håll definitionerna i sync.
