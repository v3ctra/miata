# Miata
[![C++](https://img.shields.io/badge/language-C%2B%2B-%23f34b7d.svg?style=plastic)](https://en.wikipedia.org/wiki/C%2B%2B) 
[![GitHub issues](https://img.shields.io/github/issues/v3ctra/miata?color=green)](https://github.com/v3ctra/miata/issues)

Free and open-source radar-hack software for Counter-Strike 2.

## Example Config
If config.txt doesn't exist - cheat will use default values, defined in entrypoint.
```
radar.pos.x=5       # x position of radar on the screen
radar.pos.y=5       # y position of radar on the screen
radar.size.w=128    # width of the radar's window
radar.size.h=128    # height of the radar's window
cheat.window.hide=0 # Determines if window should be hidden on the screenshots and recordings
```

## Updating
Cheat will perform an attempt to read file called "offsets.txt". The contents of the file should look like:
```
offsets.client.dwLocalPlayerPawn=29302688
offsets.client.dwEntityList=30500696
offsets.client.dwViewAngles=31712480
offsets.client.m_hPlayerPawn=2300
offsets.client.m_iHealth=844
offsets.client.m_iTeamNum=1003
offsets.client.m_vOldOrigin=5560
```
Updated offsets can be found [HERE](https://github.com/a2x/cs2-dumper/blob/main/output/offsets.json). and [HERE](https://github.com/a2x/cs2-dumper/blob/main/output/client_dll.json).
