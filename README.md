# Overview​
The StreamUP Hotkey Display will allow you to have a dock in OBS that will show you what keyboard shortcuts are being pressed currently. You can then use the data to send it to a text source in OBS to display the key press.

# Guide​
If you want to find out more about the plugin and how to use it then please check out the [full guide](https://streamup.notion.site/OBS-Hotkey-Display-53b513b427e8425eb584bdc408117daa)!

If you have any feedback or requests for this plugin please share them in the [StreamUP Discord server](https://discord.com/invite/RnDKRaVCEu?).

# Build
1. In-tree build
    - Build OBS Studio: https://obsproject.com/wiki/Install-Instructions
    - Check out this repository to UI/frontend-plugins/obs-hotkey-display
    - Add `add_subdirectory(obs-hotkey-display)` to UI/frontend-plugins//CMakeLists.txt
    - Rebuild OBS Studio

1. Stand-alone build (Linux only)
    - Verify that you have package with development files for OBS
    - Check out this repository and run `cmake -S . -B build -DBUILD_OUT_OF_TREE=On && cmake --build build`

# Support
This plugin is manually maintained by Andi as he updates all the links constantly to make your life easier. Please consider supporting to keep this plugin running!
- [**Patreon**](https://www.patreon.com/Andilippi) - Get access to all my products and more exclusive perks
- [**Ko-Fi**](https://ko-fi.com/andilippi) - Get access to all my products and more exclusive perks
- [**PayPal**](https://www.paypal.me/andilippi) - Send me a beer to say thanks!
- [**Twitch**](https://www.twitch.tv/andilippi) - Come say hi and feel free to ask questions!
- [**YouTube**](https://www.youtube.com/andilippi) - Learn more about OBS and upgrading your streams

