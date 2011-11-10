import usb.core

MODE_DISABLED=0
MODE_SVMI=1
MODE_SIMV=2
gainMapping = {1:0x00<<2, 2:0x01<<2, 4:0x02<<2, 8:0x03<<2, 16:0x04<<2, 32:0x05<<2, 64:0x06<<2, .5:0x07<<2}

def unpackSign(n):
	return n - (1<<12) if n>2048 else n

class CEE(object):
	def __init__(self):
		self.gains = 4*[1]
		self.dev = usb.core.find(idVendor=0x9999, idProduct=0xffff)
		if not self.dev:
			raise IOError("device not found")
			
	def b12unpack(self, s):
		"Turn a 3-byte string containing 2 12-bit values into two ints"
		return s[0]|((s[1]&0x0f)<<8), ((s[1]&0xf0) >> 4)|(s[2]<<4)

	closest=lambda self,a,l:min(l,key=lambda x:abs(x-a))

	def readADC(self):
		data = self.dev.ctrl_transfer(0x40|0x80, 0xA0, 0, 0, 6)
		l = self.b12unpack(data[0:3]) + self.b12unpack(data[3:6])
		vals = map(unpackSign, l)
		return {
			'a_v': vals[0]/2048.0*5/self.gains[1],
			'a_i': ((vals[1]/2048.0*2.5))/45/.07,
			'b_v': vals[2]/2048.0*5/self.gains[2],
			'b_i': ((vals[3]/2048.0*2.5))/45/.07,
		}

	def set(self, chan, v=None, i=None, x=None):
		"""v is voltage in volts, i is current in amps, x is a tuple of (channel, gain)"""
		cmd = 0xAA+chan
		if v is not None:
			dacval = int(round(v/5.0*4095))
			self.dev.ctrl_transfer(0x40|0x80, cmd, dacval, MODE_SVMI, 0)
		elif i is not None:
			dacval = int((2**12*(1.25+(45*.07*i)))/2.5)
			self.dev.ctrl_transfer(0x40|0x80, cmd, dacval, MODE_SIMV, 0)
		elif x is not None:
			gain = self.closest(x[1], gainMapping.keys())
			gainval = gainMapping[gain]
			self.gains[x[0]] =  gain
			self.dev.ctrl_transfer(0x40|0x80, 0x65, gainval, x[0], 0)
		else:
			self.dev.ctrl_transfer(0x40|0x80, cmd, 0, MODE_DISABLED, 0)	

	def setA(self, v=None, i=None):
		self.set(0, v, i)
	
	def setB(self, v=None, i=None):
		self.set(1, v, i)

	def bootload(self):
		self.dev.ctrl_transfer(0x40|0x80, 0xBB, 0, 0, 1)

if __name__ == "__main__":
	cee = CEE()
