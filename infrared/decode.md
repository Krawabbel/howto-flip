see https://github.com/flipperdevices/flipperzero-firmware/blob/21e7c46033b0a41a642e3fcb615edd850ae54c6f/lib/infrared/encoder_decoder/nec/infrared_protocol_nec.h

protocol: NECext
address: 10 E7 00 00
command: 00 FF 00 00

9ms lead 8934

4.5ms space 4472

16-bit address 0x10 0xE7

0 554 557
0 553 555
0 555 555
0 555 557
1 555 1662
0 555 555
0 555 556
0 554 556

1 556 1663
1 554 1663
1 554 1661
0 556 558
0 554 555
1 555 1663
1 554 1661
1 556 1665

16-bit command 0x00 0xFF

0 553 555 
0 555 556
0 554 556
0 554 557
0 555 555
0 555 555
0 555 555
0 555 557

1 555 1662
1 555 1662
1 555 1662
1 555 1664
1 554 1662
1 555 1663
1 554 1662
1 555 1665

final 562.5 micro sec pulse 554

pause 40ms 40944

4x repeat 

leading burst 9ms 8935
space 2.25ms 2237
trailing pulse burst 552

pause 95ms 95357

leading burst 9ms 8937
space 2.25ms 2235
trailing pulse burst 554

pause 95m 95360

leading burst 9ms 8935
space 2.25ms 2236
trailing pulse burst 554

pause 95m 95359

leading burst 9ms  8934
space 2.25ms  2235
trailing pulse burst  555

pause 95m 95361

leading burst 9ms 8930
space 2.25ms 2269
trailing pulse burst 521