# Mood — Roadmap

Planerade förbättringar och öppna trådar. Se [CLAUDE.md](CLAUDE.md) för nuläge/arkitektur
och [design/design_notes.txt](design/design_notes.txt) för den ursprungliga datamodellen.

Status: Steg 1–4 + dataexport-grund + UI-buggfix är klara (se git-historiken). Nedan är
det som återstår, grovt prioriterat men inte hugget i sten.

---

## 1. Färdig grunduppsättning (seed-data)

Vid första start ska appen seedas med användbara grupper + metrics. Riktlinjer:

- **Max 3 metrics per grupp** som standard (ska vara snabbt att fylla i).
- Resten av standard-metricsen seedas som **o-inkopplade** (ingen grupp) men finns redo, och
  användaren ska **smidigt kunna koppla in dem** i befintliga grupper.

Förslag på default:

**Grupp "Morning" (~07:30)** — humör, sömnkvalitet, utvilad
**Grupp "Evening" (~21:30)** — humör, tränat?, stressnivå
**O-inkopplade (finns men ej i grupp)** — humör (för spontan), sjuk, medicin tagen,
sömntimmar, kost, alkohol, koffein, tid utomhus, social kontakt, produktiv dag, skärmtid,
smärta/huvudvärk, ångest

(Humör finns medvetet i båda grupperna — kräver many-to-many, se avsnitt 5.)

Designnotis: seedas hårt i koden, gatat på `first_start`, plus ev. "återställ till standard".

---

## 2. Svarsmodell (typer, knappar, ikoner) — NÄSTA STEG

Metrictyper och hur de besvaras:

- **Bool (2 alternativ)** — besvaras med **Upp/Ner**. Default-ikoner: **bock / kryss**.
- **3 alternativ** — **egen typ**, besvaras med de **tre vänstra knapparna** (Upp/Select/Ner).
  Default: emoji-/ansiktsikoner (🙁 / 😐 / 🙂). Egen typ just för att man direkt ska kunna gå
  in och **ändra ikon och text per alternativ**.
- **Interval (0..max)** — behålls för finare skalor, stegas med Upp/Ner.

Gemensamt för alla typer:
- Användaren kan välja **metricens namn**.
- Användaren kan välja **redigerbar ikon + text per alternativ** (gäller både 2- och
  3-alternativsmetrics).
- Användaren kan välja en **huvudikon** som visas **stort i mitten** av registrerings-
  skärmen för snabb igenkänning.

Status:
- [x] Datamodell (`Metrics`: `main_icon`, `option_icons[3]`, `option_text_ids[3]`).
- [x] `IconChoice`-register + ikon-väljare i metric-config (huvudikon + per alternativ).
- [x] Registreringsfönstret mappar knappar per typ (2→Upp/Ner, 3→Upp/Select/Ner) och ritar
      huvudikonen centrerat.
- [x] **Text per alternativ** — redigerbar via röst i metric-config ("Option N text"-rad),
      lagras i `option_text_ids`. Sträng-id:n börjar nu på 1 så 0 = "ingen text". (Visas ännu
      inte på registreringsskärmen — det görs i ikon-/registrerings-polishen.)
- [ ] **Ikon-assets + storlekar** (se nedan).

### Ikoner: fler + passande storlekar

- **Lägg till fler valbara ikoner** (sömn, mat, alkohol, kaffe, sol/utomhus, stress, smärta,
  hjärta, etc.) så metrics kan få igenkännbara symboler. Default för 3-svar bör vara riktiga
  emoji-/ansiktsikoner (🙁 / 😐 / 🙂) — finns ej än, placeholders ▲/✓/▼ används nu.
- **Storlekar per användningsområde.** Pebble har **ingen runtime-skalning** av bitmaps, så
  varje ikon behöver levereras i rätt storlek(ar):
  - **Action bar / option-ikoner + settings-rader:** liten (~20–25 px).
  - **Huvudikon på registreringsskärmen:** stor (~50 px), centrerad.
  - Mappa varje `IconChoice` till sina storleksvarianter och välj rätt variant beroende på
    var den ritas. Idag visar settings bara *namnet* för stora ikoner (mood/exercise 50px) för
    att undvika överflöd; med en liten variant kan vi visa en snygg förhandsvisning även där.
- Option-ikoner begränsas redan till små ikoner (de stora hoppas över vid cykling) så action
  baren inte svämmar över.

---

## 3. Spontan registrering + quick launch

- Registrera en metric **spontant** (framför allt humör) utan att vänta på alarm — appen tänkt
  som **quick launch** på klockan, så få knapptryck som möjligt till sparad registrering.
- Spontana registreringar räknas som "besvarad idag" för respektive metric (se avsnitt 4).

---

## 4. Smarta alarm (hoppa över redan besvarat) — KLART

Status: [x] implementerat och verifierat på emery. Gruppalarm frågar bara metrics som
inte redan registrerats idag; om hela gruppen är besvarad visas inget alarm (ingen
vibration) och appen avslutas direkt. Kvar: använd samma "registrerad idag"-koll för den
riktiga "Idag"-vyn (Upp-knappen, avsnitt 6).


- När en grupps alarm triggas ska metrics som **redan registrerats idag** (t.ex. spontant)
  **inte** visas i registreringsflödet.
- Om **alla** metrics i gruppen redan är besvarade för dagen ska alarmet **inte visas alls** —
  inget UI och ingen vibration.

Implementationsnotis: kräver en "registrerad idag?"-koll per metric (jämför `time_stamp` mot
dagens datumgräns i lokal tid). Påverkar både registreringsflödet och beslutet att visa/vibrera.

