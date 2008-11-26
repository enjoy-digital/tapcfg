
env = Environment()
env.Tool('gmcs', toolpath = ['scons-tools'])

conf = Configure(env)
conf.CheckLib('pthread')
conf.CheckLib('ws2_32')
env = conf.Finish()

env.SConscript('src/SConscript', exports='env')

