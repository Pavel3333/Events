a = input()+'\x00'
b = []
for i in a:
    b.append(ord(i))

for i in xrange(1, len(b)):
    b[i] = b[i] ^ b[i - 1]

c_hex = ''

for i in b:
    c_hex += '\\x' + (str(hex(i)))[2:]

print 'hex        : "' + c_hex +          '"'
print 'hex len    : "%s"'%(len(b) + 1)
print 'without hex: "' + a + '"'

    
