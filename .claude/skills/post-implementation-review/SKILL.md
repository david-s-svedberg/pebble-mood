---
name: post-implementation-review
description: Grundlig post-implementation-review av Mood (native C Pebble-app + PebbleKit JS-export + Android companion-app). Passen i ordning - 1 Minnessäkerhet & resurslivscykler, 2 Persistens & dataintegritet, 3 Alarm & tidslogik, 4 Watch↔telefon-kontraktet (export + config-synk), 5 Arkitektur & död kod, 6 UI-konsekvens (tema/ikoner/plattformar), 7 Minnesbudget & assets, 8 Companion data & synk-integritet, 9 Companion privacy/nätverk/livscykel, 10 Uppdatera docs/minne. Körs efter en längre implementationsrunda; stannar för godkännande mellan varje pass.
---

# Post-implementation review — Mood

En sekvens fokuserade granskningspass efter en längre implementationsrunda. Varje pass läser
hela kodbasen genom EN lins, presenterar fynd och STANNAR för godkännande innan något fixas.
Ordningen är efter allvarlighetsgrad: först klockans C-app (minnesfel kraschar klockan —
double-free, window-läckor har hänt; persistensfel förstör data; tidsfel gör att alarm
uteblir), sedan watch↔telefon-kontraktet, struktur, utseende, budget, DÄREFTER companion-
appen (data-integritet + privacy/nätverk), och sist dokumentation.

Projektet har numera **två kodbaser**:
- **Klockan:** en **inbyggd C-app för Pebble** (SDK 4.x, targets diorite/basalt/emery). Här
  ÄR "säkerhet" minnessäkerhet + dataintegritet; klassiska webbsäkerhetspass utelämnas.
  Verifiering: `pebble build` (alla tre plattformar) + röktest i emery-emulatorn (och vid
  behov fysisk klocka). Ingen automatisk testsvit.
- **Companion-appen:** en **Android-app** (Kotlin + Jetpack Compose + Room) i `companion/`,
  byggd fas 2–7. Den har en riktig nätverks-/privacy-yta som klock-appen saknar: en
  loopback-HTTP-server, en foreground service, WorkManager, Health Connect-läsning och ett
  Claude-API-anrop. Verifiering: `./gradlew :app:assembleDebug` i `companion/` + röktest på
  telefonen via `adb`. Ingen automatisk testsvit här heller. Companion-specifika riskklasser
  (Room-migrationer, idempotent import, samtidighet, dataläckage) täcks av pass 8–9.

## Grundregler för hela körningen

- Grundlighet före hastighet. Läs faktisk kod, följ kodvägar, verifiera misstankar — gissa inte.
- Läs HELA kodbasen i varje pass, genom det passets lins. Lita inte på minnet från ett
  tidigare pass; en tidsbugg kan gömma sig i en fil som bara skummades under minnespasset.
- STANNA efter varje pass: presentera fynd grupperade efter allvarlighetsgrad med `fil:rad`,
  föreslå en konkret fixplan, och vänta på uttryckligt godkännande. Fixa bara det som godkänts.
- Commit-grind mellan passen: godkända fixar byggs (`pebble build`, alla plattformar) och
  röktestas i emulatorn, committas till `main` med beskrivande meddelande enligt projektets
  rutin, och pushas — först därefter börjar nästa pass.
- Ett pass i taget. Följ användarens globala preferenser (språk, kodstil, git-rutin).

## Vad "hela kodbasen" betyder här

- **Klock-produktkod (full granskning):** `src/c/**` (appen), `src/pkjs/**` (telefonsidan av
  export/synk), `package.json` (resurser/messageKeys/targets), `wscript` (bygget).
- **Companion-produktkod (full granskning, pass 8–9):** `companion/app/src/main/java/**`
  (Kotlin), `companion/app/src/main/AndroidManifest.xml`, `companion/app/build.gradle.kts`.
  Bygg: `cd companion && ./gradlew :app:assembleDebug`.
- **Genererade assets (lägre ribba):** `resources/images/**` — granska att rätt varianter
  finns och används (svart/vit, storlekar), inte pixlarna i sig.
