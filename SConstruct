
env = Environment()
env.Tool('mcs', toolpath = [''])
env.Append(CPPPATH = '#src/include')

env.SConscript('src/SConscript', exports='env')

