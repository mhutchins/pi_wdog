#!/usr/bin/env python
'''
A Python class implementing KBHIT, the standard keyboard-interrupt poller.
Works transparently on Windows and Posix (Linux, Mac OS X).  Doesn't work
with IDLE.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

'''

import os
import usb # 1.0 not 0.4
import sys
import time
import termios
import atexit
from select import select

def getStringDescriptor(device, index):
    """
    """
    # TODO: Refer to 'libusb_get_string_descriptor_ascii' for error handling
    response = device.ctrl_transfer(usb.util.ENDPOINT_IN,
                                    usb.legacy.REQ_GET_DESCRIPTOR,
                                    (usb.util.DESC_TYPE_STRING << 8) | index,
                                    0, # language id
                                    255) # length
    return response[2:].tostring().decode('utf-16')

REQUEST_TYPE_SEND = usb.util.build_request_type(usb.util.CTRL_OUT,
                                                usb.util.CTRL_TYPE_CLASS,
                                                usb.util.CTRL_RECIPIENT_DEVICE)

REQUEST_TYPE_RECEIVE = usb.util.build_request_type(usb.util.CTRL_IN,
                                                usb.util.CTRL_TYPE_CLASS,
                                                usb.util.CTRL_RECIPIENT_DEVICE)

USBRQ_HID_GET_REPORT = 0x01
USBRQ_HID_SET_REPORT = 0x09
USB_HID_REPORT_TYPE_FEATURE = 0x03

class ArduinoUsbDevice(object):
    """
    """

    def __init__(self, idVendor, idProduct):
        """
        """
        self.idVendor = idVendor
        self.idProduct = idProduct

        # TODO: Make more compliant by checking serial number also.
        self.device = usb.core.find(idVendor=self.idVendor,
                                    idProduct=self.idProduct)

        if not self.device:
            raise Exception("Device not found")


    def write(self, byte):
        """
        """
        # TODO: Return bytes written?
        #print "Write:"+str(byte)
        self._transfer(REQUEST_TYPE_SEND, USBRQ_HID_SET_REPORT,
                       byte,
                       []) # ignored


    def read(self):
        """
        """
        response = self._transfer(REQUEST_TYPE_RECEIVE, USBRQ_HID_GET_REPORT,
                              0, # ignored
                              1) # length

        if not response:
            raise Exception("No Data")

        return response[0]


    def _transfer(self, request_type, request, index, value):
        """
        """
        return self.device.ctrl_transfer(request_type, request,
                                        (USB_HID_REPORT_TYPE_FEATURE << 8) | 0,
                                         index,
                                         value)


    @property
    def productName(self):
        """
        """
        return getStringDescriptor(self.device, self.device.iProduct)


    @property
    def manufacturer(self):
        """
        """
        return getStringDescriptor(self.device, self.device.iManufacturer)

class KBHit:

    def __init__(self):
        '''Creates a KBHit object that you can call to do various keyboard things.
        '''

        if os.name == 'nt':
            pass

        else:

            # Save the terminal settings
            self.fd = sys.stdin.fileno()
            self.new_term = termios.tcgetattr(self.fd)
            self.old_term = termios.tcgetattr(self.fd)

            # New terminal setting unbuffered
            self.new_term[3] = (self.new_term[3] & ~termios.ICANON & ~termios.ECHO)
            termios.tcsetattr(self.fd, termios.TCSAFLUSH, self.new_term)

            # Support normal-terminal reset at exit
            atexit.register(self.set_normal_term)


    def set_normal_term(self):
        ''' Resets to normal terminal.  On Windows this is a no-op.
        '''

        if os.name == 'nt':
            pass

        else:
            termios.tcsetattr(self.fd, termios.TCSAFLUSH, self.old_term)


    def getch(self):
        ''' Returns a keyboard character after kbhit() has been called.
            Should not be called in the same program as getarrow().
        '''

        s = ''

        if os.name == 'nt':
            return msvcrt.getch().decode('utf-8')

        else:
            return sys.stdin.read(1)


    def getarrow(self):
        ''' Returns an arrow-key code after kbhit() has been called. Codes are
        0 : up
        1 : right
        2 : down
        3 : left
        Should not be called in the same program as getch().
        '''

        if os.name == 'nt':
            msvcrt.getch() # skip 0xE0
            c = msvcrt.getch()
            vals = [72, 77, 80, 75]

        else:
            c = sys.stdin.read(3)[2]
            vals = [65, 67, 66, 68]

        return vals.index(ord(c.decode('utf-8')))


    def kbhit(self):
        ''' Returns True if keyboard character was hit, False otherwise.
        '''
        if os.name == 'nt':
            return msvcrt.kbhit()

        else:
            dr,dw,de = select([sys.stdin], [], [], 0)
            return dr != []


if __name__ == "__main__":

    try:
        theDevice = ArduinoUsbDevice(idVendor=0x16c0, idProduct=0x05df)
    except:
        sys.exit("No DigiUSB Device Found")

    kb = KBHit()

    print('Connected to USB device. Type command(s), or ESC to exit')

    while True:

        if kb.kbhit():
            c = kb.getch()
            if ord(c) == 27: # ESC
                break
            theDevice.write(ord(c))

        try:
            lastChar = chr(theDevice.read())
            sys.stdout.write(lastChar)
            sys.stdout.flush()

        except:
            pass

    kb.set_normal_term()
