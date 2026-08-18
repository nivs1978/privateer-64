Priority 1: Harbour gameplay loop
- Implement the harbour-entry mini-game from BASIC lines 1260-1490 (currently intentionally skipped: entering a harbour tile jumps straight to the trading menu).
- Show the harbour approach briefing, wind direction warning, and start prompt.
- Position the player ship correctly at mini-game start.
- Auto-scroll the harbour upward with difficulty-scaled timing.
- Apply wind gust pushes during the run.
- Detect collisions with harbour ships and remove repair points on impact.
- Detect collisions with harbour walls or missing the entrance and kill the player.
- Return cleanly into the harbour trading screen after a successful run.

Priority 2: Cannon battle mini-game fidelity
- Factor distance (shoot_dist_lo/hi, BASIC IAFST) into the hit formula in resolve_shot_round - it is generated/displayed but currently unused.
- Add a multi-splash miss animation (BASIC 2650-2740 ICKX/ICKY array) instead of the single hit/miss marker.
- Rework the cannon and boarding damage formulas to use IDIF/IKAMP/ILOSE-equivalent ratios (difficulty_level and encounter/flee counters) instead of flat random rolls, matching BASIC lines 2600-2940 and 3120-3160.

Priority 3: Boarding battle mini-game presentation
- Add the boarding presentation/animation from BASIC lines 3100-3240 (currently a text-only status screen with no graphic/animation).

Priority 4: Encounter aftermath cleanup
- Clear/update the map cell for a defeated encounter (BASIC sets IMAP(IX/10,IY/10)=50 on both surrender and sinking); no equivalent exists yet, so defeated ships can reappear at the same tile.

Priority 5: End states and rule-complete resource checks
- Sink the ship when treasury/crew/cannons/grain exceed their weight limits (BASIC lines 960-963, 6200-6490: rigsdaler>30000, crew>500, cannons>150, grain>700) - not implemented yet.
- Add the grain upkeep/consumption drain per turn (BASIC line 1175: KORN=KORN-(MAND*(IDIF/800))) - grain currently only changes via harbour trade and loot.

Priority 6: Auxiliary systems from the original game
- Implement the F1 help system from BASIC lines 8000-9140, including the topic menu and example screens.
- Implement the F2 sound toggle behavior everywhere it matters.
- Implement escape/quit flow and end-of-game high-score handling.

Notes from BASIC analysis
- Harbour trading/economy (BASIC 1500-1720), the promotion/title/progression system (BASIC 6100-6119 and line 102), and the surrender/prize/aftermath flow (BASIC 3000-3020) are fully implemented and verified against KAPER.BAS.
- The harbour-entry sailing mini-game (Priority 1) and the auxiliary systems (Priority 6) are the only areas with zero implementation; everything else remaining is a fidelity/polish gap on already-working systems.
- Combat (cannon + boarding) is playable end-to-end but uses simplified damage math rather than BASIC's difficulty/streak-based formulas - closing that gap is Priority 2/3.
