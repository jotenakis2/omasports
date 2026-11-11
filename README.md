English | [Français](README.fr.md)

# omasports

**Version 1.0.0** — Live scores and fixtures for every sport, in one window, from the keyboard.
Native Qt Quick desktop app for Linux that follows your Omarchy theme and language.

![omasports showing Premier League results](screenshot.png)

## What it does

- Scores, schedules, and finals for 34 competitions across soccer, American
  football, basketball, baseball, hockey, tennis, golf, MMA, racing, rugby,
  cricket, volleyball, and Australian football.
- A **Live** view (`d`) scans every league you follow and keeps only the games
  in progress, with each sport and league called out.

  ![omasports showing live scores](screenshot-live.png)

- Goal scorers under the score, and a match sheet on click with goals, assists,
  cards, and key statistics.

  ![omasports showing goal scorers under a score](screenshot-scorers.png)
  ![omasports match sheet with goals, cards, and statistics](screenshot-match.png)

- A **Standings** view (`c`) shows the table for the selected league, with
  positions, played, wins, draws, losses, goal difference, and points, grouped
  into conferences or pools when the feed splits them.

  ![omasports showing Ligue 1 standings](screenshot-standings.png)

- An **Upcoming** view (`a`) lists the next matches for the selected league.
- A **Settings** menu (`p`) chooses which leagues appear in the sidebar and in
  the live view, sets the text size, and turns league logos on or off in the
  sidebar and header.
- A "Today" button and day-by-day navigation, so yesterday's results are one
  key away.
- Search across every league, in the language of the desktop.
- Games in progress refresh every 30 seconds; the live view rescans every
  minute. The header shows how many games are live.
- Follows the Omarchy palette (dark or light), accent, window rounding, and
  language live.

## Install

Three ways to install omasports. Pick the one that fits your setup.

### (a) Release archive

Download `omasports-<sha>-linux-<arch>.tar.gz` and its `.sha256` file from the
[Releases](../../releases) page (`x86_64` or `arm64`). Archives are produced by
CI and published to Releases — never use a local build as a distributable
archive.

```sh
sha256sum -c omasports-<sha>-linux-<arch>.tar.gz.sha256
tar -xzf omasports-<sha>-linux-<arch>.tar.gz
cd omasports-<sha>-linux-<arch>
install -Dm755 omasports ~/.local/bin/omasports
install -Dm644 omasports.svg ~/.local/share/icons/hicolor/scalable/apps/omasports.svg
install -Dm644 omasports.desktop ~/.local/share/applications/omasports.desktop
```

Adjust `Exec=` in the `.desktop` file if the binary lives somewhere other than
`~/.local/bin/omasports`.

### (b) Arch package (PKGBUILD)

Build and install the packaged app from the repository using
`pkgbuild/PKGBUILD`:

```sh
cd pkgbuild
makepkg -si
```

Or use the helper script, which builds, packages, and installs through `pkexec`
so omasports shows up in the Omarchy launcher:

```sh
bin/install
```

### (c) Build from source

Build from a checkout and run or link the binary yourself:

```sh
bin/build
./build/omasports
```

To make it available from your launcher without installing a package:

```sh
ln -sf "$PWD/build/omasports" ~/.local/bin/omasports
install -Dm644 pkgbuild/omasports.svg \
  ~/.local/share/icons/hicolor/scalable/apps/omasports.svg
install -Dm644 pkgbuild/omasports.desktop \
  ~/.local/share/applications/omasports.desktop
```

## Data source

Scores, schedules, standings, and match details come from ESPN's public
scoreboard API. No key or account is required.

- **Unofficial feed**: the API is not a documented, supported product and comes
  with no SLA. A game can be missing, delayed by a minute, or change shape
  without notice.
- **Season handling**: omasports sends the season explicitly when querying data
  so the results match the expected competition year; when the season cannot be
  determined reliably, the interface reports an uncertain season.
- **Ghost fixtures**: duplicate or phantom fixtures returned by the feed are
  deduplicated before display.
- **Logos**: national flags are never used as club logos; only appropriate team
  or league marks are shown when logos are enabled.
- **Availability**: when the feed does not respond, omasports surfaces a network
  error and keeps the interface usable; there is no persistent offline cache.

## Configuration

Open **Settings** with `p`.

- **Text size**: five levels, applied across the interface.
- **League logos**: toggles for showing logos in the sidebar and in the header.
- **Enabled leagues**: choose which leagues appear in the sidebar and are scanned
  by the Live view.

Settings are stored in the application's Qt settings for your user account (via
`QSettings` for the omasports application) and are remembered across launches.

## Troubleshooting

- **Empty standings**: the selected league may not provide a standings table for
  the current season, or the feed returned no rows. Try refreshing, switching to
  another date/season context, or check another league.
- **Uncertain season**: the season could not be determined reliably from the
  data. Scores may still be shown, but treat season-specific context with care.
