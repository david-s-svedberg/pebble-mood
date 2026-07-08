# Mood — Roadmap

Planerade förbättringar och öppna trådar. Se [CLAUDE.md](CLAUDE.md) för nuläge/arkitektur
och [design/design_notes.txt](design/design_notes.txt) för den ursprungliga datamodellen.

**Status (uppdaterad):** hela den ursprungliga grundplanen (avsnitt 1–6 nedan) är klar,
liksom polish-batchen (tema, timeout, option-text). En full post-implementation-review
(8 pass: minne, persistens, alarm/tid, export, arkitektur, UI, budget, docs — se
`.claude/skills/post-implementation-review/`) genomfördes i commits `95e7bb2..c417516`
och åtgärdade bl.a. en sträng-UAF, tyst 256 B-persisttrunkering och döda gruppalarm.
Kvar är de stora spåren: companion-app, hälsodata och graf-hemskärmen.

**Plattformar:** diorite, basalt, emery (PT2). *Aplite (Pebble Classic/Steel) droppades* —
24KB app-RAM räckte inte längre (statiskt data svämmade över och heapen var nere på ~1,6KB).

---

## Klart — grundplanen

1. **Seed-data** — Vid första start seedas konkreta känslometrics: Joy (1–5), Anxiety (0–5),
   Irritation (0–5), Stress (0–5), Love (bool, hjärta), Sleep quality, Rested, Exercised,
   plus o-schemalagda Sick och Medication. Grupper: Morning 07:30, Lunch 12:30, Evening 21:30
   (Joy/Irritation återkommer i flera pass — spåras separat per pass).
2. **Svarsmodell** — Bool (Upp/Ner), 3 alternativ (Upp/Select/Ner), Interval
   ([min..max], min ställbart 0/1 via "Min"-raden). Redigerbart namn, huvudikon (stor,
   centrerad vid registrering), ikon + text per alternativ (texten visas nu som etikett
   bredvid respektive knapp på registreringsskärmen). Ikon-väljare (custom MenuLayer,
   highlight-medveten) istället för cykling. Alla ikoner finns i svart + vit variant;
   alla menyer och action bars väljer varianten som kontrasterar mot bakgrunden.
3. **Spontan registrering** — Select på hemskärmen → metriclista (utan subtitlar) →
   registrera; lägger alltid till en ny datapunkt (flera per dag ok). Spontana
   registreringar syns i Today under en "Misc"-sektion.
4. **Smarta alarm** — Gruppalarm frågar bara metrics som inte redan besvarats idag *för det
   passet*; helt besvarad grupp ⇒ inget alarm, ingen vibration.
5. **Many-to-many + o-schemalagda** — `GroupMetric`-join; metric utan grupp = o-schemalagd.
   Registreringar bär `group_id` (0 = spontan) så samma metric spåras per pass och dag.
6. **Hemskärm + Today** — Hemskärm med stor mood-ikon centrerad, "Next: HH:MM" nere till
   vänster, action bar (Upp=Today, Select=spontan, Ner=Settings). Today är grupperad per
   grupp (+Misc), subtitle = dagens svar för det passet (Yes/No, 1..n eller option-text),
   klick uppdaterar svaret på plats.

## Klart — polish

- [x] **Theme-toggle** i Settings — växlar mörkt/ljust tema live. Alla menyer, action bars
      och ikoner är temamedvetna (inverterade bar-ikoner genererade för ljust tema).
- [x] **Alarm timeout** ställbar i Settings (1–4 min; `alarm_timeout_sec` är uint8).
- [x] **Option-text på registreringsskärmen** — etikett vid respektive knapp.
- [x] Alarmrubrik = gruppnamn; Max bara för Interval; edit-alarm emery-layout;
      `Alarm.index`-sentinels; wakeup-id persisteras; window-leak + double-free fixade;
      eget UUID (krockade med meds-appen); launcher-ikon (svart smiley).

---

## Nästa stora spår

### 1. Companion-app — SPIKE VERIFIERAD, målbild beslutad

