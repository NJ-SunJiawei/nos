set(HAVE_LOCAL_NETINET_SCTP_H 1)
set(sctp_static_lib ${OS_CORE_DEPS_PATH}/libsctp/libsctp.a)
set(sctp_shared_lib ${OS_CORE_DEPS_PATH}/libsctp/libsctp.so)

configure_file(${OS_CORE_DEPS_PATH}/libsctp/inc/netinet/sctp.h  ${OS_CORE_BSAE_PATH}/inc/netinet/sctp.h)