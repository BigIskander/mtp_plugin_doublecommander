# MTP plugin for Double Commander

[По русски](README_RUS.md)

This is the plugin for file manager [Double Commander](https://doublecmd.sourceforge.io) in macOS. 

Plugin allows Double Commander to interact with files in devices transferring data using MTP (Media Transfer Protocol).

## How to install and setup

### 1. Install dependencies
To work with MTP devices plugin uses [libmtp](https://github.com/libmtp/libmtp). For that reason it is required to install libmtp. Easiest way is to install it via [Homebrew](https://brew.sh).

```
brew install libmtp
```

### 2. Add plugin in Double Commander
1. Download plugin (file *.wfx64) from Releases page of this repository or get this file by compiling plugin from source code.
2. In Double Commander open settings window: ***Configuration > Options***
3. In settings window in side menu select ***Plugins WFX***
4. Click ***Add*** button and choose path to plugin file (file with **.wfx64** extension).

## How to use
1. Connect MTP device to computer via USB, then in device's settings turn on data transferring via MTP (in modern Android smartphones *File transfer/Android auto* option).
2. Open Double Commander, then in Double Commander open **//** (**Open VFS list**) tab. In **vfs:** tab open **MTP** folder. List of connected MTP devices will be there.

## For developers

1. To compile the plugin, libmtp header file and installed libmtp library (version 1.1.22) are necessary.

    Install libmtp:
    ```
    brew install libmtp
    ```

2. To compile the plugin, clang++ compiler included in Xcode Command Line Developer Tools is used. 

    Install Xcode Command Line Developer Tools:
    ```
    xcode-select --install 
    ```

3. To compile the plugin: mark compilation script as executable file and run it.

    For mac on Apple Silicon:
    ```
    chmod +x compile_mac.sh
    ./compile_mac.sh
    ```

    For mac on Intel:
    ```
    chmod +x compile_mac_x86_64.sh
    ./compile_mac_x86_64.sh
    ```

    Compiled plugin, file **fsplugin.wfx64**, will be located in the same folder.
