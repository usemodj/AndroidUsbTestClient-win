http://libusb.info/

Download latest file: libusb-1.0.19-rc1-win.7z

1. Copy files compiled in previous step to their appropriate locations

Copy libusb-1.0.19-win\include\libusb-1.0\libusb.h  to C:\MingW\include\
Copy libusb-1.0.19-win\MinGW32\dll\libusb-1.0.dll.a to C:\MingW\lib\
Copy libusb-1.0.19-win\MinGW32\static\libusb-1.0.a  to C:\MingW\lib\
Copy libusb-1.0.19-win\MinGW32\dll\libusb-1.0.dll  	to C:\MingW\bin\

2. MinGW C Linker libraries add: usb-1.0 

 Eclipse properties> C/C++ Build> Settings> MinGW C Linker>
    Libraries: usb-1.0