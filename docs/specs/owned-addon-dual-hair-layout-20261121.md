# User-authored dual-tone hair creation layout

FINAL / STABLE / HELD. Evidence is confined to the added UI lines in owner commit db92284, in the character creation interface and its style widget.

## Panel placement and fit

The creation interface requests a 300-by-330 panel for both the character style and second-hair controls. It measures the style panel's content size and uses that measurement for both panels. The pair is intended to sit to the left of the character. Width and height are converted to logical view units by dividing the incoming dimensions by the interface scale. The left margin is 20 units and the nominal gap between panels is 10.

For the width calculation, the available span is half the logical view width minus 40 units. The nominal pair width is two measured panel widths plus 10. Dividing the available span by the nominal pair width gives the width fit, capped at one. Before applying any height constraint, the layout checks the anticipated right edge of the pair plus 10 units against the nearer left edge of the name edit control and the Create button. Both control positions are converted to logical units. If the anticipated edge extends strictly past that nearer control, the layout reserves 80 units at the bottom. Otherwise it reserves 20. Exact equality keeps the 20-unit reserve.

The height calculation reserves 20 units plus the Back button's measured height at the top, then another 10 units, then the selected footer reserve. The remaining logical height divided by the measured panel height gives the height fit. The final fit is the lesser of the width and height fits, with a floor of 0.01. The raw placement anchor supplied for both panels is the left edge at the full incoming view height. Both top offsets are negative by the sum of the footer reserve and the fitted panel height. The primary panel starts 20 logical units from the left. The secondary panel starts a further fit times the sum of one measured panel width and the 10-unit gap to its right.

For an illustration using 300-by-330 measured content, a logical width of 1,300 allows full width fit. With enough height, the panels begin at X 20 and 330. At that width the larger footer reserve is selected if the nearer footer control begins before X 640. At a logical width of 800, width fit is about 0.590 and the reservation cutoff becomes X 390. With enough height the secondary panel begins at about X 203. Limited height can reduce the final fit in either case. A very small viewport can reach the 0.01 minimum. The additions directly establish the computed fit, anchor and offsets. How the widget applies scale to every painted element and handles an impossibly small viewport is outside the added lines.

## Labels, palette and pointer behavior

The secondary widget draws the title “Dual-tone hair” at local (15, 27). It draws “Second color” at Y 182, using half the content width as the horizontal text anchor. The two groups of repeated-row positions visible in the additions start at Y 42 and Y 57 respectively and advance by 30 per row. The added lines leave the repeated-row labels, row count, exact text alignment and action attached to each row unspecified. An explicit text-stroke color is opaque black. The additions do not establish a border color, thickness or shape around the selected palette swatch.

The palette constants give local X 13, Y 197 and a 17-unit cell size. Both widgets use the same Y origin for palette creation, drawing and pointer lookup. The primary palette displays the current hair color. The secondary palette displays the gradient tips color. Each supplies a temporary preview color to the pointer lookup and separately tests whether a lookup succeeded for its current selection. The palette is wired to a pointer-pressed event. The coordinate-to-cell mapping, treatment of exact boundaries or outside clicks, palette dimensions, and what the handler does after a successful lookup are outside the additions.

## Race and mode conditions

Commit db92284 adds a secondary panel and names the tips color it can edit. The authorized lines contain no race comparison, dual-tone toggle condition or test governing the visibility of the panel or its color controls. They cannot establish whether a Frogman sees the panel, retains a single-color appearance, or can change a second color. They also leave any model- or style-specific control enablement unresolved.
