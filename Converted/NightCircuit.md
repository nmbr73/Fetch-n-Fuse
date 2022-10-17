# NightCircuit
[![Download Installer](https://img.shields.io/static/v1?label=Download&message=NightCircuitJipi-Installer.lua&color=blue)](NightCircuit-Installer.lua "Installer")

Based on '_[Night circuit JiPi](https://www.shadertoy.com/view/tdyBR1)_' by [gaz](https://www.shadertoy.com/user/gaz) and ported by [JiPi](../../Site/Profiles/JiPi.md).

There is a striking performance difference between Cuda and OpenCL. While the shader is rendered loosely in real time with Cuda, with OpenCL each image is rendered individually to the screen. Possibly the use of the arrays is the cause.



[![Thumbnail](NightCircuit.png)](https://www.shadertoy.com/view/tdyBR1 "View on Shadertoy.com")



## Compatibility
- [ ] Tested on macOS/Metal
- [ ] Tested on macOS/OpenCL
- [x] Tested on Windows/Cuda
- [x] Tested on Windows/OpenCL
