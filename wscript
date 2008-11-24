#! /usr/bin/env python
# encoding: utf-8
# Juho Vähä-Herttua, 2008

VERSION='0.0.1'
APPNAME='tapcfg'

srcdir = '.'
blddir = 'build'

def set_options(opt):
	opt.tool_options('compiler_cc')
	opt.tool_options('cs')

def configure(conf):
	conf.check_tool('compiler_cc')
	conf.check_tool('cs')
	conf.env['PREFIX'] = '..'

def build(bld):
	bld.add_subdirs('src')
