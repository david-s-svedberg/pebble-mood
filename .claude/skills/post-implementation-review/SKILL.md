---
name: post-implementation-review
description: Grundlig post-implementation-review av Mood (native C Pebble-app + PebbleKit JS-export). Passen i ordning - 1 Minnessäkerhet & resurslivscykler, 2 Persistens & dataintegritet, 3 Alarm & tidslogik, 4 Export/pkjs-kontrakt, 5 Arkitektur & död kod, 6 UI-konsekvens (tema/ikoner/plattformar), 7 Minnesbudget & assets, 8 Uppdatera docs/minne. Körs efter en längre implementationsrunda; stannar för godkännande mellan varje pass.
---

# Post-implementation review — Mood

En sekvens fokuserade granskningspass efter en längre implementationsrunda. Varje pass läser
hela kodbasen genom EN lins, presenterar fynd och STANNAR för godkännande innan något fixas.
Ordningen är efter allvarlighetsgrad för just den här appen: minnesfel kraschar klockan (har
hänt: double-free, window-läckor), persistensfel förstör användarens data, tidsfel gör att
alarm uteblir — därefter kontrakt, struktur, utseende, budget och sist dokumentation.

Detta är en **inbyggd C-app för Pebble** (SDK 4.x, targets diorite/basalt/emery) utan
nätverk, auth eller multi-tenancy — klassiska webbsäkerhetspass är medvetet utelämnade.
"Säkerhet" här ÄR minnessäkerhet och dataintegritet. Det finns **ingen automatisk testsvit**:
verifiering av fixar sker via `pebble build` (alla tre plattformar) + röktest i
emery-emulatorn, och vid behov på den fysiska klockan.

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

- **Produktkod (full granskning):** `src/c/**` (appen), `src/pkjs/**` (telefonsidan av
  exporten), `package.json` (resurser/messageKeys/targets), `wscript` (bygget).
- **Genererade assets (lägre ribba):** `resources/images/**` — granska att rätt varianter
  finns och används (svart/vit, storlekar), inte pixlarna i sig.
- **Verktyg (lägre ribba):** `.vscode/`, `.devcontainer/`.
- **Granskas aldrig:** `build/`, `.claude/`, `design/` (historiska anteckningar).
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
- DST-alarmet (`SUMMER_TIME_ALARM_ID` 03:31): omschemaläggs allt korrekt vid avfyring?
- Dygnsgränsen: `start_of_today()` använder `localtime`/`mktime` — korrekt över midnatt,
  DST-övergångar, och när klockan byter tidszon?
- "Registrerad idag per grupp-slot"-logiken (`registration_today_for_group_metric`):
  spontana (grupp 0) kan ha flera per dag → senaste vinner; grupp-slots uppdateras på plats.
- Kant: grupp raderas medan dess alarm är schemalagt; metric raderas som har registreringar.

Presentera, planera, STANNA.

### 4. Export & pkjs-kontraktet

Klockan skickar registreringar till telefonen via AppMessage (`data_export.c` ↔
`src/pkjs/index.js`, nycklar i `package.json` `messageKeys`). Companion-appen (kommande, se
ROADMAP) blir konsument — kontraktet måste vara komplett och synkat. Tänk på:

- Fältparitet: exporterar C-sidan ALLA fält i `Registration` som analysen behöver? (Känt
  exempel på drift: `group_id` lades till i datamodellen utan att exporten uppdaterades.)
- `messageKeys` i package.json ↔ `MESSAGE_KEY_*` i C ↔ nycklarna JS läser — samma uppsättning.
- Felhantering: utebliven ACK, `app_message_outbox_send`-fel, retry/backpressure i
  chunk-loopen, JS-sidan vid halvfärdig överföring.
- Typbredder: `time_t` → uint32, uint16-id:n — konsekvent på båda sidor.

Presentera, planera, STANNA.

### 5. Arkitektur & död kod

Konventionerna som håller kodbasen navigerbar. Tänk på:

- Window/logic-splitten (`*_window.c` = UI, `*_window_logic.c` = beteende) — följs den i nya
  fönster, eller har logik smugit in i UI-filer (och tvärtom)?
- Repository-lagret är enda vägen till persist — inga direkta `persist_*`-anrop utanför
  `repositories/` (undantag: legacy).
- Död kod: `persistance.c`/`.h` (flagga för borttagning), oanvända funktioner (t.ex.
  `main_window_format_average` sedan Today visar svar istället för snitt — behålls den
  medvetet för graf-hemskärmen?), oanvända ikon-getters/resurser, utkommenterade block
  (`deinit`:s tear_downs, `app_glance`).
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

### 8. Uppdatera docs/minne (sist)

För CLAUDE.md, ROADMAP.md och auto-minnet (`~/.claude/.../memory/`) i fas med verkligheten:

- CLAUDE.md: stämmer arkitekturbeskrivningen, targets, "transitional state"-avsnittet
  (persistance.c-status), bygginstruktionerna?
- ROADMAP.md: bocka av det som gjorts under passen, lägg in nya beslut/idéer.
- Auto-minnet: fånga icke-uppenbara lärdomar från rundan (nya gotchas av
  AppConfig-klassen, mönster som bekräftats). Radera minnen som blivit fel.

Presentera ändringarna som diff, STANNA för godkännande, committa.

## Avslutning

Efter alla pass: en kort sammanfattning i klartext — vad som granskades per pass, vad som
fixades + committades, och vad som medvetet lämnades (med motivering). Uppdatera
självkontroll-avsnittets antaganden om något pass visade sig felkalibrerat.
