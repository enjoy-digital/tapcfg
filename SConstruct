
AddOption('--cross-mingw',
          action='store_true', dest='mingw', default=False,
          help='Cross compile using MinGW for Windows')

env = Environment()
env.Tool('gmcs', toolpath = ['scons-tools'])

if GetOption('mingw'):
	env.Tool('crossmingw', toolpath = ['scons-tools'])
	env.Append(CPPDEFINES = ['WINVER=0x0501'])

env.SConscript('src/SConscript', exports='env')

