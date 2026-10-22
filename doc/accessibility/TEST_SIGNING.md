# Public signing key for development APKs

`android/app/accessibility-test.keystore` is deliberately public, non-production
test material. Store type: PKCS12. Store/key password: `android`. Alias:
`androiddebugkey`. Certificate SHA-256:

`4b2aa6143aee39bf179fecc853c803113ed8bb8c911b6251088ef945babf701f`

CI uses it only with the explicit `-Paccessibility_test_signing=true` property
and the `accessibility` debug flavor, so consecutive test APKs can update the
same isolated test app without uninstalling it or losing its test saves.
Release signing is unchanged and must never use this public key. Do not use
this key for a trusted production distribution or place production signing
credentials in this repository.

Build from `android/`:

```sh
bash ./gradlew -Pabi_arm_32=false -Paccessibility_test_signing=true assembleAccessibilityDebug
```

The ARM64 candidate workflow verifies the APK signature against the fingerprint
file, in addition to the library, Vietnamese catalog and APK checksum checks.
