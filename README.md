# Shadertoys Incubator

## Setup

* Drag'n'drop the `install.lua` onto your Fusion composition pane.
* Restart DaFusion.

But the whole thing is in a very early stage and will probably not work all to well.

Then `cd` into your working copy of the repository and do a:
```
chmod a+x fetch
./fetch
```

Now edit the `.env` file to add you credentials.

## Fetch a Shader

In your working copy do a ...
```
./fetch --id <TOYID>
```
... with *&lt;TOYID&gt;* the shadertoy ID as it is used in the URLs. If the shader could be fetched (and this is not all to often the case), then a corresponding source file should have been created under `Conversions/`.

## Show in DaFusion

* Add a 'ShadersInc' Fuse to you composition.
* This should allow you to load all 'Conversions/*&lt;FUSENAME&gt;*.*&lt;TOYID&gt;*.c' files.
