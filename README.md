

![image](https://github.com/RafaelMicro/RT58x_SDK/blob/main/Docs/RT58x_SDK_Reference_Guide/logo.JPG)


## ![latest tag](https://img.shields.io/badge/Release-v1.9.4-color=brightgreen) RT58x SDK Release Note
 About the content of Software Release Note
[RT58x SDK Release Note](https://github.com/RafaelMicro/RT58x_SDK/blob/main/RT58x_SDK_Release_Notes.pdf "link")

## Evaluation Kit Board
The RT58x EVK provides an SWD interface with connector for use with an ICE debugger (J-Link Adapter) via 20pin IDE cable.

Reference [RT58x_SOC_Platform_Getting_Started_V1.4.pdf](https://github.com/RafaelMicro/RT58x_SDK/blob/main/Docs/%5BSW_01%5DRT58x_SOC_Platform_Getting_Started_V1.4.pdf "link")

Development Board 
![image](https://github.com/RafaelMicro/RT58x_SDK/blob/main/Docs/RT58x_SDK_Reference_Guide/RT58X_EVK.jpg) 

## Software Development Kit
 Rafael RT58x SDK is a complete software development kit for application development such as BLE, Zigbee, Mesh, and Sub-G.
 Rafael RT58x SDK is specifically designed for Rafael’s SoC with the ability to perform high-performance Cortex-M microcontroller and support to driver powerful RF and peripheral features.

   ##### Git clone option as shown in the example below.
 
 ```bash
git clone --recursive https://github.com/RafaelMicro/RT58x_SDK.git
```

  ##### Download reposiory Zip
Downloading the repository as a ZIP file will not include the submodules along with the rest of the firmware.
Therefore, you will need to manually download the submodules separately and add them to the firmware.

 ## SDK Folder Struct
 
 The following item shows the related files and directories in the Rafael RT58x SDK. 
 These files and directories contain project, middleware, library, tools, and documents.
 ```bash
 - Docs
 - Library (sub module)
 - Middleware
 - Project
 - Tool
```

```bash
SDK
 |
 |__Docs       Ble/Ble mesh/multi protocol/Zigbee/Thread application, development tooldocument
 |__Library    include RF,peripheral driver, startup document 
 |__Middleware zigbee/ble/fota/prebuild libary/third party/ bsp/bootloader file
 |__Project    usage Application and Peripheral sample code.
 |__Tool       ble/zigbee application app (android/ios) , MP Tool, ISP Tool, flash algorithm   
```
