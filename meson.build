project('csp_ftp', 'c')

csp_dep = dependency('csp', fallback : ['csp', 'csp_dep'])

csp_ftp_src = files([
	'src/ftp_server.c',
	'src/ftp_perf.c',
	'src/ftp_client.c',
	'src/ftp_status.c',
])

csp_ftp_inc = include_directories('include')

slash_dep = []
if get_option('slash') == true
	slash_dep = dependency('slash', fallback : ['slash', 'slash_dep'], required: false)
	if slash_dep.found()
		csp_ftp_src += files([
			'src/ftp_slash.c',	
		])
	endif
endif

csp_ftp_lib = static_library('csp_ftp',
	sources: [csp_ftp_src],
	include_directories : csp_ftp_inc,
	dependencies : [csp_dep, slash_dep],
	install : false
)

csp_ftp_dep = declare_dependency(include_directories : csp_ftp_inc, link_with : csp_ftp_lib)
