[![Buy Me A Coffee](https://img.shields.io/badge/Buy%20Me%20a%20Coffee-ffdd00?&logo=buy-me-a-coffee&logoColor=black)](https://ko-fi.com/integr)

# better blur dx+ (linux, interface-effect, *precompiled*)
<img width="468" height="216" alt="ezgif-2791664fde5198af" src="https://github.com/user-attachments/assets/1b245ea7-5e8f-4c0f-9cd5-7d6458cf8d19" />

## dynamic "liquid" ui-effect for kde plasma
### ('better-blur dx' plasma-extension,)
![vokoscreenNG-2025-12-04_16-21-39-ezgif com-video-to-gif-converter](https://github.com/user-attachments/assets/26c3578d-b37d-495d-9e78-0d6f556c68ab)

forked from: https://github.com/xarblu/kwin-effects-better-blur-dx

*precompiled* for easy installation

experimental: added new features (border hightlight, mouse highlight) > 2.6


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
 
 # install (arch, plasma 6.7.2, QT 6.10.2)
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


# usage
simply 
1. *dis*able other / common 'blur' & 'transparency' - effects (settings-window)
2. *en*able 'better blur dx' (settings-window)
3. use a global theme that supports translucency, like [this](https://www.pling.com/p/2328884)

# nothing changed
* do a reboot
* use a theme supporting the effect like [this](https://www.pling.com/p/2328884)
* Go to *System Settings* > *Window Management* > *Desktop Effects* > '*Better Blur*' : turn it on
* Go to *System Settings* > *Window Management* > *Desktop Effects* > '*Blur*' : turn it off
* Toggle *Better Blur* Configure icon > '*Force blur*' > activate everything > esp 'Blur all except matching'
* Try putting a *Konsole* Window transparent and move it around.

# versions
(2.4a - req.: linux, deb, kde-plasma: 6.7.2, qt: 6.10.2) > debian: sid
 2.4a - req.: linux, arch, kde-plasma: 6.7.2, qt: 6.10.2) 
 
<details>
  <summary>🔽older</summary>
  
(2.23e - req.: linux, deb, kde-plasma: 6.6.5, qt: 6.10.2) > debian: forky)

(1.6f - req.: linux, arch, kde-plasma: 6.5.4, qt: 6.9.2)

(1.6f - req.: linux, deb, kde-plasma: 6.5.4, qt: 6.9.2)

(1.6e - req.: linux, arch, kde-plasma: 6.5.3, qt: 6.9.2)

(1.6e - req.: linux, deb, kde-plasma: 6.5.3, qt: 6.9.2)

(1.6c - req.: linux, arch, kde-plasma: 6.4+, qt: 6.9+)

(1.6a - req.: linux, kde-plasma: 6.3.6, qt: 6.8.2) > ubuntu: universe / debian: trixie

</details> 



# Disclaimer
1. Use at your own risk!
2. This is for educational and research purposes only!
3. No responsibility taken for any local customizations of the git!


<a href="https://artsandculture.google.com/experiment/viola-the-bird/nAEJVwNkp-FnrQ?cp=e30."><img src="https://images.pling.com/img/00/00/78/78/79/2160403/proxy-image1.jpeg"/></a>