- **Verktyg (lägre ribba):** `.vscode/`, `.devcontainer/`, Gradle-wrappern.
- **Granskas aldrig:** `build/`, `companion/**/build/`, `.claude/`, `design/` (historiska
  anteckningar — men håll `design/companion_app_plan.md` faktakorrekt i pass 10).
- **Legacy:** `src/c/persistance.c`/`.h` är en död kvarleva från före repository-lagret
  (headern helt utkommenterad). Granska den inte rad för rad — flagga den för borttagning.

## Före pass 1: självkontroll av den här skillen

Kontrollera alltid först om DENNA skill behöver uppdateras för att projektet ändrats sedan
den senast kördes: nya kataloger/komponenter som passen inte nämner (t.ex. en companion-app,
hälsodata-källor, en graf-hemskärm — se ROADMAP.md), flyttade/omdöpta filer som ett pass
pekar på, ändrade konventioner, borttagna features (t.ex. droppade plattformar), eller en ny
riskklass. Om något hittas: föreslå ändringen, STANNA för godkännande, uppdatera + committa
skillen FÖRST. En inaktuell review-skill granskar fel saker.

## Passen, i ordning

### 1. Minnessäkerhet & resurslivscykler

Den dominerande riskklassen i denna kodbas — historiken innehåller en double-free-krasch
(config-menyns friade arrayer NULL:ades inte) och en heap-utmattning (fönster läckte per
öppning). Tänk på:

- Varje `malloc` har exakt en ägare och en `free`; pekare NULL:as efter free; ingen väg
  dubbel-friar (öppna/stäng samma fönster två gånger i rad i tanken).
- Fönster-livscykeln: `window_create` en gång + återanvänd (`if(m_window == NULL)`-mönstret),
  layers skapas i `.load` och förstörs ALLA i `.unload` (varje `*_create` har sin `*_destroy`).
- **Hängande pekare efter repository-mutationer:** `dynamic_add`/`dynamic_delete` gör
  `malloc`+`memcpy`+`free` på hela item-arrayen — varje `Metrics*`, `MetricsGroup*`,
  `Registration*`, `String*` som ett fönster eller en statisk variabel håller kvar över en
  add/delete är efter det ogiltig. Följ varje sådan pekare (t.ex. `m_metric` i
  metrics_config, `m_metric_group`, `Registration*` från `registration_today_...`).
- Buffertar: `snprintf`-storlekar vs faktiskt innehåll (rubriker, option-text, tider),
  `ROW_TEXT_LEN`-trunkering, arrayindex från `m_row_option`/`MenuIndex` mot faktiska antal.
- GBitmap-cacharna i `icons.c`: laddas lazy, förstörs via `destroy_all_icons` — anropas den
  någonsin? Är det OK att den inte gör det (appen avslutas ändå)? Motivera eller fixa.
- Klick-handlers som kan träffa NULL (t.ex. layers före `.load`, `m_metrics[m_current_index]`
  vid tom lista).

Presentera fynd efter allvarlighetsgrad med `fil:rad`, föreslå plan, STANNA.

### 2. Persistens & dataintegritet

Användarens data bor enbart i klockans persist-lagring via repository-lagret. Historiken
innehåller ett struct-layout-haveri (fält mitt i `AppConfig` → korrupta temafärger) och en
id-sentinel-kollision (id 0 = både Mood och "ingen grupp"). Tänk på:

- Struct-ändringar vs lagrade blobbar: gate:as allt av `CURRENT_DATA_VERSION` +
  storlekskontroll? Nya fält sist? (Pre-release: datareset är OK, tyst korruption är det inte.
  Efter publicering krävs riktiga migrationer — flagga när det börjar bli aktuellt.)
  `AppConfig` är nu **v7**: `alarms_suspended` (telefonläge) och `favorite_metrics[3]`
  (hemskärmsgrafen) är appendade sist — verifiera det, och att seeden nollställer dem rätt.
