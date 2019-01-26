# -*- coding: cp1251 -*-
import json
from struct import pack
from os import listdir

ENCODE = True

def encode_Vig(c, k):
    return ''.join([chr((ord(j) + ord(k[i % len(k)])) % 256) for i, j in enumerate(c)])

trj_summary_count = 0

firing = 0
lighting = 0
LFD = 0

for _map in listdir('./input/'):
    models_count = 0
    s = {}
    fil_bytes = ''
    
    if 'json' in _map:
        with open('./input/' + _map, 'r') as fil:
            s = json.loads(fil.read())
        
        print _map
        
        len_sect = len(s)
        
        firing_count = lighting_count = LFD_count = 0
        
        for sect in s:
            if sect == u'1':
                for billboard_sect in s[sect]: 
                    if billboard_sect:
                        models_count += 1
                        firing_count += 1
            elif sect == u'2':
                for billboard_sect in s[sect]: 
                    if billboard_sect:
                        models_count += 1
                        lighting_count += 1
            elif sect == u'3':
                for billboard_sect in s[sect]: 
                    if billboard_sect:
                        models_count += 1
                        LFD_count += 1

        fil_bytes += pack('<B', len_sect)
        fil_bytes += pack('<B', firing_count)
        fil_bytes += pack('<B', lighting_count)
        fil_bytes += pack('<B', LFD_count)

        trj_summary_count += models_count
        firing += firing_count
        lighting += lighting_count
        LFD += LFD_count

        for sect in s:
            if sect == u'1':
                print '  есть настрел'
                fil_bytes += pack('<B', 0)
                for i in range(len(s[sect])):
                    for j in range(len(s[sect][i])):
                        fil_bytes += pack('<f', s[sect][i][j])
            elif sect == u'2':
                print '  есть насвет'
                fil_bytes += pack('<B', 1)
                for i in range(len(s[sect])):
                    for j in range(len(s[sect][i])):
                        fil_bytes += pack('<f', s[sect][i][j])
            elif sect == u'3':
                print '  есть НЛД'
                fil_bytes += pack('<B', 2)
                for i in range(len(s[sect])):
                    for j in range(len(s[sect][i])):
                        fil_bytes += pack('<f', s[sect][i][j])

        print 'Позиций:', models_count, '\n'
        #print len(fil_bytes) + 2
        
        #print models_count

        fin = fil_bytes
        
        if(ENCODE): fin = encode_Vig(fil_bytes, 'DIO4Gb941mfOiHox6jLntKn6kqfgopFX1xaCu1JWlb3ag')
        
        fin = pack('<H', len(fil_bytes) + 2) + fin
        
        with open('./output/' + _map.replace('json', 'bin'), 'wb') as fil:
            fil.write(fin)
        #with open('./output/' + _map.replace('.json', '_decoded.bin'), 'wb') as fil:
        #    fil.write(pack('<H', len(fil_bytes) + 2) + fil_bytes)

print 'Всего позиций:', trj_summary_count
print 'Позиций настрела:', firing
print 'Позиций насвета:', lighting
print 'Позиций НЛД:', LFD
