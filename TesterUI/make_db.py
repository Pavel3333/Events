import json

dic2={"DB": []}

cfg = json.loads(open('positions.json','r').read())
for map_ in cfg:
    dic2['DB'].append({map_:cfg[map_]})

db = json.dumps(dic2, sort_keys=True)
with open('positions_DB.json','w') as my_file:
    my_file.write(db.replace('}}, ', '}}, \n').replace('{"DB": [', '{"DB": [\n'))
