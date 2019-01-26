import json
import os

for _map in os.listdir('./input/'):
    if 'json' in _map:
        print _map
        s = json.loads(open('./input/' + _map, 'r').read())
        k = json.dumps(s, sort_keys=True, indent=4)
        with open('./input/' + _map, 'w') as fil:
            fil.write(k)
