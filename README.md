# liquid glassy (linux, interface-upgrade)
<img src="blur.png"/>

## dynamic "liquid glassy" ui-effect ('better-blur' plasma-extension)
[video.webm](https://github.com/user-attachments/assets/ddb1f5be-c153-4187-a3bc-98f0710da18a)

(req.: linux, kde-plasma: 6.3.6+, qt: 6.8.2+)
*precompiled* for easy installation

source: https://github.com/iGerman00/kwin-effects-forceblur

# install (debian)
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
   

# useage
simply 
1. *dis*able old 'blur' effect (settings-window)
2. *en*able 'better blur' (settings-window)
3. use a theme that supports translucency, like [this](https://github.com/vinceliuice/MacTahoe-kde)

# nothing changed
* do a reboot
* use a theme supporting like [this](https://github.com/vinceliuice/MacTahoe-kde)
* Go to *System Settings* > *Window Management* > *Desktop Effects* > '*Better Blur*' : turn it on
* Go to *System Settings* > *Window Management* > *Desktop Effects* > '*Blur*' : turn it off
* Toggle *Better Blur* Configure icon > '*Force blur*' > activate everything > esp 'Blur all except matching'
* Try putting a *Konsole* Window transparent and move it around.

# Disclaimer
1. Use at your own risk!
2. This is for educational and research purposes only!
3. No responsibility taken for any local customizations of the git!


<a href="https://artsandculture.google.com/experiment/viola-the-bird/nAEJVwNkp-FnrQ?cp=e30."><img src="https://images.pling.com/img/00/00/78/78/79/2160403/proxy-image1.jpeg"/></a>
