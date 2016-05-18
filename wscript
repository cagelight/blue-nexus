#!/bin/python

from waflib import *
import os, sys

top = '.'
out = 'build'

projname = 'bluenexus'
coreprog_name = 'bnex'

g_comflags = ["-Wall", "-Wextra", "-Wpedantic"]
g_cflags = ["-std=gnu11"] + g_comflags
def btype_cflags(ctx):
	return {
		"DEBUG"   : g_cflags + ["-Og", "-ggdb3", "-march=core2", "-mtune=native"],
		"NATIVE"  : g_cflags + ["-Ofast", "-march=native", "-mtune=native"],
		"RELEASE" : g_cflags + ["-O3", "-march=core2", "-mtune=generic"],
	}.get(ctx.env.BUILD_TYPE, g_cflags)

def options(opt):
	opt.load("gcc")
	opt.add_option('--build_type', dest='build_type', type="string", default='RELEASE', action='store', help="DEBUG, NATIVE, RELEASE")

def configure(ctx):
	ctx.load("gcc")
	ctx.check(features='c cprogram', lib='jemalloc', uselib_store='JMAL')
	ctx.check(features='c cprogram', lib='ncurses', uselib_store='NCURSES')
	btup = ctx.options.build_type.upper()
	if btup in ["DEBUG", "NATIVE", "RELEASE"]:
		Logs.pprint("PINK", "Setting up environment for known build type: " + btup)
		ctx.env.BUILD_TYPE = btup
		ctx.env.CFLAGS = btype_cflags(ctx)
		Logs.pprint("PINK", "CFLAGS: " + ' '.join(ctx.env.CFLAGS))
		if btup == "DEBUG":
			ctx.define("BLUE_NEXUS_DEBUG", 1)
	else:
		Logs.error("UNKNOWN BUILD TYPE: " + btup)

def build(bld):
	files =  bld.path.ant_glob('src/*.c')
#	hfiles = bld.path.ant_glob('src/bnex/*.h')
#	bld.install_files('${PREFIX}/include/bnex', hfiles)
	coreprog = bld (
		features = "c cprogram",
		target = coreprog_name,
		source = files,
		lib = ["pthread"],
		uselib = ['JMAL', 'NCURSES']
	)
	
def clean(ctx):
	pass
