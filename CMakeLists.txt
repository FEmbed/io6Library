idf_build_get_property(components_to_build BUILD_COMPONENTS)

set(src)
set(include)
set(priv_requires)

set(srcs    "Ethernet/socket.c"
            "Ethernet/wizchip_conf.c"
            "Ethernet/W6100/w6100.c"
            "Internet/DHCP4/dhcpv4.c"
            "Internet/DHCP6/dhcpv6.c"
            "Internet/DNS/dns.c"
            "Application/loopback/loopback.c"
            )
set(include "Ethernet" "Ethernet/W6100" "Internet/DHCP4" "Internet/DHCP6" "Internet/DNS" "Application" "Application/loopback")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS ${include}
                       PRIV_REQUIRES ${priv_requires})