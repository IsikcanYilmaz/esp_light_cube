ESP32S3 Light Cube
=====

[![Demo](https://img.youtube.com/vi/rb5LZi5gW-o/0.jpg)](https://www.youtube.com/watch?v=rb5LZi5gW-o)

3rd iteration of the Light Cube. This time on an ESP32S3, running on RIOT OS!

replace RIOTBASE with wherever RIOT exists in your file system


`
. $HOME/KODMOD/esp/esp-idf/export.sh # Get idf
make flash term PORT="/dev/tty.usbmodem111301" WERROR=0
`

RIOT Related:
=====
RIOT uses v4.4.1 of the ESP32 SDK. Commit 1329b19fe494500aeb79d19b27cfd99b40c37aec is the cutoff (as of 12 Jan 25).
