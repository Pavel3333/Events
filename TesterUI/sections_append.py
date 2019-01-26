import json
cfg = json.loads(open('positions.json', 'r').read())

sections = ('1', '2', '3')

for map_ in cfg:
    for sect in sections:
        if sect not in cfg[map_]:
            cfg[map_][sect] = []

open('positions.json', 'w').write(json.dumps(cfg, indent=4, sort_keys=True))
