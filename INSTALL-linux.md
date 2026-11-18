# Installing Bibirble on Linux 🐧

Bibirble for Linux is **one file** called `Bibirble`. The Bible verses and the
icon are built into it, so you don't need any data files next to it.

## 1. Check your Linux is new enough

You need a **64-bit (x86_64) Linux** with **glibc 2.38 or newer**. That means
roughly:

| Distro | Works from |
| --- | --- |
| Ubuntu / Kubuntu / Xubuntu | 24.04 |
| Linux Mint | 22 |
| Pop!_OS | 24.04 |
| Debian | 13 (trixie) |
| Fedora | 39 |
| Arch, Manjaro, EndeavourOS, openSUSE Tumbleweed | any current install |

Not sure? Run this and check that the number is 2.38 or higher:

```bash
ldd --version | head -1
```

On an older system, build it from source instead (see the last section).

## 2. Install the libraries it needs

Bibirble has wxWidgets and the C++ runtime built in. What it still needs from
your system is **GTK 3** (plus X11 session support). Most desktops already have
these, so this step usually just says "already installed" 👍

**Ubuntu, Mint, Pop!_OS, Debian**

```bash
sudo apt install libgtk-3-0 libsm6
```

**Fedora**

```bash
sudo dnf install gtk3 libSM
```

**Arch, Manjaro, EndeavourOS**

```bash
sudo pacman -S --needed gtk3 libsm
```

**openSUSE**

```bash
sudo zypper install libgtk-3-0 libSM6
```

<details>
<summary>The full list of shared libraries, for other distros</summary>

`libgtk-3.so.0`, `libgdk-3.so.0`, `libgdk_pixbuf-2.0.so.0`, `libpango-1.0.so.0`,
`libpangocairo-1.0.so.0`, `libpangoft2-1.0.so.0`, `libcairo.so.2`,
`libgio-2.0.so.0`, `libgobject-2.0.so.0`, `libglib-2.0.so.0`,
`libfontconfig.so.1`, `libxkbcommon.so.0`, `libX11.so.6`, `libSM.so.6`, plus
glibc (`libc.so.6`, `libm.so.6`). Everything except `libSM` comes with GTK 3.

If Bibirble won't start, this shows anything that's missing:

```bash
ldd ./Bibirble | grep "not found"
```

</details>

## 3. Unpack and run

From the folder where you downloaded `Bibirble-Linux-x86_64.tar.gz`:

```bash
tar xzf Bibirble-Linux-x86_64.tar.gz
```

```bash
./Bibirble/Bibirble
```

If it says "Permission denied", make it runnable first:

```bash
chmod +x Bibirble/Bibirble
```

It should also run on Wayland desktops (GNOME, KDE), since GTK 3 supports both
Wayland and X11.

## 4. Add it to your app menu (optional)

This copies Bibirble into your home folder and adds a menu entry with its icon.
It doesn't need `sudo`. Run it from the folder you unpacked into:

```bash
cd Bibirble
```

```bash
mkdir -p ~/.local/bin ~/.local/share/applications ~/.local/share/icons/hicolor/512x512/apps
```

```bash
cp Bibirble ~/.local/bin/ && cp bibirble.png ~/.local/share/icons/hicolor/512x512/apps/ && cp bibirble.desktop ~/.local/share/applications/
```

Bibirble should now be in your app menu (look under Games or Education). If it
doesn't show up, log out and back in. If it shows up but won't open, your
`~/.local/bin` isn't on your `PATH`, so change the `Exec=` line in
`~/.local/share/applications/bibirble.desktop` to the full path, for example
`Exec=/home/yourname/.local/bin/Bibirble`.

**To uninstall:**

```bash
rm ~/.local/bin/Bibirble ~/.local/share/applications/bibirble.desktop ~/.local/share/icons/hicolor/512x512/apps/bibirble.png
```

Your Daily progress is saved in `~/.Bibirble`. Delete that folder too if you
want a completely clean slate.

## Building from source instead

You need this route for older distros, ARM computers (like a Raspberry Pi), or
if you'd just rather build it yourself. Install the build tools and libraries:

**Ubuntu, Mint, Debian**

```bash
sudo apt install build-essential cmake libwxgtk3.2-dev nlohmann-json3-dev
```

**Fedora**

```bash
sudo dnf install gcc-c++ cmake wxGTK-devel json-devel
```

**Arch**

```bash
sudo pacman -S --needed base-devel cmake wxwidgets-gtk3 nlohmann-json
```

Then build from the repository folder:

```bash
./build.sh Release
```

```bash
./build/Bibirble
```

A source build links to your distro's wxWidgets, so the result needs that
wxWidgets package installed to run. It isn't a standalone single file like the
download. The verses and icon are still built in.
