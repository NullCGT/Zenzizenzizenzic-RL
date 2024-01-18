# Zenzizenzizenzic

![Release Status](https://github.com/NullCGT/Zenzizenzizenzic-RL/actions/workflows/cmake-single-platform.yml/badge.svg)

A traditional roguelike with mechanics that mimic those found in fighting games.

![Screenshot](/img/screenshot.png)

## Combat Basics

An attack can be one or more of four types: Low, Mid, High, or Grab. Conversely, a character can be in one of three stances: Crouch, Stand, or Tech. The way an attack interacts with a target depends on the stance that the target is in, as shown in the following table.

|          | Low   | Mid   | High  | Tech  |
|----------|-------|-------|-------|-------|
|**Crouch**| Block | Block | Hit   | Hit   |
|**Stand**| Hit   | Block | Block | Hit   |
|**Tech** | Hit   | Hit   | Hit   | Block |

If an attack hits, the target takes full damage and enters a stunned state. If an attack is blocked, the target takes only partial damage and is not stunned.

## System Requirements

This is a console-based roguelike written in plain C. A moldering potato can probably run this game. The potato, however, must be equipped with the following:

- Ncurses

To download the game, find the latest [release](https://github.com/NullCGT/Zenzizenzizenzic-RL/releases). Download the version that corresponds to your operationg system and unzip it to a new folder. Launch the game in the build directory.

## Building from Source
Zenzizenzizenzic is built using CMake. 

In order to compile the game for local testing, run the following commands:
```
cmake --preset=dev
cmake --build build
```
The game binary and necessary data files will appear in the newly-created build directory. In order to compile the game with release-level compiler optimizations, replace "dev" with "release." Note that this will disable most compiler warnings and debug hooks.

## FAQ

### Is this Playable Yet?

While both the win and lose state are reachable, the game is still very far from what
I would consider playable.

### Why C?

This is a traditional roguelike, and follows the Berlin Interpretation
fairly closely. Why not make it even more traditional by writing it in pure C?

In all honesty, though, I simply like C.

### Are save files compatible across computers?

Short answer: No.

Long answer: It depends. Save files are written in binary with fwrite(). 
This means that the save file architecture depends on one's platform,
operating system, the compiler that the binary was compiled with, and a host
of other factors. It's easiest to assume that a save file made on one computer
will not be compatible with a save file made on another.

In the future, I would like to refactor save files to be in human-readable
json, but that's a long way off.

## Influences

Zenzizenzizenzic was influenced by numerous games, the most prominent of which appear here:

* [NetHack](https://github.com/nethack/nethack)

* [Dungeon Crawl Stone Soup](https://github.com/crawl/crawl)

* [The Slimy Lichmummy](http://www.happyponyland.net/the-slimy-lichmummy)

* [Sil-Q](https://github.com/sil-quirk/sil-q)

