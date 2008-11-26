
env = Environment()
env.Tool('mcs', toolpath = [''])
env.VariantDir('build', '.')
env.Append(CPPPATH = '#src/include')

env.SConscript('build/src/SConscript', exports='env')

