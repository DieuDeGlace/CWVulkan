# User-authored hair mode and tips selection

FINAL / STABLE / HELD. This addendum covers the owner-added creation-interface and hair-widget lines in commits f5a83ec and db92284. The findings below are based on added rendering and pointer behavior. Commit subjects supply no evidence for the coloring of an individual model.

## What switching the mode does

In f5a83ec the creation screen constructs a separate second-color panel. When that widget draws, it shows “Enabled,” “Start” and “Blend,” the current On or Off value, the two percentage values, the “Second color” label, and a tips palette. The added draw path paints the palette and its current selection in either mode. The enabled flag controls hover preview. In Off mode the preview starts and stays at the stored tips color. In On mode a pointer over a valid swatch previews that color. A change of preview color refreshes the preview creature. Moving outside the palette restores the stored tip selection on the next preview update.

Pressing a valid tips swatch sets the stored tips color, turns the gradient On, plays the selection sound and commits the hair colors. A click can therefore enable dual tone directly from Off. A press outside the palette fails the hit test. The separate Enabled control toggles the flag and commits the change. Its added toggle action leaves the stored tip color intact. Start and Blend each have paired controls that request five-percentage-point adjustments. These findings describe the second-color widget when it is displayed. The added actions contain no race-specific exception.

## Swatch feedback

The selected swatch gets an opaque white backing across its 17-by-17 cell. The colored square is inset one unit on all sides, leaving a one-unit white outline. The hit test uses the widget-local pointer position after subtracting the palette origin. The left and top boundaries count, the right and bottom outer boundaries do not, and an internal boundary belongs to the cell on its right or below. The later db92284 additions place the tips palette at local (13, 197), the same origin as the primary palette, and retain calls for drawing, previewing and selecting the tips. The selected swatch outline is distinct from the later black text stroke.

## Frogman markings

The added preview assignments carry both a primary hair color and a tips color into the preview character. Neither commit adds a Frogman-specific single-color rendering rule in the scoped changes. The visible UI and the two stored colors do not establish how a Frogman model colors its markings when the gradient is Off. Whether the markings take the primary color, retain their material color, or receive another treatment remains unresolved.
