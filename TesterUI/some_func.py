from os import listdir
import pyperclip

def random_bytes():
    from random import randint
    a = randint(0,2)
    if a == 0:
        return randint(ord('a'), ord('z'))
    elif a == 1:
        return randint(ord('A'), ord('Z'))
    else:
        return randint(ord('0'), ord('9'))
def get_binary(filename):
    from os.path import exists
    if not exists(filename):
        return 'Not Found: %s'%(filename)
    out = ''
    with open(filename, 'rb') as fil:
        for i in fil.read():
            out += str(hex(ord(i))).replace('0x','\\x')
    return filename + ' : ' + out

def get_binary_map(filename):
    from os.path import exists
    if not exists(filename):
        return 'Not Found: %s'%(filename)
    out = '0x'
    with open(filename, 'rb') as fil:
        for i in fil.read():
            out += ('0' + str(hex(ord(i))).replace('0x', ''))[-2:]
    print filename.split('_')[0].split('/')[2]
    pyperclip.copy(out)
#key = ''.join([chr(random_bytes()) for i in xrange(0, 252)])

#with open('TRAJECTORY.lic', 'wb') as fil:
#    fil.write(key)
#print get_binary('polynomial.txt')
#print get_binary('polynomial_thirdbyte.bin')
#print get_binary('polynomial_fourthbyte.bin')
#for name in listdir('./'):
#    if 'xored' in name:
#        print get_binary(name)
for name in listdir('./output/'):
    if 'bin' in name:
        get_binary_map('./output/' + name)
        a = input()
