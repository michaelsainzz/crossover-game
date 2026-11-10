# First playable build

## Proposed product direction

One fictional neighborhood court at dusk. Six fictional athletes share a small set of animations and two readable team palettes. A broadcast-style camera keeps the play and passing lanes in view. Bold silhouettes, painted surfaces, a restrained color grade, chunky score graphics, and original court audio deliver the semi-retro feel.

Play an offline half-court game to 11. Score one point inside the arc and two outside, with a four-minute regulation cap. A tie after regulation goes to the next basket. After a score, out-of-bounds turnover, or shot-clock violation, restart at the top of the arc with the entitled team. On a defensive recovery, clear past the arc before scoring. The player controls one teammate at a time, follows the ball on a completed teammate pass, and can switch defenders. Controls cover movement, sprint, pass, shoot with contextual dunk, and contest or steal. Five simple bots cover spacing, passing, shot selection, guarding, and loose balls.

Defer online play, couch multiplayer, career mode, character creation, multiple courts, motion capture, commentary, and storefront release. Estimates are focused work blocks and exclude engine setup surprises and time between playtests.

| Gate | Build | Evidence and owner decision |
| --- | --- | --- |
| M0 · Toolchain (1–2 blocks) | Tiny C++ project and one plain test map. | Clean checkout compiles, runs a known automation test and packages and launches a Mac build. Record engine patch and Xcode. |
| M1 · Shot feel (2–4 blocks) | One player, one hoop, gamepad and keyboard, camera, shot and dunk. | Run scripted scoring and reset checks. Play or watch 30–60 seconds and approve movement, camera, and shot feel. |
| M2 · Full 3v3 loop (3–6 blocks) | Five bots, possession, passes, contests, scoring, clock, result and rematch. | Script a complete match and five consecutive resets. Play two matches and approve spacing, readability, and difficulty. |
| M3 · Art and sound (2–4 blocks) | One stylized court, fictional roster, HUD, shot and dunk effects, and licensed or original audio. | Review a short gameplay clip and asset provenance. Approve the visual and audio direction before adding content. |
| M4 · Shareable build (1–2 blocks) | Standalone Mac build, controls card and known issues. | Package from a fresh checkout, launch outside the editor, finish and rematch on the reference Mac, and capture frame times. Approve distribution separately. |

For the first performance baseline, target 60 fps at 1920×1080 or an equivalent render resolution on the reference M4. Capture a full regulation match after shader warmup using the low preset. Require a median frame time at or below 16.7 ms and a 95th percentile at or below 22 ms for the slice target. Record resolution, preset, build, and CPU/GPU timing. If the target fails, bring a profiled fix or a measured tradeoff to the owner.

## Original identity

Use the arcade sports genre as inspiration. Create distinct players, team identities, jerseys, courts, logos, interface, sounds and voice lines. Exclude league and team marks, identifiable real athletes, copied graphics or audio, and prompts to imitate a recognizable athlete or commentator. Crossover is a working title. Run title and logo clearance before public branding.

Before adding an external or generated asset, record the destination, creator/tool and source, date, license or terms, proof reference, attribution, and whether public source redistribution and packaged-game use are allowed. Restricted source assets stay out of public Git and out of any build lacking permission. Keep third-party notices with approved assets. A public repo does not set an open-source license by itself.

References. [U.S. Copyright Office guidance on games](https://www.copyright.gov/register/tx-games.html) and [USPTO trademark searching](https://www.uspto.gov/trademarks/search/federal-trademark-searching).
