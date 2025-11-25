clang++ fsplugin.cpp \
-shared -fPIC -arch x86_64 \
-I /usr/local/Cellar/libmtp/1.1.22/include \
-L /usr/local/lib \
-l mtp -l usb-1.0 \
-o fsplugin.wfx64