#! /usr/bin/env python
# encoding: utf-8
# Juho Vähä-Herttua, 2008

VERSION='0.0.1'
APPNAME='tapcfg'

srcdir = '.'
blddir = '_build_'

def set_options(opt):
	opt.tool_options('compiler_cc')
	opt.tool_options('cs')

	opt.add_option('--conf-prefix', action='store', dest='config_prefix')
	opt.add_option('--disable-ipv6', action='store_true', default=False,
		dest='disable_ipv6')

def configure(conf):
	conf.check_tool('compiler_cc')
	conf.check_tool('cs')

	import os, Options
	if Options.options.config_prefix:
		dir = Options.options.config_prefix
		if not os.path.isabs(dir):
			dir = os.path.abspath(dir)
		conf.env.prepend_value("LIBPATH", os.path.join(dir, "lib"))
		conf.env.prepend_value("CPPPATH", os.path.join(dir, "include"))

	if Options.options.disable_ipv6:
		conf.env["CCDEFINES"] += ['DISABLE_IPV6']

	if conf.check_cc(lib='ws2_32') != None:
		conf.env["CCDEFINES"] += ['_WIN32_WINNT=0x0501']
		conf.env["LIB_socket"] = 'ws2_32'
		conf.env["shlib_PATTERN"] = 'lib%s.dll'
		conf.env["program_PATTERN"] = '%s.exe'

	if conf.check_cc(lib='pthread') != None:
		conf.env.append_unique("LIB_thread", 'pthread')

	conf.sub_config('src')

def build(bld):
	bld.add_subdirs('src')