- Hemskärmsgrafens **trendcache-beslut**: det finns AVSIKTLIGT ingen cache — `trend.c`
  räknar dagsmedel on-demand ur registrations-storen (som behåller ≥7 dygn). Verifiera att
  antagandet håller: gallringen (`registrations_prune_synced`) får inte kapa svansen under
  7 dygn, annars glesnar grafen tyst.
- Id-konventioner: 0 reserverat som sentinel i alla stores (`next_id` startar på 1),
  `SNOOZED_ALARM_ID`/`SUMMER_TIME_ALARM_ID` = 255/254 och grupp-id:n håller sig under dem.
- `persist_write_*`-returvärden ignoreras genomgående — vad händer vid fullt lagringsutrymme
  eller för stor blob (persist har ~256 B/nyckel-gränser på vissa FW — verifiera för stora
  arrayer som registrations)? Minst: logga fel; helst: hantera.
- `DynamicData`-metadatan serialiseras rakt av (inklusive pekarfältet `items`) — robust vid
  läsning? Konsistens mellan meta (antal) och items-blob om en write misslyckas mittemellan.
- String-interning: `title_id`/`option_text_ids` återkopplas efter load (`string_get`),
  raderade strängar läcker inte id:n, `string_delete` vid omdöpning.
- Exportens fullständighet mot datamodellen (fält som tillkommit — se även pass 4).

Presentera, planera, STANNA.

### 3. Alarm & tidslogik

Appen är väckningsdriven: fel här = uteblivna eller dubbla alarm, eller fel grupp. Tänk på:

- Wakeup-schemaläggning: varje grupps alarm omschemaläggs efter avfyring/registrering och
  efter omstart; `wakeup_id` persisteras; kollisionshantering (±1 min-fönstret) och
  `wakeup_schedule`-felkoder.
- Snooze-flödet: `snoozed_group_id` sätts/läses rätt, snooze-alarm routas till rätt grupp,
  timeout (`alarm_timeout_sec`, nu 1–4 min) används faktiskt av alarmfönstret.
- **Telefonläge (`alarms_suspended`):** när flaggan är satt avarmerar
  `ensure_all_alarms_scheduled` alla grupp-wakeups UTAN att röra varje grupps `alarm.active`
  (så återgång återställer exakt schemat). Verifiera: suspend→resume ger tillbaka rätt tider;
  `config_apply.c` som sätter flaggan kör om schemaläggningen; DST-alarmet lever kvar och
  respekterar flaggan när det avfyrar.
- **"Hoppa över om gruppen redan är besvarad":** kollen bor centralt i `setup_alarm_state`
  (efter omschemaläggning) — verifiera att BÅDA vägarna går via den (wakeup som startar appen
  i `app.c`, OCH `wakeup_handler` när appen redan är öppen; den senare missade tidigare kollen).
- **Telefonsvar-synk (`apply_registration` i `config_apply.c`):** en registrering som kom
  in från telefonen uppdaterar dagens (grupp, metric)-slot på plats, annars appendas — samma
  semantik som klockans eget flöde. Timestamp från telefonen: rimlig mot "idag"-kollen.
- DST-alarmet (`SUMMER_TIME_ALARM_ID` 03:31): omschemaläggs allt korrekt vid avfyring?
- Dygnsgränsen: `start_of_today()` använder `localtime`/`mktime` — korrekt över midnatt,
  DST-övergångar, och när klockan byter tidszon?
- "Registrerad idag per grupp-slot"-logiken (`registration_today_for_group_metric`):
  spontana (grupp 0) kan ha flera per dag → senaste vinner; grupp-slots uppdateras på plats.
- Kant: grupp raderas medan dess alarm är schemalagt; metric raderas som har registreringar.

Presentera, planera, STANNA.

### 4. Watch↔telefon-kontraktet (export + config-synk)

