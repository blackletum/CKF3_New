# Chicken Fortress 3, edited by weyouthey and jakulo 

NOTE FROM WYT: Do note that this still has the same big bugs with building anything else but client.dll and mp.dll. Ignore the others as they cannot really be built without doing a bunch of stupid shit that I can't be bothered to finish. Considering it mainly handles rendering, it isn't actually THAT important right now, so dont expect any changes there brah

Chicken Fortress 3 is a Half-Life mod that ported the Team Fortress 2 to GoldSRC engine.

## New changes compared to the original:
##### Bots!
Nav-mesh system in general.
Weapon fixes / readjustments to be more accurate to live Team Fortress 2.

Please check [ModDB Page](https://www.moddb.com/mods/chicken-fortress-3) for more info on the original!

## Setup
(NOTE: this was from original sdk. idk if the new one will work as well considering my new files)

1. Clone this project.
2. Run ".\prepare.bat".
3. Open ".\test\" directory and copy "ckf3" directory to "path\to\Half-Life\ckf3".
4. Open ".\develop\user.props" file and change "ChickenFortressInstallPath" to "path\to\Half-Life\ckf3".
5. Open ".\develop\ChickenFortress.sln" with "Visual Studio".
6. Then run "Build Solution" in "Visual Studio" and everything should build for you.
7. After build all dlls will be copied to your install path.
8. You can now debug or run this mod, Have fun :).

## Contribution

only 2 ppl with the newer version: jakulo and me, weyouthey.

## Credits

- [hzqst](https://github.com/hzqst)
- [goodman3](https://github.com/goodman3)
- stay
- [yuxuanchiadm](https://github.com/yuxuanchiadm)
