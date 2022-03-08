
'''
os.mkfifo(fifo_name)
'''
import struct
import array
import numpy as np


'''
f = open("/tmp/test", "rb")

buf = None
first = True

while True:
	byte = f.read(1)
	c = byte.decode('utf-8')
	if c=='\n':
		print("Es salto")
	if not byte:
		break
	else:
		if first:
			buf = byte
			first = False
		else:
			buf += byte



buf=f.read()
print(buf.decode("utf-8"))
print(len(buf))
'''
# print(struct.unpack('f', buf))

#arr = array.array('f', buf)

'''
arr = np.frombuffer(buf, dtype='float32')
print(len(arr))

print(arr[:100])
'''





f = open("/tmp/test", "wb")
f.write(str.encode("hola\n"))
f.write(str.encode("chau\n"))
f.close()

	
