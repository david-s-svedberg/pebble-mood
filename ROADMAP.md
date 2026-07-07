# Mood — Roadmap

Planerade förbättringar och öppna trådar. Se [CLAUDE.md](CLAUDE.md) för nuläge/arkitektur
och [design/design_notes.txt](design/design_notes.txt) för den ursprungliga datamodellen.

**Status (uppdaterad):** hela den ursprungliga grundplanen (avsnitt 1–6 nedan) är klar,
liksom polish-batchen (tema, timeout, option-text). Kvar är de stora spåren: companion-app,
hälsodata och graf-hemskärmen.

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

### 1. Companion-app (störst payoff)

Klock-sidan av exporten finns (AppMessage → pkjs skickar alla registreringar med metric-id,
namn, typ, värde, timestamp). Kvar: mottagare + analys.

- Leverera datan vidare från pkjs (localStorage → fil/moln/webapp).
- Överblick: historik, trender per metric.
- **Korrelationer** mellan metrics (Joy vs sömn, Anxiety vs träning, ...) — huvudpoängen
  med appen. Kräver "senaste/medel per dag"-semantik som medvetet bor här och inte på
  klockan.
- **Gallring efter export** — klockans persist-budget är begränsad (blobbar chunkas nu
  över 256 B-nycklar, men totalen är ändå ändlig). När companion-appen tagit emot datan
  bör gamla registreringar gallras från klockan (t.ex. behåll senaste ~7 dagar).

### 2. Graf-hemskärm (idé)

Ersätt/komplettera hemskärmen med en enkel graf över senaste dagarnas värden (t.ex. Joy
sparkline). Merparten av analysen bor i companion-appen; klockan visar bara en snabb glimt.
(7-dagars-snittet togs bort från listorna när Today fick "ditt svar" som subtitle —
`main_window_format_average` ligger kvar oanvänd tills grafen/companion tar över.)

### 3. Hälsodata-källor (idé)

Pebbles HealthService kan ge steg, sömn, aktiva minuter, puls. Ge `Metrics` en källa
(manual vs health-*); health-källade metrics fylls i automatiskt och behöver inte visas i
registreringsflödet. Bäst värde ihop med companion-korrelationerna.

## Öppna småtrådar

- [ ] **Visuell röktest på 144×168** (diorite/basalt-emulatorn) — layouten är kodgranskad
      (allt beräknas från bounds/konstanter) men appen har bara synats på emery. En
      engångs-syn när emulatorn beter sig.

- [ ] **Röstdiktering kräver ansluten telefon** — utan telefon får nya metrics/grupper
      defaultnamn. Ev. textfritt/förvalt alternativ.
- [ ] **Fler valbara ikoner** vid behov (pipeline finns: Pillow-script → svart/vit/20px-
      varianter → `IconChoice`).
- [ ] Emery-layouterna kan finputsas ytterligare för den större skärmen.
