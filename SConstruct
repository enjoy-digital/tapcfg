
AddOption('--force-mingw',
          action='store_true', dest='mingw', default=False,
          help='Cross compile using MinGW for Windows')

env = Environment()
env.Tool('gmcs', toolpath = ['scons-tools'])

if GetOption('mingw'):
	env.Tool('crossmingw', toolpath = ['scons-tools'])
	env.Append(CPPDEFINES = ['WINVER=0x0501'])

conf = Configure(env)
conf.CheckLib('socket')
conf.CheckLib('ws2_32')
if conf.CheckFunc('getaddrinfo'):
	env.Append(CPPDEFINES = ['HAVE_GETADDRINFO'])
env = conf.Finish()

env.SConscript('src/SConscript', exports='env')

