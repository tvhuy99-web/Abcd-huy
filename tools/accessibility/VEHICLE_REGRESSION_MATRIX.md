# Accessibility vehicle regression matrix

This file is an **on-device acceptance plan**, not a claim that these cases have
already passed. Run with the newest Android test-signed APK and a COPY of the
saved game. Record the build SHA, vehicle, heading, selected destination,
coordinates before/after, and the full `cataclysm-session-debug` file.

## Common safety rules

- No movement, turn, or reversing past a hard physical obstruction without
  native vehicle collision physics and a fresh map check.
- Ground item stacks are passable to the route/steering *classifier*. The
  ordinary vehicle movement engine remains responsible for impact and wheel
  damage. Impassable terrain, obstructing furniture, traps, creatures and other
  vehicles remain obstacles.
- All commands require the actual player in control of the actual vehicle.
  Native players and NPCs are unaffected by the accessibility-only recovery.
- A stopped car must never appear to have rotated when `face.dir()` did not
  change. Do not speak the requested steering direction as the physical one.
- There must be no invisible vehicle teleportation, obstacle modal, or
  uncontrollable reversal. Pausing/canceling must apply real brakes.

## 1. Shared obstacle policy (Manual, Automatic, Explore Places autopilot)

| Scenario | Manual 45° turn | Gesture Automatic Drive | Explore Places self-driving |
| --- | --- | --- | --- |
| Dry grass or pavement with one small loose item | If wheel/steering and sweep are clear, turn succeeds | No spurious `Automatic braking: Loose items` | A* does not mark the item as a wall |
| Passable ground with several dropped items | No fabricated terrain blocker | No stop triggered by `has_items()` alone | Same drivable classification |
| Solid wall ahead / within turning hull | Reject rotation with correct wall label | Brake before collision | Replan, then pause if no safe local path |
| Obstructing furniture | Reject if hull collides; speak furniture name | Announce furniture as obstacle | Avoid tile or pause |
| Visible trap | Reject if hull collides; speak trap name, not grass | Warn and stop in assisted mode | Avoid trap or pause |
| Other vehicle | Reject hull overlap | Announce that vehicle | Avoid or pause |
| Creature | Reject occupied swept path | Announce creature and warn/brake | Avoid or pause |
| Map streaming boundary | No off-map pivot | Never describe map edge as a physical wall | Route stays within loaded pathfinding bounds |

The native Catch2 regression `accessible_vehicle_collision_classifier_ignores_loose_ground_items`
specifically tests pavement, a dropped item, and a wall. The Python source
contracts verify that all three reader vehicle systems call the same classifier.

## 2. Native-physics reverse escape (Explore Places only)

1. Park the vehicle facing away from a reachable road destination with a
   **clear** straight one-tile rear path and an obstructed forward route.
2. Start `Khám phá địa điểm → Đi xe`.
3. If A*, safe pivot, and alternative first segments all fail, expect
   `AUTODRIVE_REVERSE result=started attempt=1` **only** if straight
   reversed body placement is safe and the rear sensor sees no near obstacle.
4. The ordinary vehicle engine should move the car backwards at cautious
   speed. On movement of the first map tile, expect
   `AUTODRIVE_REVERSE result=moved_one_tile`, physical braking, and replanning
   towards the original destination.
5. Repeat with a wall/creature/vehicle behind: no recovery thrust permitted.
6. Repeat with engine off or broken wheels: after a bounded stall, pause
   safely rather than continuing to request throttle forever.
7. Confirm no more than **two** one-tile reverse attempts per overmap segment,
   a **six-tick** maximum for a stalled attempt, and no steering while reversing.
8. Pause/cancel during recovery: immediate safe brake, no further reverse;
   another character or NPC must never inherit recovery state.

## 3. Explore Places strategic routing

- Forest east, from `Xe đuôi cụt`: verify actual position moves; reaching a
  nearby drivable area is not the same as reaching the selected forest center.
- Destination on forest, tree or other nondrivable tile: stop at a safe nearby
  reachable area and announce precisely what happened.
- An impossible forward route with no physically possible first action:
  `AUTODRIVE_REROUTE skipped=no_executable_first_step`; do not launch
  expensive cross-map search.
- A valid alternative candidate must pass *both* local A* and the real
  first-turn/first-motion collision check. Never adopt an unreachable detour.
- An invalid or dangerous route: `AUTODRIVE_ROUTE paused ...`, preserve
  journey for explicit Resume/Cancel, do not oscillate or accelerate.

## 4. Controls, gestures and spoken feedback

- In Drive, double-tap the actual world: pause/resume as intended.
- Open the comprehensive menu by the documented gesture; select
  `Khám phá địa điểm` or another menu item. The selected command executes;
  it must **not** become `reader:vehicle:pause`.
- While parked, request a 45° left/right turn with clear and blocked sweeps.
  Announce a compass change only when the actual heading changes; a rejected
  turn reads the real blocker and does not claim the car rotated.
- On a collision pause, cancel the stale `Đang lái đến...` speech but retain
  the important warning; on successful arrival, announce completion.
- Manual mode must never be silently replaced by unattended automation;
  turning and throttle remain normal native player actions.

## 5. Evidence and acceptance

For each scenario save:
`APP_VERSION`, initial/final vehicle and avatar coordinates,
`AUTODRIVE_PIVOT_BLOCK`, `AUTODRIVE_PIVOT`,
`AUTODRIVE_REVERSE_PROBE`, `AUTODRIVE_REVERSE`,
`AUTODRIVE_REROUTE`, `AUTODRIVE_ROUTE`, and relevant `TTS_*` lines.

**Acceptance requires runtime evidence**, not merely passing source-text
assertions or compiling an APK. The physical test suite cannot be declared
passed without the corresponding game save, controlled map fixture, or an
on-device run with a log.
