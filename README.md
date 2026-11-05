# omasports

Live scores and fixtures for every sport, in one window, from the keyboard.

![omasports showing live NBA scores](screenshot.png)

## What it does

- Scores, schedules, and finals for 33 competitions across soccer, American
  football, basketball, baseball, hockey, tennis, golf, MMA, racing, rugby,
  cricket, volleyball, and Australian football.
- A "Today" button and day-by-day navigation, so yesterday's results are one
  key away.
- Games in progress refresh every 30 seconds, and the header shows how many
  are live.
- Search across every league, in the language of the desktop.
- Follows the Omarchy accent color, window rounding, and language live.

Scores come from ESPN's public scoreboard API. No key, no account. The feed is
unofficial, so a rare game can be missing or lag a minute behind.

## Install

```sh
bin/install
```

That builds the app, packages it with `makepkg`, and installs it through
`pkexec`, so it shows up in the Omarchy launcher. To run from the checkout
without installing, use `bin/build && build/omasports`.

## Keys

| Key | Action |
| --- | --- |
| `J` / `K`, `↓` / `↑` | Next / previous league |
| `H` / `L`, `←` / `→` | Previous / next day |
| `T` | Back to today |
| `Space`, `R` | Refresh scores |
| `/` | Search leagues |
| `?` | Show every shortcut |
| `Esc` | Close the help, or clear the search |
| `Q` | Quit |

## Languages

Every string is translated through Omarchy's language registry; French ships
with the app. Dates, times, numbers, and plurals follow the same locale, and
the window mirrors for right-to-left languages. To add one, copy
`src/i18n/fr.json`, translate the values, and add the file to
`src/resources.qrc`.

## Development

```sh
bin/build   # qmake6 + make into build/omasports
bin/test    # Qt Test suite, offscreen
```

The scoreboard parsing, the league catalog, persistence, and day navigation
are covered by tests that never touch the network: `Backend` takes an offline
flag for exactly that.

Adding a league is one line in the `catalog` table in `src/backend.cpp`, using
ESPN's sport and league slugs. Its scoreboard lives at
`https://site.api.espn.com/apis/site/v2/sports/<sport>/<league>/scoreboard`.

## Layout

```text
bin/build  bin/test  bin/install
src/main.cpp            application setup
src/backend.{h,cpp}     catalog, scoreboard fetching and parsing, saved league
src/theme.{h,cpp}       Omarchy accent and Hyprland rounding
src/translator.{h,cpp}  Omarchy language, live
src/Main.qml            window, header, keyboard, states
src/LeagueSidebar.qml   league list and search
src/EventCard.qml       one game or event
src/TeamBadge.qml       crest with an abbreviation fallback
src/ActionButton.qml    flat theme-following button
src/HelpOverlay.qml     the shortcut sheet
tests/                  calibration tests for backend, theme, translator
pkgbuild/               PKGBUILD, desktop entry, icon
```

## Credits

The theme watcher and translator follow [Omacut](https://github.com/omacom-io/omacut)
(MIT, David Heinemeier Hansson). Scores are served by ESPN's public API.

## License

MIT.
