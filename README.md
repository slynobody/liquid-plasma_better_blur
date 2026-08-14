[![Buy Me A Coffee](https://img.shields.io/badge/Buy%20Me%20a%20Coffee-ffdd00?&logo=buy-me-a-coffee&logoColor=black)](https://ko-fi.com/integr)

# better blur dx+
<img width="468" height="216" alt="ezgif-2791664fde5198af" src="https://github.com/user-attachments/assets/1b245ea7-5e8f-4c0f-9cd5-7d6458cf8d19" />

## "liquid" blur for kde plasma (linux, interface-effect, *precompiled*)
### ('better-blur dx' plasma-extension, forked)

forked from: https://github.com/xarblu/kwin-effects-better-blur-dx

*precompiled* for easy installation

<img width="800" height="450" alt="ezgif-8fbc2ed7f426659f (1)" src="https://github.com/user-attachments/assets/90194af8-508c-47cc-81ab-6ee37c275e4f" />

experimental: (v2.6)

new features 
* border hightlight
* mouse highlight
* wobbly-windows-compatibility (though bundled [BetterWobblyWindows](https://github.com/GoldenBilly/BetterWobblyWindows)
* better rounded corners
* performace-tweaks

# usage
1. *dis*able 'blur' & 'transparency' - effects (settings-window)
2. *en*able 'better blur dx+' (settings-window)
3. use a global theme that supports translucency, like [this](https://www.pling.com/p/2328884)



# install (debian, sid, Plasma 6.7.4, QT 6.10.2)
> wget https://github.com/slynobody/liquid-plasma_better_blur/releases/download/2.6/int_better_blur_liquid_2-6-6_7_4_amd64.deb
> 
> sudo apt install ./int_better_blur_liquid_2-6-6_7_4_amd64.deb
> 
> wget https://raw.githubusercontent.com/slynobody/liquid-plasma_better_blur/refs/heads/main/defaults
>
> cp ~/.config/kwinrc ~/.config/kwinrc_bu
> 
> cat ./defaults > ~/.config/kwinrc
>
> kcmshell6 kcm_kwin_effects


<details>
  <summary>older (Debian)</summary>

# install (debian, forky)
> wget https://github.com/slynobody/liquid-plasma_better_blur/releases/download/experimental/int_better_blur_liquid_1-6_7_2_amd64.deb
> 
> sudo apt install ./int_better_blur_liquid_1-6_7_2_amd64.deb
> 
> wget https://raw.githubusercontent.com/slynobody/liquid-plasma_better_blur/refs/heads/main/defaults
>
> cp ~/.config/kwinrc ~/.config/kwinrc_bu
> 
> cat ./defaults > ~/.config/kwinrc

# install (debian, trixie)
> wget https://github.com/slynobody/liquid-plasma_better_blur/releases/download/1.6/int_better_blur_liquid_1-6_amd64.deb
> 
> sudo apt install ./int_better_blur_liquid_1-6_amd64.deb
> 
> wget https://raw.githubusercontent.com/slynobody/liquid-plasma_better_blur/refs/heads/main/defaults
>
> cp ~/.config/kwinrc ~/.config/kwinrc_bu
> 
> cat ./defaults > ~/.config/kwinrc
> 
> kcmshell6 kcm_kwin_effects

</details>


## manual install
> [!NOTE]
> On Fedora Kinoite and other distributions based on it, the effect must be built in a container.

### Dependencies
- CMake
- Extra CMake Modules
- Plasma 6
- Qt 6
- KF6
- KWin development packages

<details>
  <summary>Arch Linux</summary>
  <br>

  Wayland:
  ```
  sudo pacman -S base-devel git extra-cmake-modules qt6-tools kwin
  ```

  X11:
  ```
  sudo pacman -S base-devel git extra-cmake-modules qt6-tools kwin-x11
  ```
</details>

<details>
  <summary>Debian-based (KDE Neon, Kubuntu, Ubuntu)</summary>
  <br>

  Wayland:
  ```
  sudo apt install -y git cmake g++ extra-cmake-modules qt6-tools-dev kwin-dev libkf6configwidgets-dev gettext libkf6crash-dev libkf6globalaccel-dev libkf6kio-dev libkf6service-dev libkf6notifications-dev libkf6kcmutils-dev libkdecorations3-dev libxcb-composite0-dev libxcb-randr0-dev libxcb-shm0-dev libxcb-res0-dev libxcb-sync-dev
  ```

  X11:
  ```
  sudo apt install -y git cmake g++ extra-cmake-modules qt6-tools-dev kwin-x11-dev libkf6configwidgets-dev gettext libkf6crash-dev libkf6globalaccel-dev libkf6kio-dev libkf6service-dev libkf6notifications-dev libkf6kcmutils-dev libkdecorations3-dev libxcb-composite0-dev libxcb-randr0-dev libxcb-shm0-dev libxcb-res0-dev libxcb-sync-dev
  ```
</details>

<details>
  <summary>Fedora 41, 42</summary>
  <br>

  Wayland:
  ```
  sudo dnf -y install git cmake extra-cmake-modules gcc-g++ kf6-kwindowsystem-devel plasma-workspace-devel libplasma-devel qt6-qtbase-private-devel qt6-qtbase-devel cmake kwin-devel extra-cmake-modules kwin-devel kf6-knotifications-devel kf6-kio-devel kf6-kcrash-devel kf6-ki18n-devel kf6-kguiaddons-devel libepoxy-devel kf6-kglobalaccel-devel kf6-kcmutils-devel kf6-kconfigwidgets-devel kf6-kdeclarative-devel kdecoration-devel kf6-kglobalaccel kf6-kdeclarative libplasma kf6-kio qt6-qtbase kf6-kguiaddons kf6-ki18n wayland-devel libdrm-devel
  ```

  X11:
  ```
  sudo dnf -y install git cmake extra-cmake-modules gcc-g++ kf6-kwindowsystem-devel plasma-workspace-devel libplasma-devel qt6-qtbase-private-devel qt6-qtbase-devel cmake extra-cmake-modules kf6-knotifications-devel kf6-kio-devel kf6-kcrash-devel kf6-ki18n-devel kf6-kguiaddons-devel libepoxy-devel kf6-kglobalaccel-devel kf6-kcmutils-devel kf6-kconfigwidgets-devel kf6-kdeclarative-devel kdecoration-devel kf6-kglobalaccel kf6-kdeclarative libplasma kf6-kio qt6-qtbase kf6-kguiaddons kf6-ki18n wayland-devel libdrm-devel kwin-x11-devel
  ```
</details>

<details>
  <summary>openSUSE</summary>
  <br>

  Wayland:
  ```
  sudo zypper in -y git cmake-full gcc-c++ kf6-extra-cmake-modules kcoreaddons-devel kguiaddons-devel kconfigwidgets-devel kwindowsystem-devel ki18n-devel kiconthemes-devel kpackage-devel frameworkintegration-devel kcmutils-devel kirigami2-devel "cmake(KF6Config)" "cmake(KF6CoreAddons)" "cmake(KF6FrameworkIntegration)" "cmake(KF6GuiAddons)" "cmake(KF6I18n)" "cmake(KF6KCMUtils)" "cmake(KF6KirigamiPlatform)" "cmake(KF6WindowSystem)" "cmake(Qt6Core)" "cmake(Qt6DBus)" "cmake(Qt6Quick)" "cmake(Qt6Svg)" "cmake(Qt6Widgets)" "cmake(Qt6Xml)" "cmake(Qt6UiTools)" "cmake(KF6Crash)" "cmake(KF6GlobalAccel)" "cmake(KF6KIO)" "cmake(KF6Service)" "cmake(KF6Notifications)" libepoxy-devel kwin6-devel
  ```

  X11:
  ```
  sudo zypper in -y git cmake-full gcc-c++ kf6-extra-cmake-modules kcoreaddons-devel kguiaddons-devel kconfigwidgets-devel kwindowsystem-devel ki18n-devel kiconthemes-devel kpackage-devel frameworkintegration-devel kcmutils-devel kirigami2-devel "cmake(KF6Config)" "cmake(KF6CoreAddons)" "cmake(KF6FrameworkIntegration)" "cmake(KF6GuiAddons)" "cmake(KF6I18n)" "cmake(KF6KCMUtils)" "cmake(KF6KirigamiPlatform)" "cmake(KF6WindowSystem)" "cmake(Qt6Core)" "cmake(Qt6DBus)" "cmake(Qt6Quick)" "cmake(Qt6Svg)" "cmake(Qt6Widgets)" "cmake(Qt6Xml)" "cmake(Qt6UiTools)" "cmake(KF6Crash)" "cmake(KF6GlobalAccel)" "cmake(KF6KIO)" "cmake(KF6Service)" "cmake(KF6Notifications)" libepoxy-devel kwin6-x11-devel
  ```
</details>

### Building

When building under normal conditions, simply clone the repo and run the included build script.

```sh
git clone https://github.com/slynobody/liquid-plasma_better_blur
cd kwin-effects-better-blur-dx
chmod +x build.sh
./build.sh
```

By default, this will build and install the effect for the regular (Wayland) KWin.
To build a version for KWin X11, run the script with the `--x11` flag.

<details>
  <summary>Building on Fedora Kinoite</summary>
  <br>

  When building for Fedora Kinoite, run the build script with the `--kinoite` flag inside your container to generate the RPM package.

  ```sh
  # enter container
  git clone https://github.com/slynobody/liquid-plasma_better_blur
  cd kwin-effects-better-blur-dx
  chmod +x build.sh
  ./build.sh --kinoite
  exit # exit container
  sudo rpm-ostree install kwin-effects-better-blur-dx/build/kwin-better-blur-dx.rpm
  ```
</details>

**Rerun the build script when rebuilding the effect.**

# Usage
This effect conflicts with the default KWin blur effect (and other effects replacing it).

1. Install the plugin.
2. Open the *Desktop Effects* page in *System Settings*.
3. Disable any blur effects.
4. Enable the *Better Blur DX* effect.

### Window transparency
The window needs to be translucent in order for the blur to be visible. This can be done in multiple ways:
- Use a transparent theme for the program if it supports it
- Use a transparent color scheme, such as [Alpha](https://store.kde.org/p/1972214)
- Create a window rule that reduces the window opacity

### Obtaining window classes
The classes of windows to blur can be specified in the effect settings. You can obtain them in two ways:
  - Run ``qdbus org.kde.KWin /KWin org.kde.KWin.queryWindowInfo`` and click on the window. You can use either *resourceClass* or *resourceName*.
  - Right click on the titlebar, go to *More Options* and *Configure Special Window/Application Settings*. The class can be found at *Window class (application)*. If there is a space, for example *Navigator firefox*, you can use either *Navigator* or *firefox*.

# Known Issues
## Incompatibility with other effects
This effect has some compatibility issues with some other effects.

- "Blur" from KWin - Because we effectively replace the KWin blur you shouldn't use both in parallel. Some windows might get double blurred and "look off" if you do.
- "Wobbly Windows" from KWin - We're blurring a square region behind the window. Either that square will bleed out of the deformed window or KWin will skip the blur entirely while the "wobble effect" is active.

## nothing changed
* do a reboot
* use a theme supporting the effect like [this](https://www.pling.com/p/2328884)
* Go to *System Settings* > *Window Management* > *Desktop Effects* > '*Better Blur*' : turn it on
* Go to *System Settings* > *Window Management* > *Desktop Effects* > '*Blur*' : turn it off
* Toggle *Better Blur* Configure icon > '*Force blur*' > activate everything > esp 'Blur all except matching'
* Try putting a *Konsole* Window transparent and move it around.

## High cursor latency or stuttering on Wayland
This effect can be very resource-intensive if you have a lot of windows open. On Wayland, high GPU load may result in higher cursor latency or even stuttering. If that bothers you, set the following environment variable: ``KWIN_DRM_NO_AMS=1``. If that's not enough, try enabling or disabling the software cursor by also setting ``KWIN_FORCE_SW_CURSOR=0`` or ``KWIN_FORCE_SW_CURSOR=1``.

Intel GPUs use software cursor by default due to [this bug](https://gitlab.freedesktop.org/drm/intel/-/issues/9571), however it doesn't seem to affect all GPUs.

 
 # (legacy) install (arch, plasma 6.7.2, QT 6.10.2)
> wget https://github.com/slynobody/liquid-plasma_better_blur/releases/download/experimental/betterblur_liquid_kde6-1_2.4a-1-x86_64.pkg.tar.zst
> 
> sudo pacman -U ./betterblur_liquid_kde6-1_2.4a-1-x86_64.pkg.tar.zst
> 
> wget https://raw.githubusercontent.com/slynobody/liquid-plasma_better_blur/refs/heads/main/defaults
>
> cp ~/.config/kwinrc ~/.config/kwinrc_bu
> 
> cat ./defaults > ~/.config/kwinrc
> 
> kcmshell6 kcm_kwin_effects


<details>
  <summary>older (Arch)</summary>
  
 # install (arch, plasma 6.5.4)
> wget https://github.com/slynobody/liquid-plasma_better_blur/releases/download/1%2C6d_arch/betterblur_liquid_kde6-1.1.6f-1-x86_64.pkg.tar.zst
> 
> sudo pacman -U ./betterblur_liquid_kde6-1.1.6f-1-x86_64.pkg.tar.zst
> 
> wget https://raw.githubusercontent.com/slynobody/liquid-plasma_better_blur/refs/heads/main/defaults
>
> cp ~/.config/kwinrc ~/.config/kwinrc_bu
> 
> cat ./defaults > ~/.config/kwinrc
> 
> kcmshell6 kcm_kwin_effects
> 
</details> 

# Credits
- [a-parhom/LightlyShaders](https://github.com/a-parhom/LightlyShaders) - CMakeLists.txt files
- [taj-ny/kwin-effects-forceblur](https://github.com/taj-ny/kwin-effects-forceblur) - The original Better Blur
- [xarblu/kwin-effects-better-blur-dx](https://github.com/xarblu/kwin-effects-better-blur-dx) - Better Blur DX
- [GB/BetterWobblyWindows](https://github.com/GoldenBilly/BetterWobblyWindows) - Better Blur DX


# Disclaimer
1. Use at your own risk!
2. This is for educational and research purposes only!
3. No responsibility taken for any local customizations of the git!


<a href="https://artsandculture.google.com/experiment/viola-the-bird/nAEJVwNkp-FnrQ?cp=e30."><img src="https://images.pling.com/img/00/00/78/78/79/2160403/proxy-image1.jpeg"/></a>
