clang++ fsplugin.cpp \
-shared -fPIC \
-l mtp -l usb-1.0 \
-o fsplugin.wfx64