import imp
try:
    import BigWorld
    mem_helper = imp.load_dynamic('mem_helper', 'res_mods/mem_helper.pyd')
except:
    mem_helper = imp.load_dynamic('mem_helper', 'mem_helper.pyd')
print mem_helper.__doc__
