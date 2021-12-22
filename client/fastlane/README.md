fastlane documentation
----

# Installation

Make sure you have the latest version of the Xcode command line tools installed:

```sh
xcode-select --install
```

For _fastlane_ installation instructions, see [Installing _fastlane_](https://docs.fastlane.tools/#installing-fastlane)

# Available Actions

### beta

```sh
[bundle exec] fastlane beta
```



### incrementVersion

```sh
[bundle exec] fastlane incrementVersion
```



### addToTestFlight

```sh
[bundle exec] fastlane addToTestFlight
```

Update Certificates, Run All tests, Build app

Add tag to git, send to Testflight, slack notification

### certificates

```sh
[bundle exec] fastlane certificates
```



### adhoc_certificates

```sh
[bundle exec] fastlane adhoc_certificates
```



### createAPNS

```sh
[bundle exec] fastlane createAPNS
```



### firebase_test

```sh
[bundle exec] fastlane firebase_test
```

Distribute app via Firebase for testers

### distribute_firebase

```sh
[bundle exec] fastlane distribute_firebase
```

Distribute app via Firebase for testers

----

This README.md is auto-generated and will be re-generated every time [_fastlane_](https://fastlane.tools) is run.

More information about _fastlane_ can be found on [fastlane.tools](https://fastlane.tools).

The documentation of _fastlane_ can be found on [docs.fastlane.tools](https://docs.fastlane.tools).
