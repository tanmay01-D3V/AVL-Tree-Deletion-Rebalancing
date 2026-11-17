# AVL Studio — Design Direction

## Product and audience

AVL Studio serves students and instructors learning self-balancing binary
search trees, and demonstrates the same data structure in a reservation
workflow. Its two server-rendered pages share one visual language:

- **Visualizer:** change a tree, inspect its shape and balance, and follow
  actual C++ operation events.
- **Reservations:** manage reservation records indexed by the same C++ AVL
  implementation, inspect the live ID tree, and review server-side history.

The application uses C++ for routing, data processing, AVL operations, HTML
generation, and SVG generation. Forms submit to C++ routes; CSS supplies all
presentation and responsive behaviour. There is no JavaScript client.

## Visual concept

Treat the pages like an engineering field notebook opened on a live instrument
panel: warm paper surrounds a deep green-black tree stage, with restrained
citrus and coral signals for operations and rebalancing. Use generous
editorial spacing, fine rules, compact technical labels, and a clear visual
distinction between explanatory content and the interactive work surface.

## Colour palette

- Paper: `#F4F1E8`
- White card: `#FFFEFA`
- Ink: `#182722`
- Muted ink: `#718078`
- Deep forest stage: `#172923`
- Citrus action: `#D9F36A`
- Coral event: `#F27D5B`
- Quiet border: `#DCDDD2`
- Tree connector: `#61736A`

Use citrus for primary actions and current-node focus, coral for rotation or
search feedback, and never rely on colour alone to communicate status.

## Typography

- Editorial headings: Georgia/serif system stack, 400 weight, tight leading.
- Interface and explanatory copy: system sans-serif stack.
- Keys, counters, tags, and operation history: system monospace stack.
- Use compact uppercase labels with letter spacing only for metadata, not body
  copy.

## Spacing and layout

- Base spacing unit: 4px; common gaps follow a 4/8/12/16/24/32/48/64px scale.
- Content maximum width: 1280px, centered with 32px desktop gutters.
- Desktop visualizer: operation rail, flexible tree stage, and narrow
  activity/metrics rail.
- Desktop reservations: flexible ID-tree and ledger area, with a reservation
  form and operation history in the supporting column.
- Tablet: collapse side rails into a two-column workspace where space permits.
- Mobile: stack controls, tree, ledger, and history; keep actions reachable
  without horizontal page scrolling.

## Components and interactions

- A compact top bar links the visualizer and reservation manager.
- The hero introduces each page's purpose and key action.
- Operation controls use labelled fields, explicit HTML form actions, and
  keyboard-visible focus.
- Trees are accessible SVGs generated from C++ snapshots with node IDs,
  stored heights, and balance factors.
- Metric cards expose node count, height, balance, and reservation status.
- Reservation tables expose view, edit, and delete actions using links/forms.
- History rows state operations and only report rotations emitted by C++.
- Buttons follow a clear primary/secondary hierarchy and minimum 44px touch
  targets on small screens.

## Accessibility and responsive behaviour

- Use semantic headings, landmarks, labels, status announcements, and
  descriptive SVG labels.
- Preserve visible focus rings and sufficient text/control contrast.
- At 375px and 390px, stack the layout and scale the tree SVG without page-level
  overflow.
- At 768px, use a compact two-column layout where space permits.
- At 1366px and 1440px, keep the full workspace within the centered container.
- Native HTML forms preserve keyboard operation and work without client-side
  scripts.

The web pages are rendered and served by the native C++ application. Build and
run instructions are in the repository's root README.
