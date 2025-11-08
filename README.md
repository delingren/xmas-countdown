# Chritmas Countdown Clock

This is a continuation of the [dot matrix clock](https://github.com/delingren/dot-matrix-clock) project. All the hardware related material is covered in that project, in addition to WiFi connection and time synchronization.

A video is worth 1000 words.

![demo](./demo.mov)

It starts with a countdown to Xmas 2025 (the timestamp is hardcoded for simplicity). Once Xmas is reached, it displays an image, a "Merry Christmas" message, and sprinkles some snowflakes once a second.

I wrote two separate sketches to test both parts and put them together in [integration](./integration/) folder.

For the tree image, I composed a 32x32 image in GIMP, and converted it to C code using this [neat little tool](https://marlinfw.org/tools/rgb565/converter.html). I don't have a single artistic bone in me. If you come up with a better graphic, please share with me!