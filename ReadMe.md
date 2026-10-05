# DOOMENSTEIN

To compile this project, engine and the game must be in the same folder and organized as shown below

```
Workspace/
├── Engine/
│   └── Code/
├── Doomestein/
│   ├── Code/
|   └── Run/
```

- Launch Doomenstein_Release_x64.exe in Run folder to play the game

## How To Play:

- The map has four corner rooms - Green, Yellow, Blue and Red - and a courtyard in the center.
- Stand inside a corner room to capture it. Capture progress ticks up while you are inside and
  drains back down the moment you leave, so you have to hold the room.
- Rooms can only be captured at night. When the room lights are on, that is your capture window
  (in game hours 22:00 - 06:00). During the day the rooms are dark to capture and the sun is up.
- One in-game day lasts about 200 real seconds.
- Capture all four corner rooms first, then capture the courtyard to win.
- Demons respawn at 00:00 every day, so every night you clear is a night they come back.
- Dying 5 times is game over.

Enemies

- Demon - 160 health, melee, fast
- Heavy Demon - 350 health, larger, faster, stronger and tankier version of the Demon

Weapons

- Pistol - accurate, single shot, hitscan
- Plasma Rifle - fast firing, projectile

## Controls

Menus (Attract / Lobby / Briefing)

- Space Bar to advance (Attract -> Lobby -> Briefing -> Game)
- ESC to back out one screen, or to close the app from the attract screen
- A / START on the controller to advance
- B / BACK on the controller to back out

Keyboard Controls

- WSAD to move
- Mouse to look
- Left Shift to sprint
- Space Bar to jump
- Left Mouse to shoot
- 1 to equip Pistol, 2 to equip Plasma Rifle
- Left Arrow / Right Arrow or Scroll Wheel to cycle weapons
- P to pause game
- ESC to go back to attract mode

XBOX Controller

- Left stick to move
- Right stick to look
- A or Left Thumb to sprint
- Right Trigger to shoot
- X to equip Pistol, Y to equip Plasma Rifle
- D-Pad Left / D-Pad Right to cycle weapons
- START to pause game
- BACK to go back to attract mode

DEV CHEATS

- F to toggle free-fly camera (single player only)
- N to debug possess the next actor in the map
- T to toggle slow motion (0.1x)
- O to step a single frame
- F11 to hot reload Data/GameConfig.xml
- ~ (tilde) to open the dev console

Dev Console Commands

- ControlLights to toggle manual control of the sun (needed for the sun keys below)
- SunSettings to toggle the sun debug overlay
- KillAllActors to clear every actor in the map
- ShowGrid to toggle the world grid
- DebugDraw to toggle debug rendering
- DebugUI to toggle the debug UI
- Quit to close the app

Sun Controls (only active after running ControlLights)

- Up Arrow / Down Arrow to change sun pitch
- Left Arrow / Right Arrow to change sun yaw
- F6 / F7 to decrease / increase sun intensity
- F8 / F9 to decrease / increase ambient intensity

Note: the arrow keys double as weapon cycling, so expect your weapon to swap while
you are steering the sun.
