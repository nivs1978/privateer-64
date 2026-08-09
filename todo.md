Priority 1: Harbour gameplay loop
- Implement the harbour-entry mini-game from BASIC lines 1260-1490.
- Show the harbour approach briefing, wind direction warning, and start prompt.
- Position the player ship correctly at mini-game start.
- Auto-scroll the harbour upward with difficulty-scaled timing.
- Apply wind gust pushes during the run.
- Detect collisions with harbour ships and remove repair points on impact.
- Detect collisions with harbour walls or missing the entrance and kill the player.
- Return cleanly into the harbour trading screen after a successful run.

Priority 2: Harbour trading and port economy
- Implement the port menu from BASIC lines 1500-1720.
- Randomize port prices for crew, repairs, cannons, grain, and jewel sales.
- Support buying crew, repairs, cannons, and grain with validation and caps.
- Support selling cannons, grain, and jewels with validation.
- Handle Copenhagen prize-crew and prize-money payout on arrival.
- Return from port to the sea map with updated resources.

Priority 3: Cannon battle mini-game
- Replace the current text-only cannon fight with the full shooting mini-game from BASIC lines 1950-3090.
- Load and show the aiming/battle picture, enemy ship graphic, and crosshair.
- Implement distance, wind strength, wind direction, elevation, and side aim controls.
- Animate cannon fire, splashes, and hit effects.
- Apply ship damage, enemy damage, cannon loss, crew loss, and repair loss using the BASIC rules.
- Support aborting the fight and returning to the encounter choice flow.

Priority 4: Boarding battle mini-game
- Replace the current simplified boarding loop with the full boarding flow from BASIC lines 3100-3240.
- Show boarding presentation/animation and repeated boarding rounds.
- Apply losses to both crews using the original difficulty-based formulas.
- Offer continue-or-retreat choices after each round.
- Flow into surrender resolution when the enemy crew breaks.

Priority 5: Surrender, prize capture, and encounter aftermath
- Complete the post-battle resolution from BASIC lines 3000-3020 and related encounter logic.
- Award rigsdaler, grain, surviving enemy crew, and points correctly.
- Offer sink-versus-prize choice with prize crew cost.
- Track prize ships sent to Copenhagen and pay out later in harbour.
- Clear or update the map state for defeated encounters.

Priority 6: Promotion and progression systems
- Implement flaskepost promotion flow from BASIC lines 6100-6119.
- Track the naval title ladder: skibsfører, kaptajn, kommandørkaptajn, kommandør, admiral.
- Track the noble title ladder used by the ending: Borger, BARON, GREV, KAMMERHERRE, KRONPRINS.
- Recreate the point goal and turn-limit progression for each rank.
- Show the promotion text, next-goal text, and final victory text with titles.

Priority 7: End states and rule-complete resource checks
- Implement the remaining loss/win rule checks from BASIC lines 5000-6500.
- Sink the ship when money, crew, cannons, or grain exceed their weight limits.
- Handle low-repair or low-crew game-over paths consistently.
- Keep point totals, grain consumption, and turn counting aligned with the BASIC rules.

Priority 8: Auxiliary systems from the original game
- Implement the F1 help system from BASIC lines 8000-9140, including the topic menu and example screens.
- Implement the F2 sound toggle behavior everywhere it matters.
- Implement escape/quit flow and end-of-game high-score handling.
- Recreate any remaining intro/loading/status messages needed for full parity.

Notes from BASIC analysis
- The highest-priority blockers are the harbour mini-game and harbour trading, because the main map loop cannot progress into the original gameplay without them.
- The combat systems are partly scaffolded in assembly already, but they still need the real graphics, controls, and original outcome rules to match the BASIC game.
- Promotions and noble titles depend on the progression loop, so they should come after harbour and combat are playable.