Detaljplan i [design/companion_app_plan.md](design/companion_app_plan.md). **Spiken är i
land på riktig hårdvara** (2026-07-07): klocka → AppMessage → pkjs → localhost-POST →
companion-appen på telefonen, komplett kontrakt mottaget. (Gotcha: Core Devices XHR-brygga
kräver hostnamnet `localhost`, råa IP:n ger onerror.) Skelettet bor i `companion/`.

**Målbild (beslutad 2026-07-07):** companion-appen är en fullvärdig tvilling till
klockappen — inte bara en mottagare. Faserna, i ordning:

**Fas 2 — Riktig mottagning.** Foreground service (beslutat: synk ska ske direkt när data
matats in på klockan, inte bara när appen råkar vara öppen) + Room-lagring med idempotent
import (klockan omsänder hela historiken tills gallring finns; dedupe på
`(metricId, groupId, timestamp)`). Därefter **ack + gallring**: klockan rensar synkade
registreringar äldre än ~7 dagar (persist-budgeten).

**Fas 3 — Grafläge.** Välj vilka metrics som visas; utveckling över tid.
- Två upplösningar: **dag-aggregerad översikt** (medel per dag för intervall) och
  **detaljzoom** med faktiska tidsstämplar där spontana registreringar syns som egna
  punkter (dynamisk x-axel). Exakt balans utvärderas mot riktig data.
