# Fetch'n'Fuse

The idea is to **Fetch a Shader** ABC via ...
```
./fetch -id ABC
```
... then use the "Incubator.fuse" to **Convert the Code** until it runs smoothly, to then do a
```
./fuse -id ABC
```
... to finally **Make it a Fuse**.

## Project Status

The `fetch` does fetch some code and the Incubator.fuse is able to show the shader. The `fuse` command is work in progress and its results are not usable yet.

> **You see:** The whole thing is obviously in a very early stage and will probably not work all too well well! So be kind, have some patience and most importantly use at your very own risk!


## Known Limitations

The pre-conversion is very basic (not a C parser, but just some simple regex string substitutions) and does some wrong text replacements - but still it's better then doing it all by hand.

The `fetch` command does work only for shaders that had been marked by their aurhors for API publication - and it seems that quite a lot of shaders are restricted in this regard :worried:


## Setup

* Drag'n'drop the `install.lua` onto your Fusion composition pane
* Restart Fusion resp. DaVinci Resolve

> **Note:** Whenever you move the repository, get updates on the files within the `.clobber/` folder - or just from time to time - do a drag and drop of the install.lua to be equipped with a fresh version of this fuse. Alternatively (and given your `.env` does define the needed pathes) you can run the `install.py` from within the repository's folder.

Then `cd` into your working copy of the repository and do a ...
```
python3 -m pip install -r requirements.txt
chmod a+x fetch fuse
./fetch
```
... or something that does the equivalent on your system.

Now edit the `.env` file to add you credentials. You need to log in to shadertoys.com and create an APP on https://www.shadertoy.com/myapps to retrieve your own API-Key.

It should look something like this afterwards:
```
AUTHOR="nmbr73"
APIKEY="******"
DOWNLOADS="/Users/nmbr73/Downloads/"
FUSEPATH="/Users/nmbr73/Library/Application Support/Blackmagic Design/Fusion/Fuses/"
REPOPATH="/Users/nmbr73/Projects/Fetch-n-Fuse/"
```
... `DOWNLOADS` is optional, but it's recommended to set it to your browsers download folder (do not forget the trailing slash!). If set, then the downloads folder is used to search for shaders - otherwise you have to copy such downloads into the `Conversions/` folder before calling `fetch`.


# Make a Fuse

As already mentioned ...

## Fetch a Shader

In your working copy (on some sytsmes as a parameters to `python3` maybe) do a ...
```
./fetch --id <TOYID>
```
... with *&lt;TOYID&gt;* the shadertoy ID as it is used in the URLs. If the shader could be fetched (and regretably it often cannot be fetched), then a corresponding source file should have been created under `Conversions/`.

## Convert the Code

* Add a 'ShadersInc' Fuse to you composition
* This should allow you to load all 'Conversions/*&lt;FUSENAME&gt;*.*&lt;TOYID&gt;*.c' files.

And then the fun begins of porting WebGL to DCTL and eliminating all the errors raised whe DaFusion tries to render the node.

## Make it a Fuse

As soon as your conversion works without errors, is equipped with some nice controls and leads to the expected output, you are ready to do a ...
```
./fetch --id <TOYID>
```
... which should create a fuse and a markdown file in the `Converted/` folder.

:construction: WORK IN PROGRESS :construction:<br />*does not much yet ...* :construction_worker:

## Post Processing and Publication

:construction:
...