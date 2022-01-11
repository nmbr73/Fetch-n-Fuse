# iChannel Sources

Some Fuses to support the creation of compositions with Fuses derived from a shadertoy fragment shader. If your working copy resides for example in '`/Users/nmbr73/Projects/Fetch-n-Fuse/`' then you can link them into Fusion (on a Mac) by ...
```
cd '/Users/nmbr73/Library/Application Support/Blackmagic Design/Fusion/'
ln -s /Users/nmbr73/Projects/Fetch-n-Fuse/Fuses/Keyboard_nmbr73.fuse Keyboard_nmbr73.fuse
ln -s /Users/nmbr73/Projects/Fetch-n-Fuse/Fuses/Texture_nmbr73.fuse Texture_nmbr73.fuse
```
resp. for Resolve
```
cd '/Users/nmbr73/Library/Application Support/Blackmagic Design/DaVinci Resolve/Fusion/Fuses'
ln -s /Users/nmbr73/Projects/Fetch-n-Fuse/Fuses/Keyboard_nmbr73.fuse Keyboard_nmbr73.fuse
ln -s /Users/nmbr73/Projects/Fetch-n-Fuse/Fuses/Texture_nmbr73.fuse Texture_nmbr73.fuse
```
... or whatever that is on your system.


## Keyboard_nmbr73.fuse

To simulate keyboard inputs. In therory this should be an easy one, but it does nit work yet. Don't mind the stupid '_nmbr73' filename: it's just to avoid name conflicts (texture and such things might be pretty common) and you won't see it in the Fusion UI.

## Texture_nmbr73.fuse

To provide a Fuse with the usualy used textures. Needs all the files under [`Assets/media/a/`](../Assets/media/a/) and [`Assets/media/ap/`](../Assets/media/a/) to work.
