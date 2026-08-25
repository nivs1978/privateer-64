Original game in KAPER.BAS

Priority 1: Cannon battle mini-game fidelity
- Rework the cannon and boarding damage formulas to use IDIF/IKAMP/ILOSE-equivalent ratios instead of flat random rolls, matching BASIC 2600-2940 and 3120-3160. IKAMP (encounters entered, 1840/2440) and ILOSE (times fled, 1890) are not tracked at all yet.

Priority 2: Boarding and surrender presentation
- Add the boarding approach animation (BASIC 3350-3400): both ships are drawn, then the player's ship is stepped upward toward the enemy until they meet. Guarded by IBIGL/IBOARD at 3110 so it plays once per encounter and only when the gunnery screen was entered first.

Priority 3: Auxiliary systems from the original game
- 
- Implement the F1 help system from BASIC 8000-9140, including the ten-topic menu and the worked gunnery example screen (8510-8550, 9120-9140).
- Implement the escape/quit flow (BASIC 1060) and end-of-game high-score handling with the persisted record holder by saving to file (42-45, 1800-1809).