- **Match sheet unavailable**: detailed match data (scorers, cards, statistics)
  is not always published for every competition or event. A sheet may be missing
  or incomplete even when the score is known.
- **Network error**: the ESPN feed could not be reached or did not respond in
  time. Check your connection and press `space` or `r` to refresh. The app does
  not keep a persistent offline cache.
- **Shortcut does not respond while searching**: while the league search field
  has focus, text input goes to the search. Press `esc` to clear or close the
  search and restore normal shortcuts.

## Keys

Keys are shown lowercase; uppercase letters work too.

| Key | Action |
| --- | --- |
| `d` | Live view: every game in progress |
| `c` | Standings for the selected league |
| `a` | Upcoming matches for the selected league |
| Click a match | Open its sheet |
| `p` | Settings: choose which leagues to show, text size, logos |
| `j` / `k`, `↓` / `↑` | Next / previous league |
| `h` / `l`, `←` / `→` | Previous / next day |
| `t` | Back to today |
| `space`, `r` | Refresh scores |
| `/` | Search leagues |
| `?` | Show every shortcut |
| `esc` | Close the help or settings, or clear the search |
| `q` | Quit |

## Languages

Every string is translated through Omarchy's language registry; English and
French ship with the app. Dates, times, numbers, plurals, and record
abbreviations follow the same locale, and the window mirrors for right-to-left
languages. To add one, copy `src/i18n/en.json` or `src/i18n/fr.json`, translate
the values, and add the file to `src/resources.qrc`.

## Development

```sh
bin/build        # qmake6 + make into build/omasports
bin/test         # Qt Test suite, offscreen
./check.sh       # shellcheck (bin/*, check.sh), build, then offscreen tests
bin/clean        # remove build/ and dist/
```

After a local build, run `bin/clean` to leave nothing behind: distributable
archives must come from CI, not from a local machine.

There is deliberately no local cross-architecture build: the x86_64 and arm64
archives are produced by the CI workflow and nothing is built outside the native
host. Set `BUILD_DIR` to send a normal `bin/build` somewhere other than
`build/`.

Running the tests requires Qt Test and Qt Quick Test (QML module `QtTest` and
`libQt6QuickTest`, provided by `qt6-declarative`). The geometry tests load QML
components offscreen (`QT_QPA_PLATFORM=offscreen`) and make no network calls.

`check.sh` aggregates the checks (non-zero exit code if any step fails) and is
the validation entry point before committing.

### Build on Debian/Ubuntu

The CI builds and tests on Ubuntu 24.04 with the packages below (`qmake6`,
`make`, and `g++` are required by `bin/build`):

```sh
sudo apt-get install --no-install-recommends \
  build-essential pkg-config shellcheck \
  qt6-base-dev qt6-declarative-dev qt6-declarative-dev-tools \
  qt6-wayland qt6-wayland-dev qt6-tools-dev \
  qml6-module-qttest qml6-module-qtquick qml6-module-qtquick-window \
  qml6-module-qtquick-controls qml6-module-qtquick-templates \
  qml6-module-qtquick-layouts \
  qml6-module-qtqml qml6-module-qtqml-models qml6-module-qtqml-workerscript \
  libqt6quicktest6
```

The `qml6-module-*` packages provide the QML modules loaded at runtime by the
tests and the Qt Quick components. `libqt6quicktest6` provides the Quick Test
runtime linked by `QT += quicktest`.

Adding a league is one line in the `catalog` table in `src/backend.cpp`, using
ESPN's sport and league slugs. Its scoreboard lives at
`https://site.api.espn.com/apis/site/v2/sports/<sport>/<league>/scoreboard`.

## Layout

```text
bin/build  bin/test  bin/install  bin/clean
src/main.cpp            application setup
src/backend.{h,cpp}     catalog, scores, live scan, league filter, settings
src/theme.{h,cpp}       Omarchy palette and Hyprland rounding
src/translator.{h,cpp}  Omarchy language, live
src/Main.qml            window, header, keyboard, states
src/LeagueSidebar.qml   league list, sport sections, search
src/EventCard.qml       one game or event
src/StandingsView.qml   the league table
src/UpcomingView.qml    upcoming matches
src/MatchOverlay.qml    the match sheet
src/TeamBadge.qml       crest with an abbreviation fallback
src/ActionButton.qml    flat theme-following button
src/LogoMark.qml        the score-bar mark, in the live accent
src/SettingsOverlay.qml the settings checklist (leagues, text size, logos)
src/HelpOverlay.qml     the shortcut sheet
tests/                  tests for backend, theme, translator
pkgbuild/               PKGBUILD, desktop entry, icon
```

## Credits

The theme watcher and translator follow [Omacut](https://github.com/omacom-io/omacut)
(MIT, David Heinemeier Hansson). Scores are served by ESPN's public API.

## License

MIT.
