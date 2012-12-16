import usb

def findDevice(vendor_id):
  buses = usb.busses()
  for bus in buses :
    for device in bus.devices :
      print 'idVendor: %x' % device.idVendor
      if device.idVendor == vendor_id:
        print 'Found'
        return device
  return None

class VUSB:
  USB_VENDOR_ID = 0x16C0
  REQUEST_TYPE = usb.TYPE_VENDOR | usb.RECIP_DEVICE | usb.ENDPOINT_IN

  USB_BUFFER_SIZE = 20
  CMD_ZERO = 0

  def __init__(self):
    device = findDevice(self.USB_VENDOR_ID)
    if not device:
      raise Exception('Device not available')
    self.handle = device.open()

  def getBufferSize(self):
    return self.send_cmd(self.CMD_ZERO)

  def send_cmd(self, cmd, param=0):
    val = self.handle.controlMsg(requestType = self.REQUEST_TYPE, request = cmd, value = param, buffer = self.USB_BUFFER_SIZE)
    return val

def main():
  client = VUSB()
  print client.getBufferSize()

if __name__ == '__main__':
  main()
