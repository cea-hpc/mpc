set breakpoint pending on
#break lcp_context_create
#break lcp_send
#break _mpc_lowcomm_monitor_client_add
#break _mpc_lowcomm_monitor_command_send
#break proto.c:90
#break shutdown
#break monitor.c:846
#break mpc_common_io_safe_write
#break _mpc_lowcomm_monitor_wrap_send
#break _mpc_lowcomm_monitor_get_client
#break lcp_context.c:80
#break lcp_recv
#break lcp_rndv_tag_handler
run
