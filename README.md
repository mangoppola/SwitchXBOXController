# SwitchXBOXController
Turn your Nintendo Switch into an XBOX360 controller to play on your Windows computer

![Server](https://puu.sh/BVASI/d8c6c00ecc.png)

## How to use
- Download the latest [ScpDriverInterface](https://github.com/mogzol/ScpDriverInterface) release and use the `ScpDriverInstaller.exe` in there to install the Scp drivers.
- Download the latest SwitchXBOXController release and execute `SwitchXBOXController_Server.exe`. You might have to allow network access in the firewall settings.
- Put the SwitchXBOXController_Client.nro in your `/switch` folder on the SD Card of your Nintendo Switch.
- Start the homebrew application using the hbmenu

## Changes from the WerWolv project
+ You can now underclock the CPU by pressing the left SR button; pressing the button again reverts the changes. This was done because the CPU underclock was causing problems when trying to quit the program.
+ You can now quit the program by pressing the right SL button. This also restores the clock to the default.
+ The home button can be pressed by using the touchscreen if in handeld mode, or by using either left SL or right SR.
