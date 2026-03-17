**Fork with extended functionality for OpenRGB-Ambient plugin.**
=======================================================

1. Device zones toggles have been added. Now you can specify zones to be and not to be affected.
   Useful if i.e. you'd like to have audio visualization on separate zone on the same device
2. Added zone parts (Segments) to be mapped with possible reverse to the frame. Useful if you have ∩-shaped one-strip backlight.
3. Adjusted preview to work in HDR mode and show led colors.

<img width="856" height="658" alt="image" src="https://github.com/user-attachments/assets/819f6fc0-a45d-4f09-b262-fd5767dcfc45" />


Be aware: 75% of AI slop by Claude has been used!


Forked from and updated 
https://github.com/krojew/OpenRGB-Ambient.
Original readme is below ⬇️
======================================================

OpenRGB plugin inspired by NZXT HUE 2 Ambient Lighting, although it works with anything supported by OpenRGB.

Note: this plugin in no way endorses NZXT or their products, especially given their [anti-consumer practices](https://www.youtube.com/watch?v=0pomC1CfpC0).

NZXT CAM, at the moment, is very buggy. This plugin provides a lighter and more stable alternative:

* no flickering
* support for HDR and SDR
* compensation for NZXT cool white LEDs
* stable 60Hz refresh rate
* shuts LEDs down along with the OS
* less resource usage
* configurable color temperature

Note: currently confirmed to be working on Windows 10 and Windows 11.

Versions older than **2.0.0** should be used with OpenRGB **0.6**, while **2.3** should work with
OpenRGB **0.7** or **0.8**, and **2.4** is compatible with OpenRGB **0.9**; version **3** and up works with OpenRGB 
**1.0**. Yes - this is a mess, blame OpenRGB and their lack of stable plugin API.
