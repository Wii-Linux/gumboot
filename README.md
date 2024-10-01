# Gumboot

Modified version by [Techflash](https://github.com/techflashYT) and the rest of the [Wii Linux project](https://github.com/Wii-Linux).

Original Gumboot made by [neagix](https://github.com/neagix)

See [docs/index.md](docs/index.md) or https://neagix.github.io/gumboot/

# Building

This project relies on [devkitamateur / ppcskel](https://github.com/AndrewPiroli/ppcskel) to build properly.  It's tested working with the latest (at time of writing) GCC 13.2.0 / Binutils 2.41.0 release.

Thanks to [buddyjojo / JoJo Autoboy](https://github.com/buddyjojo) for pointing me in the right direction for how to get this building.

```
WIIDEV=/path/to/devkitamateur make
```

# Changes from neagix's original

- Updates to build with more modern toolchains
- Minor fix to `memstr` in order to get custom kernel cmdlines working


# Future plans

- Support for non-8.3 filenames
- Verify and fix support for larger kernels
- Speed up loading the kernel (it's quite slow compared to other Wii software)
- Initramfs support

# License

[GNU/GPL v2](COPYING)
