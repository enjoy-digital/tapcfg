
env = Environment()
env.Tool('csc', toolpath = [''])
env.VariantDir('build', '.')

env.SConscript('build/src/SConscript', exports='env', duplicate=0)

