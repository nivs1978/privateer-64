#todo
- Move pescii graphics screens to binary files that can be loaded into screen memory to minimize basic code.
- WHen ship moves on map, all variables/points are drawn, but this should be split so that only what has changed is updated to speed up drawing.
- The map has 29 columns in the internal grid so we skip one in the code. We should simply remove this column from the grid instead.
