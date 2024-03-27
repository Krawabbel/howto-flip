*The below explanation assumes the reader is using Flipper Lab.*

- [How can I capture an IR signal in the Flipper Zero CLI?](#how-can-i-capture-an-ir-signal-in-the-flipper-zero-cli)
- [How does the raw signal relate to the decoded message?](#how-does-the-raw-signal-relate-to-the-decoded-message)
- [How can I emulate a command with the Flipper Zero?](#how-can-i-emulate-a-command-with-the-flipper-zero)
  - [How to determine the carrier frequency?](#how-to-determine-the-carrier-frequency)
  - [How to determine the duty cycle?](#how-to-determine-the-duty-cycle)

## How can I capture an IR signal in the Flipper Zero CLI?

In the Flipper Zero CLI, I can e.g. capture the "Mute" command from my Edifier R1280DB remote in two different ways, see the [IR CLI documentation](https://docs.flipper.net/development/cli/#FEjwz).

1. `ir rx` reads and decodes IR data giving
```
NECext, A:0xE710, C:0xFF00
```

"NECext" is the protocol, "A" denotes the address, i.e. the intended recipient of the signal (the speakers), and "C" denotes the command. The protocol and address don't change for different commands. For instance, if I instead press the "Power" button, I get

```
NECext, A:0xE710, C:0xFE01
```


2. `ir rx raw` reads IR data in RAW format giving
```
RAW, 87 samples: 
8934 4472 554 557 553 555 555 555 555 557 555 1662 555 555 555 556 554 556 556 1663 554 1663 554 1661 556 558 554 555 555 1663 554 1661 556 1665 553 555 555 556 554 556 554 557 555 555 555 555 555 555 555 557 555 1662 555 1662 555 1662 555 1664 554 1662 555 1663 554 1662 555 1665 554 40944 8935 2237 552 95357 8937 2235 554 95360 8935 2236 554 95359 8934 2235 555 95361 8930 2269 521
```

We can plot the corresponding raw IR signal file `mute_raw.ir` (I'll explain how to build this in a bit.) in the Flipper Zero Pulse Plotter. 

:::image type="content" source="pulse-plotter.png" alt-text="pulse plot of mute_raw.ir":::

We see that every number of the raw data represents a duration (in microseconds) of alternating "high" and "low", i.e. 8934 microseconds "high", 4472 microseconds "low", 554 microseconds "high", ... 

## How does the raw signal relate to the decoded message?

Here, the encoding is NECext, which is similar to (plain) NEC, see https://techdocs.altium.com/display/FPGA/NEC+Infrared+Transmission+Protocol.

I wasn't able to find a precise specification of the NECext protocol anywhere but the implementation of the flipper-zero firmware actually gives us an idea, see https://github.com/flipperdevices/flipperzero-firmware/blob/21e7c46033b0a41a642e3fcb615edd850ae54c6f/lib/infrared/encoder_decoder/nec/infrared_protocol_nec.h.


With this information, we can understand the raw data.

| signal part             |                         | data     | message             |
| ----------------------- | ----------------------- | -------- | ------------------- |
| Begin                   |                         | 8934     | 9ms leading pulse   |
|                         |                         | 4472     | 4.5ms space         |
| 16-Bit Address (0xE710) | 0x10 = 0x00001000 = 16  | 554 557  | Logical '0'         |
|                         |                         | 553 555  | Logical '0'         |
|                         |                         | 555 555  | Logical '0'         |
|                         |                         | 555 557  | Logical '0'         |
|                         |                         | 555 1662 | Locical '1'         |
|                         |                         | 555 555  | Logical '0'         |
|                         |                         | 555 556  | Logical '0'         |
|                         |                         | 554 556  | Logical '0'         |
|                         | 0xE7 = 0b11100111 = 231 | 556 1663 | Logical '1'         |
|                         |                         | 554 1663 | Logical '1'         |
|                         |                         | 554 1661 | Logical '1'         |
|                         |                         | 556 558  | Logical '0'         |
|                         |                         | 554 555  | Logical '0'         |
|                         |                         | 555 1663 | Logical '1'         |
|                         |                         | 554 1661 | Logical '1'         |
|                         |                         | 556 1665 | Logical '1'         |
| 16-Bit Command (0xFF00) | 0x00 = 0x00000000 = 0   | 553 555  | Logical '0'         |
|                         |                         | 555 556  | Logical '0'         |
|                         |                         | 554 556  | Logical '0'         |
|                         |                         | 554 557  | Logical '0'         |
|                         |                         | 555 555  | Logical '0'         |
|                         |                         | 555 555  | Logical '0'         |
|                         |                         | 555 555  | Logical '0'         |
|                         |                         | 555 557  | Logical '0'         |
|                         | 0xFF = 0b11111111 = 255 | 555 1662 | Logical '1'         |
|                         |                         | 555 1662 | Logical '1'         |
|                         |                         | 555 1662 | Logical '1'         |
|                         |                         | 555 1664 | Logical '1'         |
|                         |                         | 554 1662 | Logical '1'         |
|                         |                         | 555 1663 | Logical '1'         |
|                         |                         | 554 1662 | Logical '1'         |
|                         |                         | 555 1665 | Logical '1'         |
| End                     |                         | 554      | 562.5µs pulse burst |
| Repeat (1)              |                         | 40944    | pause 40ms          |
|                         |                         | 8935     | 9ms leading pulse   |
|                         |                         | 2237     | 2.25ms space        |
|                         |                         | 552      | 562.5µs pulse burst |
| Repeat (2)              |                         | 95357    | pause 95ms          |
|                         |                         | 8937     | 9ms leading pulse   |
|                         |                         | 2235     | 2.25ms space        |
|                         |                         | 554      | 562.5µs pulse burst |
| Repeat (3)              |                         | 95360    | pause 95m           |
|                         |                         | 8935     | 9ms leading pulse   |
|                         |                         | 2236     | 2.25ms space        |
|                         |                         | 554      | 562.5µs pulse burst |
| Repeat (4)              |                         | 95359    | pause 95m           |
|                         |                         | 8934     | 9ms leading pulse   |
|                         |                         | 2235     | 2.25ms space        |
|                         |                         | 555      | 562.5µs pulse burst |
| Repeat (5)              |                         | 95361    | pause 95m           |
|                         |                         | 8930     | 9ms leading pulse   |
|                         |                         | 2269     | 2.25ms space        |
|                         |                         | 521      | 562.5µs pulse burst |

We see, that this signal is transmitted in "little endian" mode, i.e. the least significant bits and bytes  come first.

## How can I emulate a command with the Flipper Zero?

Generally, I can simply generate the files `mute.ir` and `mute_raw.ir` which I can store on the Flipper Zero and use to send the "mute" command to my speakers. For the latter, we need some more information.

https://www.phidgets.com/docs/IR_Remote_Control_Guide#:~:text=Duty%20cycle%20is%20the%20period,stream%20cannot%20be%20automatically%20determined.

### How to determine the carrier frequency?

According to [this explanation](https://techdocs.altium.com/display/FPGA/NEC+Infrared+Transmission+Protocol), the carrier frequency of NECext is ~33kHz. But if we only see the raw data, how do we find this?

> [!IMPORTANT]
> todo

https://www.laser.com/dhouston/ir-rf_fundamentals.html

### How to determine the duty cycle?

> [!IMPORTANT]
> todo

> [!IMPORTANT]
> in CLI