Detta är nu ett **tvåvägskontrakt** (companion-appen är byggd, inte längre "kommande"):
klockan → telefon (`data_export.c` streamar config + registreringar) OCH telefon → klocka
(`config_apply.c` applicerar köade ändringar). pkjs (`src/pkjs/index.js`) är bryggan;
nycklarna lever i `package.json` `messageKeys`. Paritetskedjan är fyrdelad:
`messageKeys` ↔ `MESSAGE_KEY_*` (C) ↔ pkjs-payloadnycklar ↔ companion (`ConfigSync`/import).
Tänk på:

- **Export-paritet (klocka→telefon):** `data_export.c` exporterar nu inte bara registreringar
  (alla `Registration`-fält inkl. `group_id`) utan även config (metrics + grupper +
  medlemskap). Skickas alla fält analysen/spegeln behöver? (Känd drift: `group_id`
  tillkom i datamodellen utan att exporten följde med.)
- **Apply-paritet (telefon→klocka):** varje `SET_*`-nyckel pkjs skickar
  (`SET_GROUP_*`, `SET_METRIC_*`, `SET_ALARMS_SUSPENDED`, `SET_REG_*`) hanteras i
  `config_apply.c`, och companion bygger exakt de nycklarna i `buildChangeMessage`. id 0 =
  skapa (klockan allokerar id, speglas tillbaka nästa export — ingen id-mappning).
- **STALE-PKJS-FÄLLAN (bitit två gånger):** om `index.js` eller `messageKeys` ändrats sedan
  senaste `pebble clean` regenereras INTE JS-bundlen — `pebble install` deployar gammal JS.
  Symptom: kön töms aldrig, inga `config:`-loggar. Verifiera att bundlen matchar källan
  (`grep` efter en ny sträng i `build/pebble-js-app.js`); fix = `pebble clean && build`.
- `messageKeys` i package.json ↔ `MESSAGE_KEY_*` i C ↔ nycklarna JS läser ↔ companion — en
  och samma uppsättning i alla fyra.
- Felhantering: utebliven ACK, `app_message_outbox_send`-fel, retry/backpressure i
  chunk-loopen; pkjs vid halvfärdig överföring; partiell pending-batch (acka bara det som
  landade). Gallring efter export (`EXPORT_ACK_THROUGH` → `registrations_prune_synced`).
- Typbredder: `time_t` → int32/uint32, uint16-id:n, uint8-värden — konsekvent på båda sidor
  (särskilt `SET_REG_TIMESTAMP` och effektiv min/max per typ).

Presentera, planera, STANNA.

### 5. Arkitektur & död kod

Konventionerna som håller kodbasen navigerbar. Tänk på:

- Window/logic-splitten (`*_window.c` = UI, `*_window_logic.c` = beteende) — följs den i nya
  fönster, eller har logik smugit in i UI-filer (och tvärtom)? Nya filer sedan sist:
  `trend.c` (dagsaggregering för grafen), `favorites_window.c` (favoritväljare),
  `config_apply.c` (telefon→klocka-apply). Följer de konventionerna? (Menyfönster har logik
  inline by design; `config_apply` är ren beteendekod utan UI.)
- Repository-lagret är enda vägen till persist — inga direkta `persist_*`-anrop utanför
  `repositories/` (undantag: legacy).
- Död kod: `persistance.c`/`.h` (flagga för borttagning), oanvända funktioner
  (`main_window_format_average` — hemskärmsgrafen använder numera `trend.c`, INTE denna, så
  om den fortfarande är oanvänd: flagga den), `config_prune_favorite` (definierad men inget
  delete-flöde anropar den ännu — medvetet för framtiden?), oanvända ikon-getters/resurser,
  utkommenterade block (`deinit`:s tear_downs, `app_glance`).
- Duplicering värd att extrahera (t.ex. status-bar-setup upprepas i varje fönster;
  temafärgs-appliceringen).
- Namngivning/idiom-konsekvens med omgivande kod.

Presentera, planera, STANNA.

### 6. UI-konsekvens: tema, ikoner, plattformar

Appen har mörkt/ljust tema som kan växlas live, och tre skärmstorlekar. Tänk på:

- Temat: varje fönster läser färgerna vid `.load` (eller re-themar i `.appear` om det ligger
  kvar på stacken som huvudfönstret) — inget hårdkodat `GColorBlack/White` i produktvyer.
