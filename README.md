# lofi-buddy

## Overview

Desktop buddy pixel art lofi girl music player.

## Todo

### Features

- [ ] Playlist integration
    - Playlist dir that stores m3u8 files
        - Can import m3u8 files
- Menu system
    - [X] Click on girl's head, she looks up and sees the menu items above her head
        - Basically done beside the art and animation
    - [X] Background
    - Entries
        - [X] Quit
        - [ ] Settings
            - [ ] Writes to .config or equivalent
            - [ ] Entries
                - [ ] Playlist directory location
                - [ ] Window size
                - [ ] Enable desktop buddy system (ie just render as a normal window)
                    - [ ] Need to have a sprite or colour for the non-transparent background
        - [ ] Playlist manager
            - [ ] Add file(s), folder(s) or URL to playlist
            - [ ] Rearrange files in playlist
            - [ ] Switch playlist
            - [ ] Save current playlist to m3u8 file
- [ ] Media control
    - [ ] Appears on the side of the desk
    - [ ] Features
        - [ ] Play/pause toggle button
        - [ ] Skip next/prev
        - [ ] Repeat single/playlist 
        - [ ] Shuffle
        - [ ] Mute
        - [ ] Volume
        - [ ] Progress??

### Implementation

- [ ] File dialog
    - [ ] Dialog
    - [ ] Ensure cross platform
    - [ ] Allow selecting folders or files
- [ ] Button class
    - [ ] Hover and pressed sprites
    - [ ] Pressed state
        - [ ] Check sprint bounds with mouse pos
- [ ] Text input control
    - [ ] https://stackoverflow.com/a/53765163
    - [X] Need font
- [ ] Art
    - [ ] Girl head and body
    - [ ] Desk and decorations
    - [ ] Cat
- [ ] Animation
    - [ ] Some idle animation of the girl's head and body moving slightly
    - [ ] Girl's head looking up when accessing menu
    - [ ] Cat movement
- [ ] Improve performance
    - [ ] Trigger transparency only when sprites change (animation)
    - [ ] Trigger bringToTop only when window manager does something
- [ ] Possible debug mode that can print information about the current song and state to the console every second
- [ ] Separate transparency and bringToTop code into related classes for each OS rather than using preprocessors
