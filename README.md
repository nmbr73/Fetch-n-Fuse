# Fetch'n'Fuse

The idea is to **Fetch a Shader** ABC via ...
```
./fetch -id ABC
```
... then use the "Incubator.fuse" to convert the code until it runs smoothly, to then do a
```
./fuse -id ABC
```
... to **Make it a Fuse**.

## Project Status

The `fetch` does fetch some code and the Incubator.fuse is able to show the shader (still a lot of crashes to be expected). Next step will be to implement the `fuse` command (does nothing yet).

## Known Limitations

The pre-conversion is very basic (not a C parser, but just some simple regex string substitutions) and does some wrong text replacements - but still it's better then doing it all by hand.

The `fetch` command does work only for shaders that had been marked by their aurhors for API publication - and it seems that quite a lot of shaders are restricted. :worried:

## Setup

* Drag'n'drop the `install.lua` onto your Fusion composition pane.
* Restart DaFusion.

But the whole thing is in a very early stage and will probably not work very well.

Then `cd` into your working copy of the repository and do a:
```
python3 -m pip install -r requirements.txt
chmod a+x fetch fuse
./fetch
```

In particular on Windows machines it might be (but I'm just guessing):

```
python -m pip install -r requirements.txt
python ./fetch
```

Now edit the `.env` file to add you credentials. You need to log in to shadertoxs.com and create an APP on https://www.shadertoy.com/myapps to rtrieve your API-Key.

## Fetch a Shader

In your working copy do (on Windows as parameters to `python3` maybe) a ...
```
./fetch --id <TOYID>
```
... with *&lt;TOYID&gt;* the shadertoy ID as it is used in the URLs. If the shader could be fetched (and this is not all to often the case), then a corresponding source file should have been created under `Conversions/`.

## Show in DaFusion

* Add a 'ShadersInc' Fuse to you composition.
* This should allow you to load all 'Conversions/*&lt;FUSENAME&gt;*.*&lt;TOYID&gt;*.c' files.

## Make it a Fuse

:construction: WORK IN PROGRESS :construction:<br />*does nothing yet ...* :construction_worker: