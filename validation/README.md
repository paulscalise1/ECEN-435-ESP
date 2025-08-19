# Validation

Ensure ```DEBUG``` is set to 1 in both code files for debugging.

## Sender

The files in the sender folder can be used to test sending a clean image over the air to the receiver, without using the ArduCAM.
Include ```jpeg_test.h``` in the sender arduino sketch, and comment out any 'packetBuf' or 'packetLen' definitions.
Change the main code to disregard UART data, and only send the test image.

## Receiver

In the receiver folder there are two files.
The first contains UART output of example BMP data in debug mode, and you can copy your UART text into that file as well to test.
Once included, run:

```bash
python3 bmp.py
```

This will paint the pixels on a canvas in their expected orientation. 