clang++ fsplugin.cpp \
-shared -fPIC \
-I /opt/homebrew/Cellar/libmtp/1.1.22/include \
-L /opt/homebrew/lib \
-l mtp -l usb-1.0 \
-o fsplugin.wfx64