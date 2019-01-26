import json

data = {}

with open('positions_DB.json', 'r') as fil:
    data = json.loads(fil.read())

for cfg in data['DB']:
    for _map in cfg:
        with open('input/' + _map + '.json', 'w') as config:
            config.write(json.dumps(cfg[_map], indent=4, sort_keys=True))
