
AddOption('--force-mingw',
          action='store_true', dest='mingw', default=False,
          help='Cross compile using MinGW for Windows')

AddOption('--universal',
          action='store_true', dest='universal', default=False,
          help='Create Mac 32-bit and 64-bit universal binaries')

BuildDir('build', 'src')
env = Environment()
env.Tool('gmcs', toolpath = ['scons-tools'])

if GetOption('mingw'):
	env.Tool('crossmingw', toolpath = ['scons-tools'])
	env.Append(CPPDEFINES = ['WINVER=0x0500'])

env.Append(CFLAGS = ['-Wall', '-Werror', '-O2'])

conf = Configure(env)
conf.CheckLib('socket')
if conf.CheckFunc('getaddrinfo'):
	env.Append(CPPDEFINES = ['HAVE_GETADDRINFO'])
else:
	if conf.CheckLibWithHeader('ws2_32', 'ws2tcpip.h','c','getaddrinfo(0,0,0,0);'):
		env.Append(CPPDEFINES = ['HAVE_GETADDRINFO'])
	else:
		if conf.CheckLib('ws2_32'):
			# We have windows socket lib without getaddrinfo, disable IPv6
			env.Append(CPPDEFINES = ['DISABLE_IPV6'])
env = conf.Finish()

env.SConscript('build/SConscript', exports='env')