---

## 5. Relationer + o-schemalagda metrics (datamodell)

- **En metric ska kunna ingå i flera grupper** (many-to-many). Alltså **inte** "välj grupp per
  metric" — istället **"välj vilka metrics som ingår i en grupp"**.
- En metric som inte ingår i någon grupp är **o-schemalagd** (registreras bara spontant, t.ex.
  "Sjuk"). Ska ingå i default-uppsättningen.

Implementationsnotis: ersätt dagens enkla `group_id`/`group`-fält på `Metrics` med en
**membership-relation** — t.ex. en join-post (`group_id`, `metric_id`) backad av en egen
`DynamicData` i metrics_repository. "Ingen grupp" blir då helt enkelt avsaknad av
membership-poster (ta bort dagens implicita `group_id = 0`).

---

## 6. Ny huvudskärm (custom UI istället för lista)

Status: KLART.
- [x] Custom hemskärm (titel + nästa tid + action bar) istället för lista.
- [x] **Select** → spontanregistrera (metric-lista → välj → registrera en metric). Avsnitt 3.
- [x] **Ner** → Settings.
- [x] **Upp** → "Idag"-vyn: schemalagda metrics + spontant registrerade, med bock för de som
      besvarats idag; Select registrerar/ändrar svaret. 7-dagars­snittet visas som undertext.

Eget UI istället för dagens lista:
- **Ner** → Settings (config-menyn).
- **Select (mitten)** → spontanregistrera en metric (avsnitt 3).
- **Upp** → **"Idag"-vy**:
  - Visa alla metrics som är **schemalagda för idag**, med indikation om de **är besvarade
    eller ej**.
  - Visa även metrics som användaren **spontant registrerat** och som inte ingår i någon grupp.
  - Tillåt att **registrera direkt** därifrån, och att **ändra ett redan givet svar** på en
    redan besvarad metric.

7-dagars-snittet (nuvarande hemskärm) flyttas till en egen vy (ej tas bort).

---

## Öppna trådar / polish (från kodgenomgången)

- [ ] **"App Config" → Theme**: visar Dark/Light men callbacken är `NULL` — temaväxling ej
      inkopplad (`config_toggle_theme` finns men anropas inte).
- [ ] **"App Config" → Alarm timeout**: `tick_alarm_timeout` är en tom stub — timeout går
      inte att ändra via UI (`config_set_alarm_timeout` finns).
- [x] **Alarmrubriken** visar nu gruppens namn (t.ex. "Morning") istället för "Take Your Meds",
      med fallback "Time to register".
- [ ] **Companion-app saknas** — exporten skickar registreringar till telefonen, men ingen
      mottagande app/analys finns än. Nästa: leverera datan vidare + bygg korrelations-UI.
- [x] **"Max visas för BOOL"** — löst: metric-config bygger nu raderna dynamiskt per typ, så
      Max-raden finns bara för Interval.
- [ ] **Re-registrering / dubbelräkning i snittet** — registreringar är tidsstämplade datapunkter
      och flera per dag är giltiga (t.ex. humör morgon + kväll, samma metric i två grupper). Att
      "ändra" via Idag-vyn lägger därför till en ny punkt snarare än att skriva över. 7-dagars­snittet
      medelvärdar alla punkter. Rätt "senaste/medel per dag"-semantik (och ev. grupp-kontext på
      registreringar) hör hemma i analys-/companion-lagret — medvetet inte ändrat i klockan nu.
- [ ] **Snooze-tidsrad** — alarmets tidsrad visar gruppens registreringstid (meningsfullt även för
      en snooze); bedömt som ej-bugg.
- [ ] **Röstdiktering kräver ansluten telefon** — utan telefon får nya metrics/grupper
      defaultnamn. Ev. erbjuda ett textfritt/förvalt alternativ.
- [x] **edit-alarm tidsindikator för emery** — centrerad i innehållsytan + större font
      (BITHAM-42 på emery, GOTHIC-28 på 144px). Övriga layouter kan fortfarande ses över för
      emerys större skärm.
- [x] **`Alarm.index`-sentinels fixade.** `SNOOZED_ALARM_ID`/`SUMMER_TIME_ALARM_ID` är nu
      255/254 istället för -1/-2. `index` är en `uint8_t` och användes som wakeup-cookie; de
      negativa värdena wrappade till 255/254 i lagringen men jämfördes som int (-1/-2) → snooze/
      DST-wakeups routades fel. Genom att definiera om konstanterna till de uint8-värden de redan
      lagrades som matchar jämförelserna nu — utan struct-ändring eller migration (lagrad byte är
      oförändrad). Grupp-id:n håller sig under 254.
- [x] **Wakeup-id persisteras nu.** `schedule()` sparar gruppen (eller configen för snooze/DST)
      efter att `wakeup_id` satts, så appen kan fråga sina egna wakeups efter omstart (hemskärmens
      "nästa tid" blir korrekt istället för "None").

---

## Förslag på ordning

1. **Avsnitt 2 (svarsmodell + ikoner)** — påverkar datamodellen, görs först. ← pågår
2. Avsnitt 5 (many-to-many + o-schemalagda) — datamodell.
3. Avsnitt 1 (seed-data) — bygger på 2 och 5.
4. Avsnitt 3 (spontan) + 6 (ny huvudskärm) — hänger ihop (Select = spontant, Upp = Idag).
5. Avsnitt 4 (smarta alarm) — kräver "registrerad idag"-kollen.
6. Polish-trådarna löpande; companion-appen som eget spår.
