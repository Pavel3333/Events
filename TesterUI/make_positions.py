import os
import json
d = {}
for name in os.listdir('./input/'):
    if '.json' in name:
      d[name.replace('.json', '')] = json.loads(open('./input/%s'%(name), 'r').read())
open('positions.json', 'w').write(json.dumps(d, sort_keys=True, indent=4))
