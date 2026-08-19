Priority 1: Cannon battle mini-game fidelity
- Factor distance (shoot_dist_lo/hi, BASIC IAFST) into the hit formula in resolve_shot_round - it is generated/displayed but currently unused. BASIC 2740 derives IAFV from IAFST, elevation and wind, and 2860-2880 uses its sign to decide short/over/hit.
- Add the wind deflection and difficulty tolerance that decide where the shot lands: IRET (BASIC 2630, from wind strength, wind direction and aim) tested against IGRP (2780-2790, tolerance shrinks from 2 to 0 as IDIF rises) at 2800-2830.
- Add a multi-splash miss animation (BASIC 2650-2740 ICKX/ICKY array) instead of the single hit/miss marker; the splash count derives from the damage actually taken that round.
- Add the enemy ship sinking animation (BASIC 3030-3070) for the IAKTREP<15 outcome - the ship graphic is redrawn progressively lower until it disappears.
- Rework the cannon and boarding damage formulas to use IDIF/IKAMP/ILOSE-equivalent ratios instead of flat random rolls, matching BASIC 2600-2940 and 3120-3160. IKAMP (encounters entered, 1840/2440) and ILOSE (times fled, 1890) are not tracked at all yet.

Priority 2: Boarding and surrender presentation
- Add the boarding approach animation (BASIC 3350-3400): both ships are drawn, then the player's ship is stepped upward toward the enemy until they meet. Guarded by IBIGL/IBOARD at 3110 so it plays once per encounter and only when the gunnery screen was entered first.
- Add the striking of the colours on surrender (BASIC 1990-2020 and 2950-2990): a flagpole carrying the enemy ensign - Union Jack (JACK) for the English ships, Jolly Roger (JOLLY) when IIF=8 - with the flag sliding down the pole before being erased. BASIC only draws the pole when IDIF>4 (1970).

Priority 3: End states and rule-complete resource checks
- Sink the ship when treasury/crew/cannons/grain exceed their weight limits (BASIC 960-963 and 6200-6490: rigsdaler>30000, crew>500, cannons>150, grain>700), each with its own cause-of-death line.
- Add the grain upkeep/consumption drain per turn (BASIC 1175: `KORN=KORN-(MAND*(IDIF/800))`, and when it hits zero you lose a tenth of the crew). Grain currently only changes via harbour trade and loot.

Priority 4: Sound
- No in-game audio exists; the SID is only touched by the intro tune in intro.inc. BASIC has effects for running aground (1225), harbour collision (1730), fleeing (1890), cannon fire (2260-2270), shot splashes (2680), the fog event (2470), boarding (3100), enemy surrender and sinking (2950/3050), death (1755-1780) and promotion (6100).
- Implement the F2 sound toggle (BASIC 3430) once there is audio to toggle, including the startup sound prompt (83-88).

Priority 5: Auxiliary systems from the original game
- Implement the F1 help system from BASIC 8000-9140, including the ten-topic menu and the worked gunnery example screen (8510-8550, 9120-9140).
- Implement the escape/quit flow (BASIC 1060) and end-of-game high-score handling with the persisted record holder (42-45, 1800-1809).

Notes from BASIC analysis
- Harbour trading/economy (BASIC 1500-1720), the promotion/title/progression system (BASIC 6100-6119 and line 102), and the surrender/prize/aftermath flow (BASIC 3000-3020) are implemented and verified against KAPER.BAS, except for the specific gaps listed above.
- The harbour-entry sailing mini-game (BASIC 1260-1490) is complete: briefing with wind warning, ship placement, scrolling channel with difficulty-scaled speed and obstacle count, difficulty-scaled and randomly placed entrance, wind gusts, sprite/background collision damage, a fatal miss at the entrance, and a clean return into the trading menu.
- Deliberate deviation: BASIC 1390-1400 prints the entrance gap on two rows (21 and 22); we draw one row. Sprite/background collision cannot tell the wall from an obstacle the way BASIC's POINT colour test can, and a two-row wall would shift which step the fatal check lands on. Revisit only if wall hits ever need to be distinguished from ship hits.
- Combat (cannon + boarding) is playable end-to-end but is text-only and uses simplified damage math rather than BASIC's difficulty/streak-based formulas - Priorities 1 and 2.
- Sound and the auxiliary systems (Priorities 4 and 5) are the areas with zero implementation.
