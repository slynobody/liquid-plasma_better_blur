# liquid glassy (linux, interface-effect, precompiled)
<img width="262" height="114" alt="screenshot-20250908-1902001" src="https://github.com/user-attachments/assets/f3bc40d7-e854-4ae4-8c4f-f2a8738df662" />
<img width="262" height="114" alt="screenshot" src="https://github.com/user-attachments/assets/6d799362-bd51-43f9-9d80-1ca331876460" />
<noscript><a href="https://liberapay.com/integr/donate"><img alt="Donate using Liberapay" src="https://liberapay.com/assets/widgets/donate.svg"></a></noscript>

## dynamic "liquid glassy" ui-effect
### ('better-blur' plasma-extension, incl. currently unmerged 'lens-refraction'-code)
![vokoscreenNG-2025-12-04_16-21-39-ezgif com-video-to-gif-converter](https://github.com/user-attachments/assets/26c3578d-b37d-495d-9e78-0d6f556c68ab)

(1.6a - req.: linux, kde-plasma: 6.3.6+, qt: 6.8.2+) > trixie

(1.6c - req.: linux, arch, kde-plasma: 6.4+, qt: 6.9+)

(1.6d - req.: linux, deb, kde-plasma: 6.5.1, qt: 6.9.2)

(1.6d - req.: linux, deb, kde-plasma: 6.5.2, qt: 6.9.2) > forky

(1.6e - req.: linux, deb, kde-plasma: 6.5.3, qt: 6.9.2) > sid

(1.6e - req.: linux, arch, kde-plasma: 6.5.3, qt: 6.9.2)

*precompiled* for easy installation

source: https://github.com/iGerman00/kwin-effects-forceblur


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

# install (debian, sid / experimental)
> wget https://github.com/slynobody/liquid-plasma_better_blur/releases/download/1.6d2_deb/int_better_blur_liquid_1-6_5-3_amd64.deb
> 
> sudo apt install ./int_better_blur_liquid_1-6_53_amd64.deb
> 
> wget https://raw.githubusercontent.com/slynobody/liquid-plasma_better_blur/refs/heads/main/defaults
>
> cp ~/.config/kwinrc ~/.config/kwinrc_bu
> 
> cat ./defaults > ~/.config/kwinrc

# install (arch, plasma 6.5.3)
> wget https://github.com/slynobody/liquid-plasma_better_blur/releases/download/1%2C6d_arch/betterblur_liquid_kde6-1.1.6e-1-x86_64.pkg.tar.zst
> 
> sudo pacman -U ./betterblur_liquid_kde6-1.1.6e-1-x86_64.pkg.tar.zst
> 
> wget https://raw.githubusercontent.com/slynobody/liquid-plasma_better_blur/refs/heads/main/defaults
>
> cp ~/.config/kwinrc ~/.config/kwinrc_bu
> 
> cat ./defaults > ~/.config/kwinrc
> 
> kcmshell6 kcm_kwin_effects


# useage
simply 
1. *dis*able other / common 'blur' & 'transparency' - effects (settings-window)
2. *en*able 'better blur' (settings-window)
3. use a global theme that supports translucency, like [this](https://www.pling.com/p/2328884)

# nothing changed
* do a reboot
* use a theme supporting like [this](https://www.pling.com/p/2328884)
* Go to *System Settings* > *Window Management* > *Desktop Effects* > '*Better Blur*' : turn it on
* Go to *System Settings* > *Window Management* > *Desktop Effects* > '*Blur*' : turn it off
* Toggle *Better Blur* Configure icon > '*Force blur*' > activate everything > esp 'Blur all except matching'
* Try putting a *Konsole* Window transparent and move it around.

# Disclaimer
1. Use at your own risk!
2. This is for educational and research purposes only!
3. No responsibility taken for any local customizations of the git!


<a href="https://artsandculture.google.com/experiment/viola-the-bird/nAEJVwNkp-FnrQ?cp=e30."><img src="https://images.pling.com/img/00/00/78/78/79/2160403/proxy-image1.jpeg"/></a>
