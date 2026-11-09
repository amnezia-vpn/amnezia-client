# AGENTS.md

Operating guide for this repository. Read it before changing anything.

## Project Overview

AmneziaVPN is a cross-platform VPN client built with C++ and Qt Quick (QML) for
Windows, macOS, Linux, Android and iOS. It has three kinds of servers, each
with its own config model (`serverConfigUtils::ConfigType`):

- **Self-hosted.** The admin connects the app to their own server over SSH; the
  app installs protocols as Docker containers (AmneziaWG, WireGuard, OpenVPN,
  XRay, IPsec and others) and generates client configs (`SelfHostedAdmin`).
  Access is shared with other users as `vpn://` keys, which import without
  server management rights (`SelfHostedUser`).
- **Amnezia Premium and Free.** Configs come from the Amnezia gateway API. The
  gateway passes the backend's config through; the client does not invent
  server-side values.
- **Native protocol configs.** Third-party configs imported as they are:
  WireGuard and AmneziaWG `.conf`, OpenVPN `.ovpn`, XRay and Shadowsocks links
  (`Native`). The app only connects with them; there is no server to manage.

A change to shared code (protocol configs, import, connection) must keep
working for all three.

The desktop builds run a privileged service next to the GUI; the GUI talks to
it over Qt Remote Objects. Android, iOS and the macOS App Store build
(`MACOS_NE`) run the tunnel in the platform's VPN extension instead.

## Repository Layout

- `client/core/` — business logic with no UI: controllers, configurators
  (client config generation per protocol), installers (server-side setup over
  SSH), protocol and server config models, repositories (encrypted settings
  storage).
- `client/ui/controllers/` — `*UiController` classes exposed to QML. Thin
  adapters over `core/`: they translate between QML and core types and hold no
  business logic of their own.
- `client/ui/models/` — list and property models for QML.
- `client/ui/qml/` — the interface:
  - `Pages2/` — screens, one file per page.
  - `Components/` — composite pieces of screens.
  - `Controls2/` — design system controls, named `<Name>Type.qml`; text styles
    live in `Controls2/TextTypes/`.
  - `Modules/Style/AmneziaStyle.qml` — design tokens (colours and the like).
  - `Config/` — `GlobalConfig`, `Dialogs` and other singletons.
- `client/platforms/`, `client/android/`, `client/ios/`, `client/macos/` —
  platform code: VPN extensions, Kotlin, Swift, Objective-C++.
- `client/daemon/`, `client/mozilla/`, `service/`, `ipc/` — the privileged
  service and its IPC.
- `client/server_scripts/` — the shell scripts and Dockerfiles run on the
  user's server.
- `client/translations/` — Qt `.ts` files.
- `client/tests/` — Qt Test suites, run through `ctest`.
- `recipes/` — Conan recipes for native dependencies (protocol libraries,
  OpenSSL, libssh and others).
- `deploy/` — build and installer scripts. `.github/workflows/` — CI.
- `client/3rd/` — vendored third-party code. Do not edit it.

## Essential Commands

Run from the repository root.

```bash
deploy/build.sh -h                                     # build options
deploy/build.sh                                        # build for the host platform
deploy/build.sh -t android --aab                       # Android APK and AAB
cmake -S . -B build -DCMAKE_PREFIX_PATH=<Qt>/<platform> -DAMNEZIA_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build/client/tests --output-on-failure
```

Qt 6.10 or later and Conan are required; native dependencies are built by Conan
from `recipes/` on the first configure.

## Repository Rules

These three come first. Each one has been asked for repeatedly.

### 1. Clean, self-documenting code, no comments

- Names and structure carry the meaning. Code must be readable without help.
- No explanatory comments in code: no `//` notes in QML or C++, no doc blocks.
- The one exception is a hack that looks wrong on purpose: a workaround for a
  platform or Qt bug, an ordering that must not be "fixed". Then a short comment
  next to it says why.
- Explanations of a change go into the chat summary or the commit message, not
  into the code.
- Leave existing comments alone unless the change removes the code they
  describe.

### 2. Reusable code (DRY), without abstraction for its own sake

- **QML.** Every reusable component lives in its own file. A piece of a page
  goes into its own file in `Components/` even when it is used once, if that
  keeps the page short and readable. No inline `component` definitions or
  `Component` blocks inside pages. Before writing a new control, look in
  `Controls2/` and `Components/` for one that already does the job, and extend
  it if it almost does.
- **C++.** Logic shared by several places is extracted once and reused:
  duplicated parsing, validation or config building is a bug waiting to diverge.
- **No fanaticism.** Do not create a base class, interface, helper or file
  that only renames a single call, has one trivial user, or exists "for the
  future". Decompose when it makes the code shorter, clearer or removes
  duplication; otherwise keep it direct.

### 3. Do not add what was not asked for

- Implement exactly the request. No extra screens, buttons, settings,
  refactors, renames, translations or "while I'm here" fixes.
- Ideas, improvements and problems noticed along the way are reported to the
  user as suggestions. Nothing is implemented without the user agreeing first.