- Ikonvarianter: allt som ritas på temabakgrund använder `get_icon_by_choice_ex(..., dark?)`
  / `menu_theme_icon_light`; allt på action bars (förgrundsfärgad) använder `get_bar_icon`
  eller `!is_dark`-varianten. Ingen ikon blir osynlig i något tema/markeringsläge.
- Menyer: custom `MenuLayer` + `menu_theme_apply_colors` överallt (ingen kvarbliven
  `SimpleMenuLayer` i produktvyer).
- Layout på 144×168 (diorite/basalt) OCH 200×228 (emery): inga hårdkodade koordinater som
  klipper text/ikoner på den mindre skärmen; `ACTION_BAR_WIDTH`/`STATUS_BAR_LAYER_HEIGHT`
  används istället för magiska tal.
- Registreringsflödet: option-etiketter/ikoner mappar rätt knapp→värde (Upp=högst, Ner=lägst)
  konsekvent mellan bool/3-option; intervall respekterar `[min_value, max_value]`.
- **Hemskärmsgrafen** (`main_window.c` + `trend.c`): sparklines ersätter ikonen först vid
  ≥3 dagars data (`MIN_GRAPH_DAYS`), annars ikonen kvar. Ritas med temats förgrundsfärg
  (ingen hårdkodad färg). Den delade veckodagsaxeln: etikettbredden följer slot-avståndet så
  2-bokstavsetiketterna inte överlappar på 144 px (~17 px/slot) — verifiera visuellt på BÅDE
  144×168 och 200×228 att axeln, raderna (upp till 3 favoriter) och linjen ryms utan att
  klippas. Saknade dagar = glapp i linjen; favorit utan data = tom rad + axel.

Röktesta gärna båda teman i emulatorn som del av passet. Presentera, planera, STANNA.

### 7. Minnesbudget & assets

Aplite dog av `.bss`-overflow — de kvarvarande plattformarna ska inte smyga sig dit. Tänk på:

- `pebble build`-rapportens fotavtryck per plattform: statiskt + resurspack + fri heap.
  Flagga trender (diorite/basalt är 64K-appar; emery har mer). Sätt siffrorna i relation
  till förra körningen om känd.
- Stora statiska arrayer (ikon-cachar, radbuffertar) — växer de med varje ny feature?
- Resurspacket: oanvända bilder i `package.json`/`resources/images` (varje variant × 3
  plattformar kostar flash), dubbletter, för stora källbilder.
- Heap-användning i värsta flödet (Today med många grupper × metrics; registreringsflöde).

Presentera, planera, STANNA.

### 8. Companion: data & synk-integritet

Companion-appen (`companion/`, Kotlin/Room) speglar klockan och gör analysen. Här ÄR
"säkerhet" data-integritet: den får aldrig dubbelräkna, tappa eller korrumpera. Bygg med
`cd companion && ./gradlew :app:assembleDebug`; röktesta på telefonen via `adb`. Tänk på:

- **Room-schema & migrering:** `AppDatabase` kör `fallbackToDestructiveMigration` (pre-release
  OK — klockan omexporterar; FLAGGA när riktiga användare/opruttad historik finns). Entities
  matchar det pkjs levererar; index/primärnycklar stämmer.
- **Idempotent import:** unika nyckeln `(metricId, groupId, timestamp)` + `IGNORE` gör att
  klockans omsändning av hela historiken inte dubblerar. Valens bevaras över import (`byId`
  innan upsert — companion-only metadata skrivs aldrig över av klockan).
- **Pending-kön (telefon→klocka):** enqueue → pkjs pull → apply → ack → delete. Partiell
  batch: acka BARA det som landade (annars tappas ändringar tyst). id-0-skapar speglas
  tillbaka via nästa export (ingen id-mappning). `ConfigSync`-payloads matchar
  `config_apply.c` (pass 4).
