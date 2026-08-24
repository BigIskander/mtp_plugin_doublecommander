clang++ fsplugin.cpp \
-shared -fPIC \
-I /opt/homebrew/opt/libmtp/include \
-L /opt/homebrew/lib \
-l mtp -l usb-1.0 \
-o fsplugin.wfx64
