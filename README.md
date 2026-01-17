# Holocure Sandbox Mod
A Holocure mod that adds sandbox tools to make it easier to track dps and create builds.
# Follow these instructions if you've used mods before December 2025
- Run `AurieManager.exe` and uninstall Aurie from `HoloCure.exe`
    - The latest version of Aurie is moving away from AurieManager and is instead patching the game to run the mods. This has the benefit of not requiring admin privileges anymore and easily disabling mods by deleting the mods folder or replacing the original exe without crashing.
# Normal installation steps
- Download `HoloCureSandboxMod.dll`, `CallbackManagerMod.dll`, `AurieCore.dll`, and `YYToolkit.dll` from the latest version of the mod https://github.com/PippleCultist/HoloCureMultiplayerMod/releases
- Download `AurieInstaller.exe` from the latest version of Aurie https://github.com/AurieFramework/Aurie/releases
- Launch `AurieInstaller.exe`, click `Find my game!`, and select `HoloCure.exe`
    - You can find `HoloCure.exe` through Steam by clicking `Browse local files`
- Click `Confirm Version`
- Go to the `mods` folder where `HoloCure.exe` is located and locate the `Aurie` folder and `Native` folder.
    - In the `Aurie` folder, replace `YYToolkit.dll` and copy over `HoloCureSandboxMod.dll` and `CallbackManagerMod.dll`
    - In the `Native` folder, replace `AurieCore.dll`
- Running the game either using the executable or through Steam should now launch the mods as well
## Sandbox Menu Buttons
- Press `Y` to open the sandbox menu. This contains all your currently unlocked weapons and items.
- Press `P` to pause all the enemies.
- Press `N` to pause the game and enable per frame increment
- Press `M` while the game is paused to go to the next frame
