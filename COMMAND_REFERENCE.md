# ESP32 Command Reference

A quick reference for commands and workflows used in this ESP-IDF workspace.
Add new entries here as you learn them.

## Open an ESP-IDF terminal in VS Code

1. Press **Ctrl+Shift+P** to open the Command Palette.
2. Search for **ESP-IDF: Open ESP-IDF Terminal** and select it.
3. Verify that ESP-IDF is available:

   ```powershell
   idf.py --version
   ```

This terminal prepares the ESP-IDF tools for building, flashing, and monitoring.

## Activate ESP-IDF in an ordinary PowerShell terminal

This workspace currently selects ESP-IDF 6.1. Activate it with:

```powershell
. C:\esp\v6.1\esp-idf\export.ps1
```

The leading dot followed by a space is intentional. Activation applies to the
current terminal session; activate again when opening a new ordinary terminal.
If the workspace changes ESP-IDF versions, update the path above accordingly.

## Go to the workspace root

Run workspace build commands from this directory:

```powershell
cd C:\Users\noels\workspace_vscode\esp32
```

## Check the ESP-IDF version

```powershell
idf.py --version
```

If PowerShell cannot find `idf.py`, open an ESP-IDF terminal or activate ESP-IDF
using the instructions above.

## Switch ESP-IDF versions (6.0.2 to 6.1)

1. In VS Code, press **Ctrl+Shift+P**.
2. Run **ESP-IDF: Select Current ESP-IDF Version**.
3. Select the installation at `C:\esp\v6.1\esp-idf`. If prompted for where
   to save the setting, choose the workspace folder.
4. Close old terminals and run **ESP-IDF: Open ESP-IDF Terminal**.
5. Run `idf.py --version` and confirm it reports 6.1.

The workspace selection is stored in `.vscode/settings.json` as
`idf.currentSetup`. The ordinary PowerShell activation command for 6.1 is:

```powershell
. C:\esp\v6.1\esp-idf\export.ps1
```

Build into a fresh directory when changing versions. This workspace keeps its
generated `sdkconfig` (including Wi-Fi and MQTT settings) inside each build
directory, so preserve settings before using `-Fullclean`.

To keep the old build intact and try 6.1 with fresh configuration:

```powershell
idf.py -C projects/sensor_device -B projects/sensor_device/build-idf61-xiao -DBOARD=seeed_xiao_esp32c6 menuconfig
idf.py -C projects/sensor_device -B projects/sensor_device/build-idf61-xiao -DBOARD=seeed_xiao_esp32c6 build
```

Enter Wi-Fi and MQTT settings in menuconfig for this new build. Substitute the
board profile if using another board. The regular `scripts/build.ps1` command
continues to use its original board-specific build directory.

Version selection alone does not verify application compatibility. Build and
test the firmware on hardware before considering the migration complete.

## Other workspace help

- [Learning Path](LEARNING_PATH.md): guided lessons and hands-on exercises.
- [Getting Started](GETTING_STARTED.md): build, flash, monitor, and configuration.
- [Project Layout](PROJECT_LAYOUT.md): how the workspace is organized.
- [Sensor Device](projects/sensor_device/README.md): sensor application setup.
