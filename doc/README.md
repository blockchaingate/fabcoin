Fabcoin Core
=============

Setup
---------------------
Fabcoin Core is the original Fabcoin client and it builds the backbone of the network. It downloads and, by default, stores the entire history of Fabcoin transactions (which is currently just started and will grow as time being); depending on the speed of your computer and network connection, the synchronization process will depends on the volume of the blocks generated.

To download Fabcoin Core, please visit [fabexplorer.info](HTTPS://Fabexplorer.info/EN/RELEASES/).

Running
---------------------
The following are some helpful notes on how to run Fabcoin on your native platform.

### Unix

Unpack the files into a directory and run:

- `bin/fabcoin-qt` (GUI) or
- `bin/fabcoind` (headless)

### Windows

Unpack the files into a directory, and then run fabcoin-qt.exe.

### OS X

Drag Fabcoin-Core to your applications folder, and then run Fabcoin-Core.



Building
---------------------
The following are developer notes on how to build Fabcoin on your native platform. They are not complete guides, but include notes on the necessary libraries, compile flags, etc.

- [OS X Build Notes](build-osx.md)
- [Unix Build Notes](build-unix.md)
- [Windows Build Notes](build-windows.md)
- [OpenBSD Build Notes](build-openbsd.md)
- [Gitian Building Guide](gitian-building.md)

Development
---------------------
The Fabcoin repo's [root README](/README.md) contains relevant information on the development process and automated testing.

- [Developer Notes](developer-notes.md)
- [Release Notes](release-notes.md)
- [Release Process](release-process.md)
- [Source Code Documentation (External Link)](https://dev.visucore.com/fabcoin/doxygen/)
- [Translation Process](translation_process.md)
- [Translation Strings Policy](translation_strings_policy.md)
- [Travis CI](travis-ci.md)
- [Unauthenticated REST Interface](REST-interface.md)
- [Shared Libraries](shared-libraries.md)
- [BIPS](bips.md)
- [Dnsseed Policy](dnsseed-policy.md)
- [Benchmarking](benchmarking.md)


### Miscellaneous
- [Assets Attribution](assets-attribution.md)
- [Files](files.md)
- [Fuzz-testing](fuzzing.md)
- [Reduce Traffic](reduce-traffic.md)
- [Tor Support](tor.md)
- [Init Scripts (systemd/upstart/openrc)](init.md)
- [ZMQ](zmq.md)

License
---------------------
Distributed under the [MIT software license](/COPYING).
This product includes software developed by the OpenSSL Project for use in the [OpenSSL Toolkit](https://www.openssl.org/). This product includes
cryptographic software written by Eric Young ([eay@cryptsoft.com](mailto:eay@cryptsoft.com)), and UPnP software written by Thomas Bernard.