- When a request is ambiguous, ask instead of guessing big.
- Keep the diff small. If a fix grows beyond the obvious change, say so and
  explain why before making it.

### Other rules

- **Committing is the user's call.** Prepare the change and a commit message;
  do not commit or push unless asked. Commit messages use a conventional prefix
  with a scope (`fix(ui): …`, `feat(macos): …`, `chore: …`): a one-line
  subject plus one or two short body lines saying what was wrong and what
  changed. Pull requests target `dev`.
- **Format C++ with `.clang-format`.** Format the lines you touch; do not
  reformat files the change does not otherwise touch.
- **Delete unused code** instead of keeping it "for later".
- **Do not edit `client/3rd/`.** Fix vendored code through its upstream or a
  Conan recipe.

## UI And Design System

- **Figma is the source of truth.** Screens follow the "Amnezia APP VPN" file;
  components follow the "Amnezia DS Library". Where the two disagree, the app
  screens win.
- **Build UI from the design system controls** in `Controls2/`
  (`ButtonType`, `ListItemType`, `TextFieldType`, `SheetType`,
  `NavigationBarType` and the others) and the text types in
  `Controls2/TextTypes/`. Do not style raw Qt Quick Controls on a page.
- **No hex colours in QML.** Every colour is a semantic token from
  `AmneziaStyle.color`. A colour the tokens do not have is added to
  `AmneziaStyle` first, with the name the design system gives it.
- **Icons come from Figma**, exported from the DS Library. Never draw an icon
  by hand. Exports include page background rectangles — strip them and keep
  the `0 0 24 24` viewBox.
- **Restyling a component never redesigns a screen.** A design system update
  changes components; screen layout and behaviour stay unless the user asks.
- **No Qt5Compat.GraphicalEffects.** It overflows the interpreted JS stack on
  iOS. Use `QtQuick.Effects` (`MultiEffect`), and create effects only while
  they are visible: each instance is expensive to create.

### QML pitfalls

- A vertical `ListView` resets the delegate's `x`. Put insets on the
  ListView's `leftMargin`/`rightMargin`, or on a child inside the delegate.
- `lineHeightMode: Text.FixedHeight` puts all the extra leading below the text,
  while Figma splits it evenly. The text types already compensate with
  `halfLeading` padding; a new text type must do the same.
- QML is loaded from `qml.qrc`, not compiled ahead of time: a QML error shows
  up only at runtime. When moving QML between files, compare the `import`
  lines — types registered from C++ under their own URIs fail only when the
  page is opened.

## Translations

- Every user-visible string goes through `qsTr()` in QML or `tr()` in C++.
- The `.ts` files contain only finished, non-empty translations: no
  `vanished`, `obsolete` or `unfinished` entries, and every language holds the
  same set of strings. `lrelease` must report `N finished and 0 unfinished` for
  every file.
- Refresh `.ts` files only when the task is about translations. `lupdate` run
  through `qt6_add_translations` misses QML, so run it over an explicit file
  list that skips `client/3rd` (from `client/translations`):

  ```bash
  find .. -path ../3rd -prune -o -path ../3rd-prebuilt -prune -o \
    \( -name '*.qml' -o -name '*.cpp' -o -name '*.h' -o -name '*.mm' \) -print \
    | sort > /tmp/src.lst
  <Qt>/bin/lupdate @/tmp/src.lst -ts amneziavpn_*.ts
  ```

- `lupdate` output differs between macOS and Windows (keychain strings swap
  places), so a large `.ts` diff after a refresh is often only that.
- Technical terms stay in English in every language: protocol parameter names
  and their descriptions (`Jc`, `S1`, `H1`…), config keys, transport and
  algorithm names. Ordinary UI words are translated.
- Renaming a QML file changes its translation context, so its strings have to
  be carried over.

## Which Check When

- **Touched C++** — build the affected targets, and run `ctest` when the change
  touches anything the tests cover (servers, imports, configs, settings,
  split tunneling, XRay).
- **Touched QML** — build, then open every page you changed. If the app cannot
  be launched, at least load the changed files through the QML engine so that
  syntax, imports and type errors surface, and say what was not checked.
- **Touched a protocol config, import or connection path** — check self-hosted,
  Premium/Free and native configs, and every platform that consumes the field
  (desktop service, Android, iOS/macOS NE).
- **Touched CMake, recipes or the app icon** — reconfigure from a clean build
  directory as well: branding paths are CMake cache variables and keep stale
  values in an existing build directory.
- **Touched translations** — `lrelease` on every `.ts` file, see above.

## Privacy

- Never commit or repeat private keys, `vpn://` keys, server addresses,
  passwords or tokens — including ones found in pasted configs, logs or test
  output.
- Logs must not contain secrets or full config bodies.
- Local absolute paths (`/Users/...`, `C:\...`) do not belong in repository
  files.

## Verification Culture

- Verify your own work before reporting it. "Should work" is not a result;
  say exactly what was built, run or opened, and what was not.
- Hypotheses are settled by an experiment, not by reasoning about the code.
- After a wrong diagnosis, remove the code it produced.
