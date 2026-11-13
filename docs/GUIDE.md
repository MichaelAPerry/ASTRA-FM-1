# The ASTRA guide

How to play ASTRA on the M-VAVE FM-1: every button and knob, how to read the screen and your horoscope, the astrology behind it, and how to shape the sound.

**New here?** [Start here](QUICKSTART.md) is the five-minute version. Come back to this guide when you want more.

**Contents:** [What ASTRA is](#what-astra-is) · [Buttons](#the-buttons-have-new-jobs) · [Knobs](#the-knobs) · [Screen and lights](#reading-the-screen-and-the-lights) · [First ten minutes](#your-first-ten-minutes) · [Your horoscope](#reading-your-horoscope) · [The astrology](#the-astrology-reading-the-sky) · [The sound](#the-sound) · [Time and memory](#time-and-memory) · [Before you install](#before-you-install)

## What ASTRA is

ASTRA is free, unofficial replacement firmware for the M-VAVE FM-1, a small handheld synthesizer with a square colour screen, 27 keys, buttons and knobs. It replaces the factory software that came with the device. The instrument works out where the Sun, Moon and planets are by itself, offline, for any moment from the year 1800 to 2200. You do not need internet or a phone connection. You can try it first in an emulator on the ASTRA website.

Two words first. "FM" means frequency modulation: one tone wobbles the pitch of another so fast you hear it as a new, richer timbre. An "operator" is one of those tones. ASTRA has ten operators, one per body: Sun, Moon, Mercury, Venus, Mars, Jupiter, Saturn, Uranus, Neptune, Pluto. On an ordinary FM synth you pick an algorithm, a diagram of which operator bends which. In ASTRA the chart draws that diagram: aspects become the connections. When the sky moves, the connections move, and you hear it.

Everything about the planets' positions is real astronomy, calculated on the device. The astrology is symbolic: it shapes the sound and the readings, and it predicts nothing.

## The buttons have new jobs

The buttons still carry the names M-VAVE printed on them (SEQ, GLO, ARP and so on), but ASTRA gives most of them a new job. Go by this table, not by the label.

| Button | What it does in ASTRA |
| :--- | :--- |
| HOME | Go to the wheel, the main page |
| EDIT | Shape the selected planet's tone (four pages) |
| ENV | How the selected planet's notes start and fade |
| LFO | The sky clock speed, plus vibrato and tremolo (two pages) |
| FX | Chorus, echo and reverb (two pages) |
| SEQ | The horoscope, four pages (ASTRA has no sequencer) |
| GLO | The clock, your birth details, chart settings (four pages) |
| PLAY | Pause or restart the moving sky; on the clock page, confirms the time shown is now |
| REC | Brings the sky back to the present; on the birth pages, builds a new sound from your chart |
| SAVE | Press once and the screen asks, press again to save the sound to its slot |
| SEL | Mute or unmute the selected planet |
| ARP | Latch: notes keep sounding after you let go; press the key again to stop it |
| OCT- | Keyboard down an octave |
| OCT+ | Keyboard up an octave |

Pressing the button of the page you are already on moves to its next page. The title at the top of the screen always names the page you are on.

## The knobs

The row of four knobs changes its job depending on which page you are on. Look at the labels drawn at the bottom of the screen; they tell you exactly what each dial does right now. Turning a knob changes the colour of that dial. If you turn it to the very end, the dial flashes.

The table below shows every page and the specific job of each knob when you are on it. The on-screen name is followed by plain words explaining it.

| Page | Knob 1 | Knob 2 | Knob 3 | Knob 4 |
| :--- | :--- | :--- | :--- | :--- |
| **HOME** (The wheel) | INDEX: how hard every bend is | RING: how strong the metallic connections are | ORB: how wide an aspect may be and still count | DAY: move the sky a day at a time |
| **EDIT 1** (Pitch and level) | RATIO: the planet's pitch relative to the note | FINE: fine tune adjustment | LEVEL: the selected planet's loudness | FEEDBACK: how much the planet's tone feeds on itself (grittier) |
| **EDIT 2** (Tone) | WAVE: Air/Fire/Earth/Water sound shape | OUTPUT: how much you hear it directly | DETUNE: slight pitch shift | VELOCITY: how much playing harder matters |
| **EDIT 3** (Free wires) | SOURCE A: which planet bends this one | DEPTH A: how strong that bend is | SOURCE B: second planet bending this one | DEPTH B: strength of second bend |
| **EDIT 4** (Ring and keys) | RING SRC: source for ring modulation | RING: amount of metallic ring effect | KEYSCALE: quieter or brighter up the keyboard | MUTE: silence the selected planet |
| **ENV** (Envelope) | ATTACK: how fast a note starts | DECAY: how long it fades initially | SUSTAIN: level it holds while held | RELEASE: how long it takes to silence |
| **LFO 1** (The sky clock) | WARP: how fast the sky moves (centre is real time) | ORBIT: how much planets swell at returns | WIDTH: stereo width | VOLUME: this sound's volume |
| **LFO 2** (LFO and bend) | LFO RATE: speed of slow modulation | VIBRATO: pitch wobble amount | TREMOLO: volume wobble amount | BEND: range of pitch bend up to 12 semitones |
| **FX 1** (Chorus and echo) | CHORUS: amount of chorus effect | SPREAD: width of the chorus | ECHO: time delay for the echo | REPEAT: how long the echoes keep coming back |
| **FX 2** (Echo and room) | ECHO MIX: balance between dry and wet | SPACE: size of the reverb room | DAMP: make the reverb darker | REVERB: amount of reverb added |
| **SEQ 1-4** (Horoscope) | DAY: move the sky a day | MONTH: move the sky a month | YEAR: move the sky a year | ORB: how close a transit must be to be listed (1-10 degrees) |
| **GLO 1** (Now: clock) | YEAR: current year | MONTH: current month | DAY: current day | HOUR: current hour |
| **GLO 2** (Birth: date) | YEAR: birth year | MONTH: birth month | DAY: birth day | HOUR: birth hour |
| **GLO 3** (Birth: place) | LATITUDE: north is positive, in tenths of a degree | LONGITUDE: east is positive, in tenths of a degree | ZONE: birth time zone | TIME: known or unknown birth time |
| **GLO 4** (Chart systems) | HOUSES: Placidus, Koch, Regiomontanus etc. | ZODIAC: tropical or sidereal system | ZONE: your local time zone for display | (not used) |

### The other four knobs

These four are knobs too, and their jobs stay the same on every page (SELECT aside).

- **SELECT:** Picks a planet (the selected planet gets a white ring on the wheel). On the clock and birth-date pages it sets the minutes. On the horoscope's transits and month pages, it picks which transit to read instead.
- **ALGORITHM:** Picks one of 37 ways to wire the operators. They come in this order: FREE WIRES, then BIRTH H1 to H12, SKY H1 to H12, TRANSIT H1 to H12, and round again.
- **PRESETS:** Steps through 64 saved sounds. A slot you have never saved builds its sound from your chart, in one of four characters: plain, pad (slow), struck (bells and plucks), moving (the sky runs a day a second).
- **MASTER:** Sets the overall volume for all voices. A thin coloured line under the top bar shows its level.

EDIT and ENV always change the planet you chose with SELECT.

## Reading the screen and the lights

Look at the top bar first. The left side shows the page name or a short message after you press something. The right side shows the preset slot number, like P01. A star appears next to it if you have made changes but not saved them yet. Four small bars light up one by one when notes play.

Now look at the button lights. The button for the page you are viewing is lit. **PLAY** lit means the sky is moving. **REC** lit means the sky follows real time. **SAVE** blinks when you have changed a sound but not saved it. **SEL** lit means your selected planet is muted. **ARP** lit means keys keep sounding after you let go. **OCT-** or **OCT+** lit shows the keyboard pitch has shifted up or down.

## Your first ten minutes

1. **Switch on.** Play a few keys. You see the wheel with planets and coloured lines. You hear each key as a full, shifting FM tone. If the screen says "NO BIRTH DATA: GLO" under the wheel, that is expected.

2. **Turn ALGORITHM.** Rotate the knob and listen. The lines on the wheel redraw themselves. The sound shifts from smooth to metallic or gritty. The top bar shows FREE WIRES, BIRTH H1, and so on.

3. **Set the clock.** Press GLO. Use the four knobs to pick your current year, month, day, and hour. Use SELECT for minutes. Press PLAY. The top bar says "CLOCK SET". The lit REC button tells you the sky follows real time.

4. **Enter birth details.** Press GLO again for the date page. Enter year, month, day, hour, and minute. Then press GLO for the place page. Enter latitude, longitude, and time zone. If you do not know your birth time, set TIME to unknown; ASTRA uses noon.

5. **Cast your sound.** While on a birth data page, press REC. You see "CAST FROM THE BIRTH CHART". Play a key to hear the sound of your chart.

6. **Switch to transit mode.** Press HOME to return to the wheel. Turn ALGORITHM left until the top bar says TRANSIT H1. You see two rings of planets. Play a key to hear today's sky bend your birth chart.

7. **Read the horoscope.** Press SEQ. The first page lists transits. The four text lines read the one you have picked. Coloured ">" means getting closer; grey "<" means fading. Turn SELECT to pick different aspects and read their descriptions below.

8. **Move the sky.** Press LFO and turn WARP right: each click is faster, up to five years a second. You hear the sound evolve quickly as months pass in seconds. Press REC to jump back to today. Turn WARP to the centre first, or the sky keeps racing.

9. **Save your sound.** Find a combination of knobs you like. Press SAVE once to ask, then again to save. The bar says "SAVED TO 03" and the star disappears. Your sound stays in that slot, even after you switch off.

## Reading your horoscope

Press SEQ for the horoscope; press it again for each of its four pages.

### Page 1: Transits
Today's sky against your birth chart, closest aspects first. Each row reads: sky planet, aspect symbol, birth planet, a bar (longer = closer to exact), an arrow (coloured ">" coming closer, grey "<" fading) and the days until exact.

Turn SELECT to pick a row. Four lines below read it:
- "MOON CONJUNCT URANUS": the aspect.
- "FEELING MERGES WITH CHANGE": its meaning, in keywords.
- "FROM VOCATION TO VOCATION": where. The first word is the house of your chart the sky planet is crossing, the second the house your birth planet is in.
- "SEPARATING 0°13' EXACT TODAY": how close, and when it is exact.

The DAY, MONTH and YEAR knobs move the sky to any date, past or future. ORB sets how close a transit must be to make the list.

### Page 2: The Month
Thirty days either side of the sky's date (the vertical line in the middle). Each curve is one transit, peaking on the day it is exact.

### Page 3: Birth Chart
Turn SELECT to pick a planet: its sign ("MARS IN ARIES"), a keyword line, its house, its dignity and whether it is retrograde, and the Tarot trumps Crowley gave it and its sign in *777*.

### Page 4: Moon and Signs
The Moon's phase, and your Sun sign, Moon sign and Ascendant (the Ascendant needs a birth time).

Every line says where it comes from: keyword rules built on traditional meanings, not predictions. With no birth data, ASTRA compares the sky with itself and says so. What the words mean: [The astrology](#the-astrology-reading-the-sky), below.

## The astrology: reading the sky

ASTRA rests on the old Hermetic idea "as above, so below": the sky as a mirror of inner life. Astrology reads it as a language with four parts. Planets are *who* acts, signs are *how*, houses are *where* in your life, and aspects are how the planets speak to each other. Every reading on the screen is built from those four, and so is the sound. It is a language of symbols that reflects a moment rather than deciding it.

### The music of the spheres

The oldest link between planets and music is the "music of the spheres": the idea, handed down from Pythagoras, that each planet sounds a tone as it moves, and together they make a harmony too fine for ordinary ears. Johannes Kepler took it up in *Harmonices Mundi* (1619), giving each planet a melody. ASTRA makes the old image literal: each planet is a tone, and the angles between them decide how the tones bend each other.

The bending follows an old ladder. The Chaldean order ranks the seven ancient planets by speed, Saturn the slowest and the Moon the fastest, and in ASTRA the faster always bends the slower. The Sun is the root of every tone. Every other planet sounds like where it stands: Mars in a water sign plays soft, the Moon in a fire sign bright.

### Planets: who

The planets are the actors. Each is also one of ASTRA's ten tones. "On screen" is the word the FM-1 uses for it in readings.

| Planet | On screen | What the tradition sees | 777 trump |
| :--- | :--- | :--- | :--- |
| Sun | THE WILL | the self, vitality, purpose, the light you shine by | XIX, The Sun, "THE LORD OF THE FIRE OF THE WORLD" |
| Moon | FEELING | instinct, memory, mood, the body, tides, the mother | II, The High Priestess, "THE PRIESTESS OF THE SILVER STAR" |
| Mercury | THE MIND | thought, speech, messages, trade, the messenger and trickster (Hermes) | I, The Magus, "THE MAGUS OF POWER" |
| Venus | LOVE | attraction, beauty, value, harmony, pleasure | III, The Empress, "THE DAUGHTER OF THE MIGHTY ONES" |
| Mars | DESIRE | drive, courage, the cutting edge, conflict | XVI, The Tower, "THE LORD OF THE HOSTS OF THE MIGHTY" |
| Jupiter | GROWTH | expansion, faith, generosity, fortune, the great benefic | X, Wheel of Fortune, "THE LORD OF THE FORCES OF LIFE" |
| Saturn | DISCIPLINE | time, limits, structure, endurance, the stern teacher, the great malefic | XXI, The Universe, "THE GREAT ONE OF THE NIGHT OF TIME" |
| Uranus | CHANGE | sudden awakening, rebellion, lightning, invention (discovered 1781) | NONE |
| Neptune | THE DREAM | dissolution, vision, mysticism, the ocean, illusion (1846) | NONE |
| Pluto | POWER | the underworld, death and rebirth, transformation (1930) | NONE |

The old system knew seven wandering stars; Uranus, Neptune and Pluto were found later, so 777 gives them no trump. The Moon, fastest of all, bends almost every other tone. Saturn, slow, is bent more than it bends.

### Signs: how

Each sign has an element and a mode, and both become sound. Fire (spirit, will) gives the bright FIRE wave, earth (matter, form) the hollow EARTH wave, air (mind) the pure AIR sine, water (feeling, the soul) the soft WATER wave. Cardinal signs begin things: a fast attack. Fixed signs hold: a slow one. Mutable signs adapt: in between.

| Sign | On screen | Element | Mode | 777 trump |
| :--- | :--- | :--- | :--- | :--- |
| Aries | ACTS FIRST | fire | cardinal | IV The Emperor "THE SON OF THE MORNING" |
| Taurus | HOLDS STEADY | earth | fixed | V The Hierophant "THE MAGUS OF THE ETERNAL" |
| Gemini | ASKS QUESTIONS | air | mutable | VI The Lovers "THE CHILDREN OF THE VOICE" |
| Cancer | SHELTERS | water | cardinal | VII The Chariot "THE CHILD OF THE POWERS OF THE WATERS" |
| Leo | SHINES | fire | fixed | VIII Strength "THE DAUGHTER OF THE FLAMING SWORD" |
| Virgo | REFINES | earth | mutable | IX The Hermit "THE PROPHET OF THE ETERNAL" |
| Libra | SEEKS BALANCE | air | cardinal | XI Justice "THE DAUGHTER OF THE LORDS OF TRUTH" |
| Scorpio | GOES DEEP | water | fixed | XIII Death "THE CHILD OF THE GREAT TRANSFORMERS" |
| Sagittarius | AIMS FAR | fire | mutable | XIV Temperance "THE DAUGHTER OF THE RECONCILERS" |
| Capricorn | BUILDS SLOWLY | earth | cardinal | XV The Devil "THE LORD OF THE GATES OF MATTER" |
| Aquarius | BREAKS PATTERNS | air | fixed | XVII The Star "THE DAUGHTER OF THE FIRMAMENT" |
| Pisces | DISSOLVES EDGES | water | mutable | XVIII The Moon "THE RULER OF FLUX AND REFLUX" |

On screen, a planet's word and its sign's words make a sentence: Mars in Aries reads "DESIRE ACTS FIRST".

### Houses: where

The houses are the stage: twelve fields of life, counted from the Ascendant, the point of the eastern horizon at your birth. Planets in the four angular houses (1, 4, 7, 10) act most strongly, and in ASTRA they are the loudest in the mix.

| House | On screen | Its field |
| :--- | :--- | :--- |
| 1 | SELF | body, how you meet the world; begins at the Ascendant |
| 2 | RESOURCES | money, possessions, what you value |
| 3 | WORDS | siblings, neighbours, learning, short journeys |
| 4 | HOME | roots, family, the end of matters |
| 5 | PLEASURE | children, creation, romance, play |
| 6 | DAILY WORK | service, health, routine |
| 7 | PARTNERS | marriage, contracts, open rivals; begins at the Descendant |
| 8 | SHARED THINGS | others' resources, inheritance, intimacy, death, the hidden |
| 9 | HORIZONS | long journeys, philosophy, faith, higher learning |
| 10 | VOCATION | career, reputation, public life; begins at the Midheaven |
| 11 | FRIENDS | allies, groups, hopes |
| 12 | SECLUSION | retreat, secrets, the unconscious, self-undoing |

Houses need a birth time. Without one, ASTRA counts them from the Sun's sign ("SOLAR HOUSE"). The house system and the zodiac (tropical or sidereal) are on GLO page 4.

### Aspects: how they speak

Aspects are the angles between planets: the verbs of the language.

- **Conjunction** (0°, MERGES WITH): fusion, a seed. Sound: a strong bend.
- **Sextile** (60°, OPENS TO): opportunity, door ajar. Sound: light touch.
- **Square** (90°, PRESSES ON): friction, pressure. Sound: metallic ring.
- **Trine** (120°, FLOWS WITH): harmony, grace. Sound: smooth bend.
- **Opposition** (180°, PULLS AGAINST): polarity, tension. Sound: a loop.

The orb is how far from exact an angle may be and still count. An applying aspect is building toward its moment and bends a little harder; a separating one has passed it and is fading.

### Strength and direction

Some signs suit a planet and some don't. The BIRTH CHART page says so: "IN ITS OWN SIGN: AT EASE" (at home), "EXALTED: AT ITS BEST" (an honoured guest), "IN DETRIMENT: ILL AT EASE" or "IN FALL: AT A LOSS". A planet at ease plays stronger and richer; one ill at ease plays weaker.

A retrograde planet seems, from Earth, to move backward; tradition reads its force as turned inward, toward review ("RETROGRADE: TURNED INWARD", a red dot on the wheel). In the sound it bends the others inverted, a push turned into a pull.

### Transits and timing

Transits are the moving sky touching your birth chart, like waves on a shore. A return is a planet back at its own birth place: the Moon every month, the Sun every birthday, Jupiter every 12 years, Saturn every 29 (the "Saturn return"). In ASTRA a planet swells as it nears its birth place and, on arrival, feeds back on itself; the ORBIT knob sets how much.

The Moon crosses a point in a few hours: its transits are moods, passing weather. The Sun, Mercury and Venus take days; Mars a few days to weeks; Jupiter and Saturn weeks to months; Uranus, Neptune and Pluto can sit on a point for a year or more, and the tradition counts those among the great turning seasons of a life. On THE MONTH page a fast transit is a sharp peak, a slow one a long plateau.

### Deeper layers

The number after each ALGORITHM name is a harmonic. In the harmonic astrology John Addey developed in the 1970s, multiplying the chart by a number reveals a hidden layer of it: H1 is the chart itself, H5 speaks of creativity and style, H7 of inspiration. Same sky, a new layer, a new wiring.

The MOON AND SIGNS page names the Moon's phase, from NEW MOON to FULL MOON and back: the month's cycle of seed, growth, fullness and release. It is shown, not played.

Aleister Crowley's *777* (1909) is a book of correspondences from the Golden Dawn: it links each planet and sign to a Hebrew letter, a path on the Tree of Life, colours, gods and a Tarot trump. ASTRA shows only the trumps, with Crowley's titles: "777: XVI IN IV" is Mars's trump, The Tower, in Aries's, The Emperor.

### Reading a transit

The transit from the screen, read as an astrologer would:

- **MOON CONJUNCT URANUS**: today's Moon has met your birth Uranus.
- **FEELING MERGES WITH CHANGE**: mood fuses with the urge to break free.
- **FROM VOCATION TO VOCATION**: it happens in your 10th house, career and public life.
- **SEPARATING 0°13' EXACT TODAY**: it peaked today and is fading.

A passing restlessness about your work, an urge to break the routine. Because the Moon moves fast, it is weather for a few hours, not a season. In the sound, the sky's Moon bends your birth Uranus hard, then eases as it separates. The symbols are a mirror, not a fate: they show the shape of a moment and leave its meaning to you.

## The sound

### How the chart plays

Ten planets, ten tones, bending each other. On the wheel, the lines between the planets are the aspects, and each kind of aspect is a different kind of bend:

| Aspect | What you hear | On the wheel |
| :--- | :--- | :--- |
| Conjunction (together) | a strong bend | solid line |
| Trine (120°) | a smooth, medium bend | solid line |
| Sextile (60°) | a light touch | solid line |
| Square (90°) | a metallic, bell-like ring | dashed line |
| Opposition (180°) | the two bend each other: a loop | solid line |

A small circle on a planet means it feeds back on itself, a grittier tone. The closer an aspect is to exact, the harder the bend. The faster planet bends the slower one.

ALGORITHM chooses which chart draws the lines (the top bar shows it):
- **FREE WIRES**: you draw the lines yourself (see Free wiring, below).
- **BIRTH**: your birth chart. It stays the same.
- **SKY**: the sky at the sky clock's date, moving.
- **TRANSIT**: that sky against your birth chart: today's sky bends your chart.

Each comes in harmonics H1 to H12: the same sky, wired another way (see Deeper layers, above).

### Each planet's tone

Each planet's starting tone comes from where it stands:

- **Pitch**: its house counted from the Sun's (the Sun is always 1, the others up to 12).
- **Wave**: its sign's element (AIR pure, FIRE bright, EARTH hollow, WATER soft).
- **Attack**: its sign's mode (cardinal fast, fixed slow, mutable between).
- **Strength**: its dignity. **Loudness in the mix**: angular houses loudest.
- **Place in the stereo field**: where it stands on the wheel, Ascendant left, Descendant right.

These are starting points. Every one of them has a knob.

### Voices

ASTRA plays four notes at once (four voices), each with all ten planets. Play a fifth and ASTRA takes the oldest note you have already let go of; only if all four are held does it cut off the oldest.

### Heard and unheard: LEVEL and OUTPUT

This is the one idea that makes FM make sense. Every planet has two numbers. LEVEL (EDIT page 1) is its strength: how hard it bends the planets it is wired to. OUTPUT (EDIT page 2) is how much of it reaches your ears.

Try it: turn a planet's OUTPUT to 0. You stop hearing it, but it still bends the others, so the sound changes colour instead of disappearing. A planet you hear is a *carrier*; one you only hear through others is a *modulator*. Most are a bit of both. SEL mutes a planet completely: unheard, and bending nothing.

### Pitch and colour

- **RATIO** (EDIT 1): the planet's pitch as a multiple of the note: 1 the note, 2 an octave up, 3 an octave and a fifth, 0 an octave down. Whole numbers sound in tune, like organs and brass. **FINE** adds a fraction: bells and metal. **DETUNE** a few cents: a slow, chorus-like beating.
- **FEEDBACK** (EDIT 1): the planet bends itself. A little buzzes, a lot is noise.
- **WAVE** (EDIT 2): AIR, FIRE, EARTH or WATER.
- **ENV**: on a carrier it shapes loudness; on a modulator, brightness. A modulator that decays fast gives a bright strike that mellows: a pluck or a bell.
- **KEYSCALE** (EDIT 4): quieter up the keyboard, so high notes don't turn harsh. **VELOCITY** (EDIT 2): how much playing harder on a MIDI keyboard matters.

### Big moves

Three knobs on HOME change everything at once. INDEX: all the bends, from soft and pure to bright and wild. RING: all the metallic connections. ORB: how many aspects count, from a few clean wires to a dense web. Vibrato, tremolo and stereo width are on LFO, chorus, echo and reverb on FX: see the knob table.

### Free wiring

Turn ALGORITHM to FREE WIRES, select a planet, and open EDIT page 3: SOURCE A and B choose which two planets bend it, DEPTH A and B how hard (below zero, the bend inverts).

### Four things to try

1. Listen to the chart breathe: ALGORITHM to TRANSIT H1; on LFO, turn WARP to one day a second; press ARP and hold a chord (it keeps sounding). The wires move as aspects form and fade.
2. Calmer or wilder: Lower INDEX and ORB on HOME for a softer, cleaner texture. Raise INDEX, RING, and ORB for more metal and a denser tone. Try a higher harmonic too.
3. A bell in FREE WIRES: Change ALGORITHM to FREE WIRES. Mute every planet but the Sun and the Moon by SELECTing each and pressing SEL. Set the Sun with OUTPUT up, ENV ATTACK at 0, DECAY long, and SUSTAIN at 0. Set the Moon with OUTPUT at 0, LEVEL up, RATIO at 3, FINE a little up, and a short DECAY. Turn SELECT back to the Sun. On EDIT page 3, set SOURCE A to the Moon, DEPTH A up and DEPTH B to 0. On EDIT page 4, set RING to 0. Play a key: it rings like a bell, bright at the strike and mellowing as the Moon's short decay closes.
4. A slow pad: On ENV, raise ATTACK and RELEASE on the planets you hear. Raise CHORUS and REVERB on FX. 

Like it? Press SAVE twice.

## Time and memory

The FM-1 has no battery clock, so it forgets the time when switched off. ASTRA remembers the last time it saved, but can't know how long it was off, so the clock page (GLO) tells you where it stands:

- **NOT SET: A CHART TIME. PLAY SETS IT**: the first start. The sky stands at 1 January 2000, noon, and is never called now.
- **UNCONFIRMED: PLAY IF IT IS RIGHT**: after a power-off. Correct the time if needed and press PLAY.
- **SET AND RUNNING**: the clock is right.

There are two clocks. The real one is on GLO. The sky clock is the date the music and the horoscope use, anywhere from 1800 to 2200: HOME's DAY knob, the SEQ knobs and LFO's WARP move it. REC brings it back to now (turn WARP to the centre first, or it keeps racing).

**Your birth chart stays**, through switching off, with no button to press. ASTRA saves your birth details, chart settings, clock and current slot by itself a couple of seconds after you stop turning knobs.

**Your sound stays only when you press SAVE twice.** Turn PRESETS to another slot without saving and your changes are gone. Slots you have never saved rebuild themselves from your current chart each time you visit them. REC on a birth page rebuilds the current sound from your chart, so save first if you want to keep your version.

The browser emulator has not been checked for any of this: assume it forgets everything when you close the page.

## Before you install

Try ASTRA first in the emulator on the [ASTRA website](https://michaelaperry.github.io/ASTRA-FM-1/). It needs a fast computer; if the sound breaks up, that is the emulator struggling, not the FM-1.

Install from the website in Chrome or Edge with a USB data cable. The installer can also put the official M-VAVE firmware (V15) back. If anything goes wrong, hold OCT- while switching on for USB rescue mode. Only install if you are comfortable with that.

The planet positions are real calculations, accurate to a couple of arcseconds. The astrology is symbolic. ASTRA is free and unofficial, and not made or endorsed by M-VAVE.
