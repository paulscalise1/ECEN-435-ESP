# Validation

Ensure ```DEBUG``` is set to 1 in ```receiver.ino``` for debugging.

## Sender

The files in the sender folder can be used to test sending a clean image over the air to the receiver, without using the ArduCAM.
Include ```jpeg_test.h``` in the sender arduino sketch, and comment out any 'packetBuf' or 'packetLen' definitions.
Change the main code to disregard UART data, and only send the test image.

## Receiver

Prior to running the python script, it is recommended to create a virtual environment. Run the following commands depending on your system.

For Windows Powershell:

```powershell
cd receiver

# Create virtual environment
python -m venv venv

# Activate it
venv\Scripts\activate
```

For macOS/Linux:

```bash
cd receiver

# Create virtual environment
python3 -m venv venv

# Activate it
source venv/bin/activate
```

Then install the dependencies once you have activated the venv:
```bash
pip install -r requirements.txt
```

In the receiver folder there are two files.
The first contains UART output of example BMP image data in ```DEBUG``` mode, and once you have received an image, you can copy your output BMP UART text into that file to view the image. **Prior to copying your UART ouput from the receiver ESP, delete the first 3 header bytes: '49' '4D' '47'.**

Once included, run the python script:

```bash
python3 bmp.py
```

This will paint the pixels on a canvas in their expected orientation.
