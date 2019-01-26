from random import randint

ID_SIZE = 4
DWNLD_TOKEN_SIZE = 252

KEY_LENGTH = ID_SIZE + DWNLD_TOKEN_SIZE

def addCode(code_line):
    global code
    code += code_line + '\n'

def genRandList(size):
    rands = []

    rands_ctr = size

    while(rands_ctr != 0):
        rand_len = randint(1, rands_ctr)
        rands.append(rand_len)
        rands_ctr -= rand_len
    
    return rands

size = int(input())

if not size:
    raise Exception('RG', 'size must be positive')

templates = {
    'init'   : 'unsigned char* rubbish%s = new unsigned char[%s];',
    'filling': 'generate_random_bytes(rubbish%s, %s);',
    
    'mod'    : 'token        [%s] = ModsID.POS_FREE; //mod\n',
    'map_ID' : 'token        [%s] = map_id;          //map ID\n',
    'key'    : 'memcpy(&token[%s], key, 256U);       //key\n',

    'rubbish': 'memcpy(&token[%s], rubbish%s, %sU);\n\nmemset(rubbish%s, NULL, %sU);\ndelete[] rubbish%s;\n'
    
}

rand_step_mod = 0
rand_step_map_ID = 0
rand_step_map_key = 0

rands_size = 3

rands = genRandList(size)

while(len(rands) < 5):
    rands = genRandList(size)

all_comm_len = max(8, len(rands) + 3)

while(
    rand_step_mod    == rand_step_map_ID  or
    rand_step_mod    == rand_step_map_key or
    rand_step_map_ID == rand_step_map_key
    ):
    rand_step_mod     = randint(0, all_comm_len - 1)
    rand_step_map_ID  = randint(0, all_comm_len - 1)
    rand_step_map_key = randint(0, all_comm_len - 1)

counter = 0
counter_rubbish = 0

code = '//-----------------------Generating rubbish section---------------------------------------------\n\n'

#init code

for i in xrange(len(rands)):
    addCode(templates['init']%(counter_rubbish, rands[counter_rubbish]))
    counter_rubbish += 1

counter_rubbish = 0

#---------

code += '\n'

#filling code

for i in xrange(len(rands)):
    addCode(templates['filling']%(counter_rubbish, rands[counter_rubbish]))
    counter_rubbish += 1

counter_rubbish = 0

code += '\n//-----------------------------------------------------------------------------------------------\n\nunsigned char* token = new unsigned char[513];\n\n'

#creating token

offset_mod = 0
offset_map_ID = 0
offset_key = [0, 0]

for i in xrange(all_comm_len):
    if(i == rand_step_mod):
        addCode(templates['mod']%(counter))

        offset_mod = i
        
        counter += 1
    elif(i == rand_step_map_ID):
        addCode(templates['map_ID']%(counter))

        offset_map_ID = i
        
        counter += 1
    elif(i == rand_step_map_key):
        addCode(templates['key']%(counter))

        offset_key = [i, i + KEY_LENGTH]
        
        counter += KEY_LENGTH
    else:
        rubbish_len = rands[counter_rubbish]
        addCode(
            templates['rubbish']%(
                counter,
                counter_rubbish,
                rubbish_len,
                counter_rubbish,
                rubbish_len,
                counter_rubbish
                ))
        counter += rands[counter_rubbish]
        counter_rubbish += 1

code += 'token[512] = NULL;\n\n//-----------------------------------------------------------------------------------------------\n\n'

print code

print '\n'

print 'offset_mod   :', offset_mod
print 'offset_map_ID:', offset_map_ID
print 'offset_key   :', offset_key