- **Booleans är events, inte linjer**: markörer/prickar på egen rad ("den dagen tränade
  jag") — aldrig interpolerade kurvor.
- Skalor normaliseras via exporterade min/max (Joy 1–5 vs Anxiety 0–5).

**Fas 4 — Konfigurationsparitet + synk.** Grupper, metrics, medlemskap och ikoner ska
kunna redigeras i Android-appen och synkas till klockan (nytt AppMessage-kontrakt:
config-synk telefon→klocka). Klockans config förblir redigerbar; enkel konfliktmodell
(senaste ändring vinner per entitet) — designas i fasen.

**Fas 5 — Telefonläge (globalt val) — KLAR (verifierad på hårdvara 2026-07-08).** Setting
"Pebble-integration" på/av (companion, Telefon-flik):
- **På** (default): larm + inmatning på klockan som idag.
- **Av**: klockans larm avaktiveras; telefonen tar över med notiser vid grupptiderna och
  samma inmatningsflöde (en metric i taget) i appen.
- **Telefonsvar synkas alltid tillbaka till klockan** så "redan besvarad idag"-logiken och
  klockans graf stämmer oavsett var man svarade.
- Klockans avstängning sker via en **global `alarms_suspended`-flagga i AppConfig** som
  nollställer grupp-wakeups utan att röra varje grupps egna `alarm.active` — att slå på igen
  återställer exakt det gamla schemat (verifierat: 09:30/12:30/21:30 kom tillbaka). Synkas
  över pending-kön (`kind:"app"`), telefonsvar likaså (`kind:"registration"`, uppdaterar
  dagens slot in-place på klockan). Notiser via AlarmManager (inexakt/idle, ingen exakt-larm-
  behörighet), återarmeras vid varje träff och efter omstart (BootReceiver).

**Fas 6 — Hälsodata via Health Connect — KLAR (verifierad på hårdvara 2026-07-08).**
Companion läser sömn, steg och puls ur Androids Health Connect som auto-metrics i
grafer/korrelationer — fångar alla källor utan klock-kod.
- **Steg 0-utfallet:** `coredevices.coreapp` HAR beviljade HC-WRITE-behörigheter för
  HEART_RATE/SLEEP/STEPS/EXERCISE — Core Devices-appen skriver alltså Pebble-hälsodatan till
  Health Connect. Läsvägen fungerar, klock-spåret behövdes aldrig.
- Companion läser STEPS/SLEEP/HEART_RATE, aggregerar per lokal dag
  (`aggregateGroupByPeriod`, `Period.ofDays(1)`), och skapar tre auto-metrics (id 9001–9003,
  utanför klockans id-rymd så de aldrig krockar/exporteras): Steg, Sömn (min), Puls (snitt).
  Full-refresh av health-raderna vid varje hämtning. De dyker upp i grafen och
  korrelationerna automatiskt (delad `DailyAggregation`); valens-default steg/sömn +1, puls −1.
  Verifierat med riktig Pebble-data: t.ex. Stress↔Steg −0,72, Sömn→Exercised(+1d) +0,65.
- connect-client pinnad till 1.1.0-alpha10 (rc/stable kräver AGP 8.9 + compileSdk 36).

**Fas 7 — Korrelation (beslutat: matte inbyggd + AI som tillval).**
- **Matematisk grund — KLAR (v1):** per-dag-serier per metric (delad aggregerings-semantik
  med grafen via `DailyAggregation`); parvis Spearman inklusive lag ±1 dag; "Insikter"-flik
  sorterad på styrka med dagantal + brasklapp vid <14 dagar. Generisk över alla metrics.
  Verifierad mot demodatans planterade samband (Irritation↔Stress +0,84, Joy↔Anxiety −0,62,
  Exercised→Joy nästa dag +0,39 — alla hittade).
- **Valens per metric (beslutad idé, ej byggd):** metadata om huruvida högt värde är bra,
  dåligt eller neutralt (Joy = positiv, Anxiety = negativ, …). Behövs INTE för matematiken
  (Spearmans tecken bär riktningen) men lyfter presentationen: "träning verkar bra för dig"
  istället för "samband åt motsatt håll", och möjliggör ett framtida "bra dagar"-index.
  Bor enbart i companion-appens MetricEntity (klockan analyserar inget — ingen export- eller
  klockändring); sätts i companion-appens metric-redigerare (fas 4), rimliga defaults för
  seed-uppsättningen.
- **AI-analys som explicit funktion:** "Analysera period"-knapp som dumpar vald period
  (registreringar + hälsodata) som JSON till Claude API och visar resonemanget om vad som
  verkar påverka vad. API-nyckel i settings + tydlig notis om att datan lämnar telefonen
  vid just de anropen.

### 2. Klockans graf-hemskärm — KLAR (emulator-verifierad 2026-07-08)

Välj **1–3 favoritmetrics** (Settings → Home graph, cap 3); hemskärmen visar deras 7-dygns
utveckling som sparklines i stället för stora ikonen (0 favoriter → ikonen kvar).
- **Lagringsbeslutet ändrat:** ingen separat trendcache. Aggregaten (dagsmedel per favorit)
  beräknas **on-demand ur registrations-storen** vid hemskärmens `.appear` — storen behåller
  redan en ≥7-dygns svans (gallringen sparar just för trender), så en cache vore redundant och
  bara en källa till staleness. `trend.c` gör en pass över registreringarna per favorit;
  effektiv min/max per typ normaliserar höjden (delad semantik med data_export).
- Favoriter lagras i AppConfig (`favorite_metrics[3]`, v7). Saknad data/dag → glapp i linjen;
  favorit utan data → "ingen data". Uppdateras automatiskt när man kommer tillbaka från en
  registrering eller telefon-sync (appear beräknar om).

## Öppna småtrådar

- [ ] **Visuell röktest på 144×168** (diorite/basalt-emulatorn) — layouten är kodgranskad
      (allt beräknas från bounds/konstanter) men appen har bara synats på emery, inkl. den
      nya hemskärmsgrafen. Sparkline-höjden på 144×168 blir ~39 px/rad vid 3 favoriter
      (beräknat, ej synat). En engångs-syn när emulatorn beter sig.
      (Not: qemu-pebble ligger i SDK:ns `toolchain/bin` och är inte på PATH i containern —
      exportera `$HOME/.local/share/pebble-sdk/SDKs/4.17/toolchain/bin` före `pebble install
      --emulator`.)

- [ ] **Röstdiktering kräver ansluten telefon** — utan telefon får nya metrics/grupper
      defaultnamn. Ev. textfritt/förvalt alternativ.
- [ ] **Fler valbara ikoner** vid behov (pipeline finns: Pillow-script → svart/vit/20px-
      varianter → `IconChoice`).
- [ ] Emery-layouterna kan finputsas ytterligare för den större skärmen.