- **Health Connect-import:** full-refresh (`deleteByMetricIds` + insert) korrekt när dagens
  totaler växer; syntetiska metric-id 9001–9003 krockar inte med klockans; per-dag-aggregering
  delar `DailyAggregation`-semantik med graf/insikter (annars divergerar siffrorna).
- **Telefonläges-svar:** lagras lokalt direkt OCH köas till klockan; ingen dubbelinsättning
  när klockans export sen speglar tillbaka samma svar.
- **Samtidighet:** DB-arbete på IO-dispatcher, inte main; `runBlocking` i serverns socket-tråd
  är kort/bundet; `StateFlow`/`dataVersion` triggar UI-refresh utan lopp.

Presentera, planera, STANNA.

### 9. Companion: privacy, nätverk & livscykel

Klock-appen har ingen nätverksyta; companion har det. Detta pass handlar om VAD som lämnar
telefonen och att bakgrundskomponenterna beter sig. Tänk på:

- **Dataläckage / egress:** ENDA vägen data lämnar telefonen ska vara Claude-API-anropet
  (`AiAnalyzer`). Allt annat (import, synk, HC, graf, insikter) stannar lokalt. Verifiera att
  inget POSTar utanför loopback utom det anropet. API-nyckeln bor lokalt (`AiSettings`,
  SharedPreferences); **engångs-samtycke** krävs före första anropet; nyckel/data loggas ALDRIG
  (kolla `Log.*`-anrop, inga `println` av payload/nyckel).
- **Claude-anropet:** HTTPS (inte cleartext), timeouts satta, felsvar parsas utan att krascha,
  `anthropic-version`-header + aktuellt modell-id. Nyckeln syns inte i logcat.
- **Health Connect:** endast läsbehörigheter (steps/sleep/heart), rationale-intent-filter finns
  (inkl. Android 14+-varianten), ingen bredare scope än nödvändigt.
- **Loopback-servern (`ImportServer`):** bunden till loopback ENDAST (inte nåbar från nätet),
  parsar en känd request-form; trasig/oväntad input kraschar inte tråden; svarar 404 på okänt.
- **Foreground service (`ImportService`):** `specialUse`-typ + property motiverad; livscykel
  (START_STICKY, stänger servern i `onDestroy`). **WorkManager (`HealthSync`):** periodisk 6h
  + KEEP, en-gångs vid start, no-op när HC saknas/ej beviljad (ingen batteri-/nätverksdrift i
  onödan).
- **Manifest:** behörigheter minimala och motiverade; receivers exporterade rätt
  (`BootReceiver` exported för BOOT_COMPLETED; `ReminderReceiver` INTE exported); PendingIntent
  `FLAG_IMMUTABLE`.
- **Compose-state:** ingen dubbel-send vid recomposition/rotation (busy-guard på
  analys/synk-knappar); state överlever konfigurationsändring rimligt.

Presentera, planera, STANNA.

### 10. Uppdatera docs/minne (sist)

För CLAUDE.md, ROADMAP.md, `design/companion_app_plan.md` och auto-minnet
(`~/.claude/.../memory/`) i fas med verkligheten:

- CLAUDE.md: stämmer arkitekturbeskrivningen, targets, "transitional state"-avsnittet
  (persistance.c-status), bygginstruktionerna? **Saknas ett companion-app-avsnitt** (den är
  numera en stor del av projektet men CLAUDE.md är klock-centrerad)?
- ROADMAP.md + `design/companion_app_plan.md`: bocka av det som gjorts under passen, lägg in
  nya beslut/idéer; håll fas-statusen korrekt.
- Auto-minnet: fånga icke-uppenbara lärdomar från rundan (nya gotchas av AppConfig-klassen,
  stale-pkjs, emulator-flakiness, companion-mönster som bekräftats). Radera minnen som blivit fel.

Presentera ändringarna som diff, STANNA för godkännande, committa.

## Avslutning

Efter alla pass: en kort sammanfattning i klartext — vad som granskades per pass, vad som
fixades + committades, och vad som medvetet lämnades (med motivering). Uppdatera
självkontroll-avsnittets antaganden om något pass visade sig felkalibrerat.
