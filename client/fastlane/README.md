fastlane documentation
----

# Installation

Make sure you have the latest version of the Xcode command line tools installed:

```sh
xcode-select --install
```

For _fastlane_ installation instructions, see [Installing _fastlane_](https://docs.fastlane.tools/#installing-fastlane)

# Available Actions

### load_api_key

```sh
[bundle exec] fastlane load_api_key
```

Load AppStore Connect APIKey for using in lanes

### fetch_and_increment_build_version

```sh
[bundle exec] fastlane fetch_and_increment_build_version
```

Increase build number based on most recent TestFlight existing one

### setup_signing_identities

```sh
[bundle exec] fastlane setup_signing_identities
```

Set up signing identities: install certificates in keychain and download provisioning profiles

### build_release

```sh
[bundle exec] fastlane build_release
```

build app for release (TestFlight)

### upload_release_to_testflight

```sh
[bundle exec] fastlane upload_release_to_testflight
```

Upload to TestFlight via AppStore Connect

### build_and_upload_to_testflight

```sh
[bundle exec] fastlane build_and_upload_to_testflight
```

Build release and upload to TestFlight

----

This README.md is auto-generated and will be re-generated every time [_fastlane_](https://fastlane.tools) is run.

More information about _fastlane_ can be found on [fastlane.tools](https://fastlane.tools).

The documentation of _fastlane_ can be found on [docs.fastlane.tools](https://docs.fastlane.tools).
