
env = Environment()
env.Tool('mcs', toolpath = [''])
env.VariantDir('build', '.')

env.SConscript('build/src/SConscript', exports='env', duplicate=0)

