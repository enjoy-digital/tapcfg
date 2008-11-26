
env = Environment()
env.Tool('gmcs', toolpath = ['scons-tools'])
env.Append(CPPPATH = '#src/include')

env.SConscript('src/SConscript', exports='env')

