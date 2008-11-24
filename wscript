#! /usr/bin/env python
# encoding: utf-8
# Juho Vähä-Herttua, 2008

VERSION='0.0.1'
APPNAME='tapcfg'

srcdir = '.'
blddir = 'build'

def set_options(opt):
	opt.tool_options('compiler_cc')

def configure(conf):
	conf.check_tool('compiler_cc')
	conf.env['PREFIX'] = '..'

def build(bld):
	bld.add_subdirs('src')
