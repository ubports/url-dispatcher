import os.path
from xdg.BaseDirectory import xdg_cache_home
from apport.hookutils import *

def attach_command_output(report, command_list, key):
	log = command_output(command_list)
	if not log or log[:5] == "Error":
		return
	report[key] = log 

def add_info(report):
	if not apport.packaging.is_distro_package(report['Package'].split()[0]):
		report['ThirdParty'] = 'True'
		report['CrashDB'] = 'url_dispatcher'

	dbpath = os.path.join(xdg_cache_home, 'url-dispatcher', 'urls-1.db')
	if os.path.exists(dbpath):
		attach_command_output(report, ['url-dispatcher-dump'], 'URLDispatcherDB')
