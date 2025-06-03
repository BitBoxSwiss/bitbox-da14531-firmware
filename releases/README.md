# Reproducible builds

The BitBox DA14531 firmware
[releases](https://github.com/BitBoxSwiss/bitbox-da14531-firmware/releases/) are tagged `v1`, `v2`,
etc. The binaries are built from those tags in a reproducible manner, based on fixed versions of all
dependencies. We use Docker to fix those dependencies.

*Note*: it is possible to reproduce the binaries without Docker by installing the correct
dependencies. The instructions below use Docker however, as it makes it a easier to get started.

## What is the purpose of this?

The purpose is twofold:

- It is much more technically involved to actually build the binaries from source, than it is to
  verify signatures using `gpg` on the command line or in a GUI. This gives users who are willing
  and eager to verify the option to verify our releases even if they are unable to build the
  binaries themselves.
- The firmware binaries are only practically reproducible for a limited amount of time after the
  release. In the future, the dependencies of our Dockerfile might have changed, or Docker itself
  might even become incompatible or obsolete. This historic record allows you to be reasonably sure
  that the released binary was created correctly, even if it becomes infeasible to re-build the
  binary.

## Verify assertions by the community

Inspect the `assertion.txt` file of the relevant subdir,
e.g. [v1/assertion.txt](v1/assertion.txt).

The assertion.txt is created by the maintainers of this repo with every release.

Verify a signature confirming its contents, for example:

```sh
cd v1/
# import any missing public keys
gpg --import pubkeys/benma.asc
gpg --verify assertion-benma.sig assertion.txt
```

A valid signature means that the signer confirms that they could reproduce the binary from the
stated version tag.

## How to reproduce

Due to the licencing of "SDK6 for DA1453x, DA14585/6" it cannot be
redistributed here. To be able to rebuild the firmware you must register on
[renesas.com](https://www.renesas.com) and download the SDK.

> [!NOTE]
> You must use the amd64 version of the compiler. Other compilers output
> slightly different machine code.

### Instructions
* Download the correct version of the SDK from [renesas.com](https://www.renesas.com):
  * Register an account on the website.
  * The file is called SDK_X.zip where X can be 6.0.22.1401. Look in the
    Dockerfile to ensure which version is required.
  * Today you can find all the versions on [this
    page](https://www.renesas.com/en/products/wireless-connectivity/bluetooth-low-energy/da14531-smartbond-tiny-ultra-low-power-bluetooth-51-system-chip)
    by scrolling down to **Software Downloads**. At the bottom of that table
    ther is a "+ Add N hidden items" button you can click to display all
    available versions. Find the item called **SDKX for DA1453x, DA14585/6**
    (replace X with the actual version).

* Build the container image:
  ```
  make dockerinit
  ```
* Run the container image and build the firmware:
  ```
  ./scripts/dockerenv.sh release
  make firmware-release
  ```
* Calculate sha256
  ```
  sha256sum build-release/bitbox-da14531-firmware.bin
  ```

## Contribute your signature

We kindly ask you to independently build the firmware binaries we released, and verify that you get
the same result. Please open a PR to add your signed message confirming this. Signing a confirmation
like this *only* confirms that you got the same result. It *does not* endorse the contents or
quality of the binary itself.

### Contributing your signature

Please inspect the `assertion.txt` in the relevant subfolder,
e.g. [v1/assertion.txt](v1/assertion.txt). If you agree to its contents and verified that the sha256
hash therein matches the one you got, please sign the file using:

```sh
cd v1 # go to the relevant subfolder
gpg -o assertion-YOURNAME.sig --detach-sign assertion.txt
```

Open a PR adding your signature file to this folder. Also add your pgp pubkey to the
[./pubkeys](./pubkeys) folder:

```sh
gpg --export --armor YOUR_PGP_KEY_ID  > ./pubkeys/YOURNAME.asc
```
